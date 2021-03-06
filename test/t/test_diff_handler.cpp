/*
 * test_diff_handler.cpp
 *
 *  Created on: 28.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <diff_handler2.hpp>
#include <postgres_table.hpp>
#include <postgres_drivers/columns.hpp>
#include "update_location_handler_factory.hpp"
#include "expire_tiles_factory.hpp"

using sparse_mmap_array_t = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

void end_copy_nodes_tables(DiffHandler2& handler) {
    handler.write_new_nodes();
}

TEST_CASE("inserting new way works") {
    // set up handler and database connection
    //TODO clean up by providing a simpler constructor of MyHandler
    CerepsoConfig config;
    config.m_append = true;
    postgres_drivers::Columns node_columns(config.m_driver_config, postgres_drivers::TableType::POINT);
    postgres_drivers::Columns untagged_nodes_columns(config.m_driver_config, postgres_drivers::TableType::UNTAGGED_POINT);
    postgres_drivers::Columns way_linear_columns(config.m_driver_config, postgres_drivers::TableType::WAYS_LINEAR);
    postgres_drivers::Columns relation_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_OTHER);
    postgres_drivers::Columns node_ways_columns(config.m_driver_config, postgres_drivers::TableType::NODE_WAYS);
    postgres_drivers::Columns node_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_NODES);
    postgres_drivers::Columns way_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_WAYS);
    postgres_drivers::Columns relation_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_RELATIONS);
    PostgresTable nodes_table ("nodes", config, node_columns);
    PostgresTable untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    PostgresTable ways_table ("ways", config, way_linear_columns);
    PostgresTable relations_table("relations", config, relation_columns);
    PostgresTable node_ways_table("node_ways", config, node_ways_columns);
    node_ways_table.init();
    PostgresTable node_relations_table("node_relations", config, node_relations_columns);
    node_relations_table.init();
    PostgresTable way_relations_table("way_relations", config, way_relations_columns);
    PostgresTable relation_relations_table("relation_relations", config, relation_relations_columns);
    ExpireTilesFactory expire_tiles_factory;
    config.m_expiry_enabled = false;
    ExpireTiles* expire_tiles = expire_tiles_factory.create_expire_tiles(config);

    std::unique_ptr<sparse_mmap_array_t> index {new sparse_mmap_array_t()};

    // build OSM objects and call the callback methods of the handler
    static constexpr int buffer_size = 10 * 1000 * 1000;
    osmium::memory::Buffer node_buffer(buffer_size);
    osmium::memory::Buffer way_buffer(buffer_size);
    std::map<std::string, std::string> node_tags;
    // build and insert necessary nodes
    osmium::Node& node1 = test_utils::create_new_node(node_buffer, 1, 9.0, 50.1, node_tags);
    index->set(node1.id(), node1.location());
    node_buffer.commit();
    osmium::Node& node2 = test_utils::create_new_node(node_buffer, 2, 9.1, 50.0,  node_tags);
    index->set(node2.id(), node2.location());
    node_buffer.commit();
    osmium::Node& node3 = test_utils::create_new_node(node_buffer, 3, 9.2, 49.8, node_tags);
    index->set(node3.id(), node3.location());
    node_buffer.commit();

    std::unique_ptr<UpdateLocationHandler> location_handler = make_handler<sparse_mmap_array_t>(nodes_table, untagged_nodes_table,
            std::move(index));
    DiffHandler2 handler(nodes_table, &untagged_nodes_table, ways_table, relations_table, node_ways_table,
            node_relations_table, way_relations_table, relation_relations_table, config, expire_tiles, *location_handler);

    handler.node(node1);
    handler.node(node2);
    handler.node(node3);

    osmium::object_id_type current_id = 1;
    osmium::builder::WayBuilder way_builder(way_buffer);
    osmium::Way& way = static_cast<osmium::Way&>(way_builder.object());
    way.set_id(1);
    test_utils::set_dummy_osm_object_attributes(way);
    way_builder.set_user("");
    std::map<std::string, std::string> way_tags;
    way_tags.insert(std::pair<std::string, std::string>("highway", "trunk"));
    way_tags.insert(std::pair<std::string, std::string>("ref", "B 9"));
    test_utils::add_tags(way_buffer, &way_builder, way_tags);
    {
        osmium::builder::WayNodeListBuilder wnl_builder(way_buffer, &way_builder);
        const osmium::NodeRef nd_ref1 (1, osmium::Location(9.0, 50.1));
        const osmium::NodeRef nd_ref2 (2, osmium::Location(9.1, 50.0));
        const osmium::NodeRef nd_ref3 (3, osmium::Location(9.2, 49.8));
        wnl_builder.add_node_ref(nd_ref1);
        wnl_builder.add_node_ref(nd_ref2);
        wnl_builder.add_node_ref(nd_ref3);
    }

    std::string ways_table_copy_buffer = handler.prepare_query(way, ways_table, nullptr);

    SECTION("check if last char is \\n") {
        REQUIRE(ways_table_copy_buffer.back() == '\n');
    }
}


