/*
 * pgimporter.cpp
 *
 *  Created on: 16.06.2016
 *      Author: michael
 */

#include <iostream>
#include <unistd.h> // ftruncate
#include <functional>
#include <getopt.h>
#include <memory>
#include <system_error>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/io/any_input.hpp>
#include <postgres_drivers/columns.hpp>
#include "update_location_handler_factory.hpp"
#include "definitions.hpp"
#include "diff_handler1.hpp"
#include "diff_handler2.hpp"
#include "relation_collector.hpp"
#include "expire_tiles_factory.hpp"
#include "column_config_parser.hpp"
#include "definitions.hpp"
#include "addr_interpolation_handler.hpp"
#include "handler_collection.hpp"

/**
 * \mainpage
 * Cerepso is a tool to import OSM data into a PostgreSQL database (with
 * PostGIS and hstore extension) and to keep the database up to date with
 * minutely, hourly or daily diff files provided by OSM itself or third parties
 * like Geofabrik.
 *
 * Cerepso is inspired by [osm2pgsql](https://github.com/openstreetmap/osm2pgsql) and
 * tries to be as compatible as possible with it but removes some "dirty hacks". See
 * differences-from-osm2pgsql for details. Small parts of osm2pgsql source code and
 * almost all database optimization techniques
 * have been reused by Cerepso.
 *
 * Cerepso makes much use of the [Osmium](http://osmcode.org/libosmium/) library for
 * reading OSM data and building geometries.
 *
 * Its written in C++11 and available under GPLv2.
 */


using sparse_mmap_array_t = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;
using dense_mmap_array_t = osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;
using sparse_file_array_t = osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, osmium::Location>;
using dense_file_array_t = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;


std::unique_ptr<dense_file_array_t> load_index(const char* filename, const std::string& type) {
    int fd = ::open(filename, O_RDWR);
    if (fd == -1) {
        std::cerr << "Can not open location cache file '" << filename << "': " << std::strerror(errno) << "\n";
        throw std::runtime_error{"opening file failed"};
    }
    if (type == "dense_mmap_array" || type == "dense_file_array") {
        return std::unique_ptr<dense_file_array_t>{new dense_file_array_t{fd}};
    }
    throw std::runtime_error{"unsupported index type"};
}

template <typename TIndex>
void dump_index_manually(index_type* location_index, const int fd) {
    using index_value = osmium::Location;
    constexpr const size_t VALUE_SIZE = sizeof(index_value);
    // cast index to desired type
    TIndex* index = static_cast<TIndex*>(location_index);
    size_t array_size = ((index->end() - 1)->first + 1);
    if (::ftruncate(fd, array_size * VALUE_SIZE) == -1) {
        throw std::system_error{errno, std::system_category(), "Failed to truncate file."};
    }
    osmium::util::TypedMemoryMapping<index_value> mapping(array_size, osmium::util::MemoryMapping::mapping_mode::write_shared, fd, 0);
    index_value* ptr = mapping.begin();
    size_t array_idx = 0;
    typename TIndex::iterator it = index->begin();
    while (array_idx < array_size) {
        if (it->first == array_idx) {
            ptr[array_idx] = it->second;
            ++it;
        } else {
            ptr[array_idx] = osmium::index::empty_value<index_value>();
        }
        ++array_idx;
    }
    mapping.unmap();
}

void dump_index(index_type* location_index, CerepsoConfig& config) {
    if (config.m_driver_config.updateable) {
        std::cerr << "Dumping location cache to file ";
        time_t ts = time(NULL);
        const int fd = ::open(config.m_flat_nodes.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666); // NOLINT(hicpp-signed-bitwise)
        if (fd == -1) {
            std::cerr << "Can not open location cache file '" << config.m_flat_nodes << "': " << std::strerror(errno) << "\n";
            std::exit(1);
        }
        try {
            location_index->dump_as_array(fd);
        } catch (std::runtime_error&) {
            if (config.m_location_handler == "sparse_mmap_array") {
                dump_index_manually<sparse_mmap_array_t>(location_index, fd);
            } else if (config.m_location_handler == "sparse_mem_array") {
                throw std::runtime_error{"This index cannot be dumped as an array."};
            }
        }
        std::cerr << "… needed " << static_cast<int>(time(NULL) - ts) << " seconds" << std::endl;
    }
}

