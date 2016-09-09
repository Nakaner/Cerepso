/*
 * pgimporter.cpp
 *
 *  Created on: 16.06.2016
 *      Author: michael
 */

#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>
#include "myhandler.hpp"
#include "diff_handler1.hpp"
#include "columns.hpp"
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/area/assembler.hpp>
//#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/io/any_input.hpp>
#include <iostream>
#include <getopt.h>
#include "relation_collector.hpp"
#include "expire_tiles_factory.hpp"

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;


int main(int argc, char* argv[]) {
    static struct option long_options[] = {
            {"help",   no_argument, 0, 'h'},
            {"debug",  no_argument, 0, 'D'},
            {"database",  required_argument, 0, 'd'},
            {"expire-tiles",  required_argument, 0, 'e'},
            {"expiry-generator",  required_argument, 0, 'E'},
            {"no-geom-indexes", no_argument, 0, 'g'},
            {"all-geom-indexes", no_argument, 0, 'G'},
            {"min-zoom",  required_argument, 0, 200},
            {"max-zoom",  required_argument, 0, 201},
            {"no-order-by-geohash", no_argument, 0, 'o'},
            {"append", no_argument, 0, 'a'},
            {"no-id-index", no_argument, 0, 'I'},
            {"location-handler", required_argument, 0, 'l'},
            {"no-usernames", no_argument, 0, 'u'},
            {0, 0, 0, 0}
        };
    Config config;
    while (true) {
        int c = getopt_long(argc, argv, "hDd:e:E:IoagGl:u", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                //print_help();
                exit(0);
            case 'D':
                config.m_debug = true;
                break;
            case 'd':
                config.m_database_name = optarg;
                break;
            case 'e':
                config.m_expire_tiles = optarg;
                break;
            case 'E':
                config.m_expiry_type = optarg;
                break;
            case 'g':
                config.m_geom_indexes = false;
                break;
            case 'G':
                config.m_all_geom_indexes = true;
                break;
            case 'o':
                config.m_order_by_geohash = false;
                break;
            case 'a':
                config.m_append = true;
                break;
            case 'I':
                config.m_id_index = false;
                break;
            case 'l':
                config.m_location_handler = optarg;
                break;
            case 'u':
                config.m_usernames = false;
                break;
            case 200:
                config.m_min_zoom = atoi(optarg);
                break;
            case 201:
                config.m_max_zoom = atoi(optarg);
                break;
            default:
                exit(1);
        }
    }
    if ((config.m_expiry_type != "dummy") && (config.m_max_zoom == 0)) {
        // set max zoom to min zoom if the user uses this tool like osm2pgsql
        config.m_max_zoom = config.m_min_zoom;
    }

    int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE]\n" \
        "  -a, --append                     this is a diff import\n" \
        "  -d, --database-name              database name\n" \
        "  -e FILE, --expire-tiles=FILE     write an expiry_tile list to FILE\n" \
        "  -E TYPE, --expiry-generator=TYPE choose TYPE as expiry list generator\n" \
        "  -g, --no-geom-indexes            don't create any geometry indexes\n" \
        "  -G, --all-geom-indexes           create geometry indexes on all tables (otherwise not on untagged nodes table),\n" \
        "                                   overrides -g"
        "  -I, --no-id-index                don't create an index on osm_id columns\n" \
        "  --min-zoom=ZOOM                  minimum zoom for expire_tile list\n" \
        "  --max-zoom=ZOOM                  maximum zoom for expire_tile list\n" \
        "  -l, --location-handler=HANDLER   use HANDLER as location handler\n" \
        "  -o, --no-order-by-geohash        don't order tables by ST_GeoHash\n" \
        "  -u, --no-usernames               don't insert user names into the database\n" << std::endl;
        exit(1);
    } else {
        config.m_osm_file =  argv[optind];
    }


    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
    auto location_index = map_factory.create_map(config.m_location_handler);
    location_handler_type location_handler(*location_index);
    Columns node_columns(config, TableType::POINT);
    Columns untagged_nodes_columns(config, TableType::UNTAGGED_POINT);
    Columns way_linear_columns(config, TableType::WAYS_LINEAR);
    Columns relation_other_columns(config, TableType::RELATION_OTHER);

    time_t ts = time(NULL);
    Table nodes_table ("nodes", config, node_columns);
    Table untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    Table ways_linear_table ("ways", config, way_linear_columns);
    // TODO cleanup: add a HandlerFactory which returns the handler we need
    if (config.m_append) { // append mode, reading diffs
        osmium::io::Reader reader(config.m_osm_file, osmium::osm_entity_bits::nwr);
        Table relations_table("relations", config, relation_other_columns);
        ExpireTilesFactory expire_tiles_factory;
        if (config.m_expire_tiles == "") {
            config.m_expiry_type = "";
        }
        ExpireTiles* expire_tiles = expire_tiles_factory.create_expire_tiles(config);
        DiffHandler1 append_handler(config, nodes_table, untagged_nodes_table, ways_linear_table, relations_table, expire_tiles);
        while (osmium::memory::Buffer buffer = reader.read()) {
            for (auto it = buffer.cbegin(); it != buffer.cend(); ++it) {
                if (it->type_is_in(osmium::osm_entity_bits::node)) {
                    append_handler.node(static_cast<const osmium::Node&>(*it));
                }
                else if (it->type_is_in(osmium::osm_entity_bits::way)) {
                    append_handler.way(static_cast<const osmium::Way&>(*it));
                }
                else if (it->type_is_in(osmium::osm_entity_bits::relation)) {
                    append_handler.relation(static_cast<const osmium::Relation&>(*it));
                }
            }
        }
        reader.close();
    } else {
        const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
        auto location_index = map_factory.create_map(config.m_location_handler);
        location_handler_type location_handler(*location_index);
        ts = time(NULL);
        std::cerr << "Pass 1 (relations)";
        osmium::io::Reader reader_rel(config.m_osm_file);
        RelationCollector rel_collector(config, relation_other_columns);
        rel_collector.read_relations(reader_rel);
        reader_rel.close();
        std::cerr << "… needed " << static_cast<int>(time(NULL) - ts) << " seconds" << std::endl;

        ts = time(NULL);
        std::cerr << "Pass 2 (nodes and ways; writing everything to database)" << std::endl;
        osmium::io::Reader reader2(config.m_osm_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);
        MyHandler handler(config, nodes_table, untagged_nodes_table, ways_linear_table);
        osmium::apply(reader2, location_handler, handler, rel_collector.handler());
        reader2.close();
        std::cerr << "… needed " << static_cast<int> (time(NULL) - ts) << " seconds" << std::endl;
    }
}
