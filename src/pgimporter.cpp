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
#include "columns.hpp"
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
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
            {"all-geom-indexes", required_argument, 0, 'a'},
            {"no-order-by-geohash", required_argument, 0, 'o'},
            {0, 0, 0, 0}
        };
    Config config;
    while (true) {
        int c = getopt_long(argc, argv, "hDd", long_options, 0);
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
            case 'a':
                config.m_all_geom_indexes = true;
                break;
            case 'o':
                config.m_order_by_geohash = false;
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
    Columns way_polygon_columns(config, TableType::WAYS_POLYGON);
    Columns relation_polygon_columns(config, TableType::RELATION_POLYGON);
    Columns relation_other_columns(config, TableType::RELATION_OTHER);

    time_t ts = time(NULL);

    std::cerr << "Pass 1 (multipolygon relations)";
    osmium::io::Reader reader1(config.m_osm_file, osmium::osm_entity_bits::relation);
    osmium::area::Assembler::config_type assembler_config;
    std::shared_ptr<osmium::area::MultipolygonCollector<osmium::area::Assembler>> collector(new osmium::area::MultipolygonCollector<osmium::area::Assembler>(assembler_config));
    collector->read_relations(reader1);
    reader1.close();
    std::cerr << "… needed " << static_cast<int>(time(NULL) - ts) << " seconds" << std::endl;

    ts = time(NULL);
    std::cerr << "Pass 2 (other relations)";
    osmium::io::Reader reader_rel(config.m_osm_file);
    RelationCollector rel_collector(collector, config, relation_other_columns);
    rel_collector.read_relations(reader_rel);
    reader_rel.close();
    std::cerr << "… needed " << static_cast<int>(time(NULL) - ts) << " seconds" << std::endl;

    ts = time(NULL);
    std::cerr << "Pass 3 (nodes and ways; writing to database)" << std::endl;
    osmium::io::Reader reader2(config.m_osm_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);
    MyHandler handler(config, node_columns, untagged_nodes_columns, way_linear_columns, way_polygon_columns, relation_polygon_columns);
    osmium::apply(reader2, location_handler, handler, collector->handler([&handler](const osmium::memory::Buffer& area_buffer) {
            osmium::apply(area_buffer, handler);
            }),
            rel_collector.handler());
    reader2.close();
    std::cerr << "… needed " << static_cast<int> (time(NULL) - ts) << " seconds" << std::endl;
}