/**
 * \brief print program usage instructions and terminate the program
 *
 * \param argv command line argument array (from main method)
 * \param message error message to output (optional)
 */
void print_help(char* argv[], std::string message = "", const int return_code = 1) {
    if (message != "") {
        std::cerr << message << "\n";
    }
    std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE]\n" \
    "  -a, --append                     this is a diff import\n" \
    "  -A, --areas                      enable area support, disables updateability\n" \
    "  --associated-streets             Apply tags of relations of type associatedStreet to their members\n" \
    "  -d, --database-name              database name\n" \
    "  -e FILE, --expire-tiles=FILE     write an expiry_tile list to FILE\n" \
    "  --expire-relations=SETTING       expiration setting for relations: NONE, ALL, NO_ROUTES\n" \
    "  -f PATH, --flat-nodes=PATH       Flatnodes file path.\n" \
    "                                     Import mode: dump node locations to this path.\n" \
    "                                     Append mode: read node locations from here and not from the untagged_nodes"
    "                                     table.\n" \
    "  -g, --no-geom-indexes            don't create any geometry indexes\n" \
    "  -H, --hstore                     Add objects with tags even if they don't have any tag matching a column.\n" \
    "  -G, --all-geom-indexes           create geometry indexes on all tables (otherwise not on untagged nodes table),\n" \
    "                                   overrides -g"
    "  -I, --no-id-index                don't create an index on osm_id columns\n" \
    "  --interpolate-addr               create virtual address points based on address interpolations\n" \
    "  --min-zoom=ZOOM                  minimum zoom for expire_tile list\n" \
    "  --max-zoom=ZOOM                  maximum zoom for expire_tile list\n" \
    "  --metadata=OPTARG                import specified metadata fields. Permitted values are \"none\", \"all\" and\n" \
    "                                   one or many of the following values concatenated by \"+\": version,\n" \
    "                                   timestamp, user, uid, changeset.\n" \
    "                                   Valid examples: \"none\" (no metadata), \"all\" (all fields),\n" \
    "                                   \"version+timestamp\" (only version and timestamp).\n" \
    "  -s FILE, --style=FILE            Osm2pgsql style file (default: ./default.style)\n" \
    "  -l, --location-handler=HANDLER   use HANDLER as location handler\n" \
    "  -o, --no-order-by-geohash        don't order tables by ST_GeoHash\n" \
    "  -O, --one                        Don't create tables and columns needed for updates.\n" \
    "  --untagged-nodes                 Create a table for untagged nodes (in parallel to flatnodes file on disk).\n\n";
    exit(return_code);
}


