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

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;


int main(int argc, char* argv[]) {
    static struct option long_options[] = {
            {"help",   no_argument, 0, 'h'},
            {"debug",  no_argument, 0, 'D'},
            {"database",  required_argument, 0, 'd'},
            {"all-geom-indexes", no_argument, 0, 'G'},
            {"no-order-by-geohash", no_argument, 0, 'o'},
            {"append", no_argument, 0, 'a'},
            {"no-id-index", no_argument, 0, 'I'},
            {0, 0, 0, 0}
        };
    Config config;
    while (true) {
        int c = getopt_long(argc, argv, "hDdIoaG", long_options, 0);
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
            default:
                exit(1);
        }
    }

    int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE]\n" \
        "  -a, --all-geom-indexes  geometry indexes on all tables (otherwise not on untagged nodes table)" \
        "  -o, --no-order-by-geohash  don't order tables by ST_GeoHash" << std::endl;
        exit(1);
    } else {
        config.m_osm_file =  argv[optind];
    }


    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
    auto location_index = map_factory.create_map("sparse_mmap_array");
    location_handler_type location_handler(*location_index);
    Columns node_columns(config, TableType::POINT);
    Columns untagged_nodes_columns(config, TableType::UNTAGGED_POINT);
    Columns way_linear_columns(config, TableType::WAYS_LINEAR);
    Columns relation_other_columns(config, TableType::RELATION_OTHER);

    time_t ts = time(NULL);
    if (config.m_append) { // append mode, reading diffs
        osmium::io::Reader reader(config.m_osm_file, osmium::osm_entity_bits::nwr);
        DiffHandler1 append_handler(config, node_columns, untagged_nodes_columns, way_linear_columns, relation_other_columns);
        while (osmium::memory::Buffer buffer = reader.read()) {
            for (auto it = buffer.cbegin(); it != buffer.cend(); ++it) {
                if (it->type_is_in(osmium::osm_entity_bits::node)) {
                    append_handler.node(static_cast<const osmium::Node&>(*it));
                }
            }
        }
        reader.close();
    } else {
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
        MyHandler handler(config, node_columns, untagged_nodes_columns, way_linear_columns);
        osmium::apply(reader2, location_handler, handler, rel_collector.handler());
        reader2.close();
        std::cerr << "… needed " << static_cast<int> (time(NULL) - ts) << " seconds" << std::endl;
    }
}
