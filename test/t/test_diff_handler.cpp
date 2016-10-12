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
#include <diff_handler2.hpp>
#include <table.hpp>
#include <columns.hpp>
#include "expire_tiles_factory.hpp"

void end_copy_nodes_tables(DiffHandler2& handler) {
    handler.write_new_nodes();
}

TEST_CASE("inserting new way works") {
    static constexpr int buffer_size = 10 * 1000 * 1000;
    osmium::memory::Buffer node_buffer(buffer_size);
    osmium::memory::Buffer way_buffer(buffer_size);
    std::map<std::string, std::string> node_tags;
    osmium::Node& node1 = test_utils::create_new_node(node_buffer, 1, 9.0, 50.1, node_tags);
    osmium::Node& node2 = test_utils::create_new_node(node_buffer, 2, 9.1, 50.0,  node_tags);
    osmium::Node& node3 = test_utils::create_new_node(node_buffer, 3, 9.2, 49.8, node_tags);

    osmium::object_id_type current_id = 1;
    osmium::builder::WayBuilder way_builder(way_buffer);
    static_cast<osmium::Way&>(way_builder.object()).set_id(1);
    test_utils::set_dummy_osm_object_attributes(static_cast<osmium::OSMObject&>(way_builder.object()));
    way_builder.add_user("foo");
    osmium::builder::WayNodeListBuilder wnl_builder(way_buffer, &way_builder);
    const osmium::NodeRef nd_ref1 (1, osmium::Location(9.0, 50.1));
    const osmium::NodeRef nd_ref2 (2, osmium::Location(9.1, 50.0));
    const osmium::NodeRef nd_ref3 (3, osmium::Location(9.2, 49.8));
    std::map<std::string, std::string> way_tags;
    way_tags.insert(std::pair<std::string, std::string>("highway", "trunk"));
    way_tags.insert(std::pair<std::string, std::string>("ref", "B 9"));
    wnl_builder.add_node_ref(nd_ref1);
    wnl_builder.add_node_ref(nd_ref2);
    wnl_builder.add_node_ref(nd_ref3);
    test_utils::add_tags(way_buffer, way_builder, way_tags);
    way_buffer.commit();

    //TODO clean up by providing a simpler constructor of MyHandler
    Config config;
    Columns node_columns(config, TableType::POINT);
    Columns untagged_nodes_columns(config, TableType::UNTAGGED_POINT);
    Columns way_linear_columns(config, TableType::WAYS_LINEAR);
    Columns relation_columns(config, TableType::RELATION_OTHER);
    Table nodes_table ("nodes", config, node_columns);
    Table untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    Table ways_table ("ways", config, way_linear_columns);
    Table relations_table("relations", config, relation_columns);
    ExpireTilesFactory expire_tiles_factory;
    config.m_expiry_type = "";
    ExpireTiles* expire_tiles = expire_tiles_factory.create_expire_tiles(config);
    DiffHandler2 handler(nodes_table, untagged_nodes_table, ways_table, relations_table, config, expire_tiles);

    // insert necessary nodes
    handler.node(node1);
    handler.node(node2);
    handler.node(node3);
    std::cout << "WOOORKS\n";
    end_copy_nodes_tables(handler);
    std::string ways_table_copy_buffer;
    handler.insert_way(way_builder.object(), ways_table_copy_buffer);

    SECTION("check if last char is \\n") {
//        std::cout << handler.m_ways_table_copy_buffer << std::endl;
        REQUIRE(ways_table_copy_buffer.back() == '\n');
    }
}