int main(int argc, char* argv[]) {
    // parsing command line arguments
    static struct option long_options[] = {
            {"help",   no_argument, 0, 'h'},
            {"associated-streets", no_argument, 0, 203},
            {"interpolate-addr", no_argument, 0, 205},
            {"debug",  no_argument, 0, 'D'},
            {"database",  required_argument, 0, 'd'},
            {"expire-tiles",  required_argument, 0, 'e'},
            {"expire-relations", required_argument, 0, 202},
            {"flat-nodes", required_argument, 0, 'f'},
            {"hstore", no_argument, 0, 'H'},
            {"no-geom-indexes", no_argument, 0, 'g'},
            {"all-geom-indexes", no_argument, 0, 'G'},
            {"min-zoom",  required_argument, 0, 200},
            {"max-zoom",  required_argument, 0, 201},
            {"metadata",  required_argument, 0, 'm'},
            {"no-order-by-geohash", no_argument, 0, 'o'},
            {"one", no_argument, 0, 'O'},
            {"append", no_argument, 0, 'a'},
            {"areas", no_argument, 0, 'A'},
            {"style", required_argument, 0, 's'},
            {"no-id-index", no_argument, 0, 'I'},
            {"location-handler", required_argument, 0, 'l'},
            {"untagged-nodes", no_argument, 0, 204},
            {0, 0, 0, 0}
        };
    CerepsoConfig config;
    while (true) {
        int c = getopt_long(argc, argv, "hDd:e:E:f:IHoOagGl:m:us:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help(argv, "", 0);
                break;
            case 'D':
                config.m_driver_config.m_debug = true;
                break;
            case 'd':
                config.m_driver_config.m_database_name = optarg;
                break;
            case 'e':
                config.m_expire_tiles = optarg;
                config.m_expiry_enabled = true;
                break;
            case 'f':
                config.m_flat_nodes = optarg;
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
            case 'O':
                config.m_driver_config.updateable = false;
                break;
            case 'a':
                config.m_append = true;
                config.m_driver_config.updateable = true;
                break;
            case 'A':
                config.m_areas = true;
                break;
            case 203:
                config.m_associated_streets = true;
                break;
            case 'I':
                config.m_id_index = false;
                break;
            case 'H':
                config.m_hstore_all = true;
                break;
            case 'l':
                config.m_location_handler = optarg;
                break;
            case 'm':
                config.m_driver_config.metadata = osmium::metadata_options(optarg);
                break;
            case 's':
                config.m_style_file = optarg;
                break;
            case 200:
                config.m_min_zoom = atoi(optarg);
                break;
            case 201:
                config.m_max_zoom = atoi(optarg);
                break;
            case 202:
                if (optarg) {
                    if (!strcmp(optarg, "ALL")) {
                        config.m_expire_options = CerepsoConfig::ExpireRelationsOptions::ALL;
                        break;
                    } else if (!strcmp(optarg, "NONE")) {
                        config.m_expire_options = CerepsoConfig::ExpireRelationsOptions::NO_RELATIONS;
                        break;
                    } else if (!strcmp(optarg, "NO_ROUTES")) {
                        config.m_expire_options = CerepsoConfig::ExpireRelationsOptions::NO_ROUTE_RELATIONS;
                        break;
                    }
                }
                print_help(argv, "ERROR option --expire-relations: Wrong parameter.");
                break;
            case 204:
                config.m_driver_config.untagged_nodes = true;
                break;
            case 205:
                config.m_address_interpolations = true;
                break;
            default:
                exit(1);
        }
    }
    if ((config.m_expiry_enabled) && (config.m_max_zoom == 0)) {
        // set max zoom to min zoom if the user uses this tool like osm2pgsql
        config.m_max_zoom = config.m_min_zoom;
    }
    if (config.m_driver_config.untagged_nodes != (config.m_flat_nodes == "")) {
        std::cerr << "WARNING: This is an import without ability to update! Add either\n" \
                "--untagged-nodes or --flat-nodes to the list of command line options.\n";
    }
    if (config.m_append && config.m_flat_nodes != "" && config.m_driver_config.untagged_nodes) {
        print_help(argv, "Ambigous command line options. A flat nodes file cannot be specified together with --untagged-nodes.");
    }
    if (config.m_append && !config.m_flat_nodes.empty() && config.m_location_handler != "dense_file_array") {
        std::cerr << "WARNING: You are using --append with a flatnodes file but the wrong location index type.\n" \
                "Flat node files can be only used with the dense_file_array location index in update mode.\n" \
                "--location-handler will be ignored and set to \"dense_file_array\"\n";
        config.m_location_handler = "dense_file_array";
    }

    int remaining_args = argc - optind;
    if (remaining_args != 1) {
        print_help(argv, "ERROR: wrong arguments.");
    } else {
        config.m_osm_file =  argv[optind];
    }

    // column definitions, parse config
    ColumnConfigParser config_parser {config};
    config_parser.parse();

    postgres_drivers::Columns untagged_nodes_columns(config.m_driver_config, postgres_drivers::TableType::UNTAGGED_POINT);
    postgres_drivers::Columns relation_other_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_OTHER);
    postgres_drivers::Columns node_ways_columns(config.m_driver_config, postgres_drivers::TableType::NODE_WAYS);
    postgres_drivers::Columns node_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_NODES);
    postgres_drivers::Columns way_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_WAYS);
    postgres_drivers::Columns relation_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_RELATIONS);
    postgres_drivers::Columns interpolation_columns(postgres_drivers::Columns::addr_interpolation_columns());

    time_t ts = time(NULL);
    PostgresTable nodes_table = config_parser.make_point_table("planet_osm_");
    nodes_table.init();
    PostgresTable untagged_nodes_table {"untagged_nodes", config, std::move(untagged_nodes_columns)};
    if (config.m_driver_config.untagged_nodes) {
        untagged_nodes_table.init();
    }
    PostgresTable node_ways_table {"node_ways", config, std::move(node_ways_columns)};
    PostgresTable node_relations_table {"node_relations", config, std::move(node_relations_columns)};
    PostgresTable way_relations_table {"way_relations", config, std::move(way_relations_columns)};
    PostgresTable relation_relations_table {"relation_relations", config, std::move(relation_relations_columns)};
    if (config.m_driver_config.updateable) {
        node_ways_table.init();
        node_relations_table.init();
        way_relations_table.init();
        relation_relations_table.init();
    }
    PostgresTable ways_linear_table = config_parser.make_line_table("planet_osm_");
    ways_linear_table.init();
    PostgresTable areas_table = config_parser.make_polygon_table("planet_osm_");
    if (config.m_areas) {
        areas_table.init();
    }
    PostgresTable interpolated_table {"interpolated_addresses", config,
        std::move(interpolation_columns)};
    AddrInterpolationHandler* interpolated_handler;
    if (config.m_address_interpolations) {
        interpolated_table.init();
        interpolated_handler = new AddrInterpolationHandler(interpolated_table);
    }

    osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonManager<osmium::area::Assembler>* mp_manager;
    if (config.m_areas) {
        mp_manager = new osmium::area::MultipolygonManager<osmium::area::Assembler>(assembler_config);
    }

    // TODO cleanup: add a HandlerFactory which returns the handler we need
    if (config.m_append) { // append mode, reading diffs
        // load index from file
        std::unique_ptr<dense_file_array_t> location_index;
        if (config.m_flat_nodes != "") {
            location_index = load_index(config.m_flat_nodes.c_str(), config.m_location_handler);
        }
        postgres_drivers::Columns untagged_nodes_columns2(config.m_driver_config, postgres_drivers::TableType::UNTAGGED_POINT);
        PostgresTable locations_untagged_table {"untagged_nodes", config, std::move(untagged_nodes_columns2)};
        postgres_drivers::Columns nodes_columns2(config.m_driver_config, postgres_drivers::TableType::POINT);
        PostgresTable locations_table {"planet_osm_point", config, std::move(nodes_columns2)};
        locations_untagged_table.init();
        locations_table.init();
        std::unique_ptr<UpdateLocationHandler> location_handler = make_handler<dense_file_array_t>(
                locations_table, locations_untagged_table, std::move(location_index));
        location_handler->ignore_errors();
        osmium::io::Reader reader1(config.m_osm_file, osmium::osm_entity_bits::nwr);
        PostgresTable relations_table("relations", config, std::move(relation_other_columns));
        relations_table.init();
        // send BEGIN to all tables
        relations_table.send_begin();
        nodes_table.send_begin();
        untagged_nodes_table.send_begin();
        ways_linear_table.send_begin();

        ExpireTilesFactory expire_tiles_factory;
        ExpireTiles* expire_tiles = expire_tiles_factory.create_expire_tiles(config);
        DiffHandler1 append_handler1(config, nodes_table, &untagged_nodes_table, ways_linear_table, relations_table, node_ways_table,
                node_relations_table, way_relations_table, relation_relations_table, expire_tiles, *location_handler, &areas_table);
        // The location handler has not to be passed to the visitor in pass 1.
        if (config.m_areas) {
            osmium::apply(reader1, append_handler1, *mp_manager);
        } else {
            osmium::apply(reader1, append_handler1);
        }
        reader1.close();

        if (config.m_areas) {
            // necessary because we don't use osmium::relations::read_relations
            mp_manager->prepare_for_lookup();
        }

        osmium::io::Reader reader2(config.m_osm_file, osmium::osm_entity_bits::nwr);
        DiffHandler2 append_handler2(config, nodes_table, &untagged_nodes_table, ways_linear_table, relations_table, node_ways_table,
                node_relations_table, way_relations_table, relation_relations_table, expire_tiles, *location_handler, &areas_table, mp_manager);
        if (config.m_areas) {
            osmium::apply(reader2, *location_handler, append_handler2,
                    mp_manager->handler([&append_handler2](osmium::memory::Buffer&& buffer) {
                        osmium::apply(buffer, append_handler2);
                    })
                );
        } else {
            osmium::apply(reader2, *location_handler, append_handler2);
        }
        append_handler2.after_relations();

        // work on incomplete multipolygon and boundary relations
        if (config.m_areas) {
            // Flush the content of the output buffer of the area assembler.
            append_handler2.flush();
            areas_table.start_copy();
            mp_manager->for_each_incomplete_relation([&](const osmium::relations::RelationHandle& handle){
                append_handler2.incomplete_relation(handle);
            });
            // Flush the content of the output buffer of the area assembler.
            append_handler2.flush();
            areas_table.end_copy();
        }

        reader2.close();
    } else {
        const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
        auto location_index = map_factory.create_map(config.m_location_handler);
        location_handler_type location_handler(*location_index);
        ts = time(NULL);
        std::cerr << "Pass 1 (relations)";
        RelationCollector rel_collector(config, relation_other_columns);
        AssociatedStreetRelationManager assoc_manager;
        osmium::io::Reader reader1{config.m_osm_file, osmium::osm_entity_bits::way | osmium::osm_entity_bits::relation};
        HandlerCollection handlers_collection1;
        handlers_collection1.add(rel_collector);
        if (config.m_areas) {
            handlers_collection1.add<osmium::area::MultipolygonManager<osmium::area::Assembler>>(*mp_manager);
        }
        if (config.m_associated_streets) {
            handlers_collection1.add<AssociatedStreetRelationManager>(assoc_manager);
        }
        if (config.m_address_interpolations) {
            handlers_collection1.add<AddrInterpolationHandler>(*interpolated_handler);
        }
        osmium::apply(reader1, handlers_collection1);
        reader1.close();
        if (config.m_areas) {
            // necessary because we don't use osmium::relations::read_relations
            mp_manager->prepare_for_lookup();
        }
        rel_collector.prepare_for_lookup();
        std::cerr << "… needed " << static_cast<int>(time(NULL) - ts) << " seconds" << std::endl;

        ts = time(NULL);
        std::cerr << "Pass 2 (nodes and ways; writing everything to database)" << std::endl;
        osmium::io::Reader reader2(config.m_osm_file);
        ImportHandler handler(config, nodes_table, &untagged_nodes_table, ways_linear_table, &assoc_manager, &areas_table, &node_ways_table,
                &node_relations_table, &way_relations_table, &relation_relations_table);
        HandlerCollection handlers_collection2;
        handlers_collection2.add(rel_collector.handler());
        if (config.m_address_interpolations) {
            interpolated_handler->after_pass1();
            handlers_collection2.add(interpolated_handler->handler());
        }
        handlers_collection2.add<ImportHandler>(handler);
        if (config.m_areas) {
            osmium::apply(reader2, location_handler, handlers_collection2,
                mp_manager->handler([&handler](osmium::memory::Buffer&& buffer) {
                    osmium::apply(buffer, handler);
                })
            );
            delete mp_manager;
        } else {
            osmium::apply(reader2, location_handler, handlers_collection2);
        }
        reader2.close();
        std::cerr << "… needed " << static_cast<int> (time(NULL) - ts) << " seconds" << std::endl;
        // dump location index
        if (config.m_flat_nodes != "") {
            ts = time(NULL);
            std::cerr << "Dumping location cache as array to " << config.m_flat_nodes << " ...";
            dump_index(location_index.get(), config);
            std::cerr << " needed " << static_cast<int> (time(NULL) - ts) << " seconds" << std::endl;
        }
    }
    if (config.m_address_interpolations) {
        delete interpolated_handler;
    }
}
