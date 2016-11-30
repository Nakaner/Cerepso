/*
 * test_prepare_relation_query.cpp
 *
 *  Created on:  2016-11-30
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"
#include <table.hpp>
#include <columns.hpp>
#include <diff_handler2.hpp>
#include <expire_tiles_factory.hpp>

void end_copy_ways_tables(DiffHandler2& handler) {
    handler.write_new_ways();
}

TEST_CASE("check if preparing a query to insert a relation works") {
    static constexpr int buffer_size = 10 * 1000 * 1000;

    Config config;
    Columns node_columns(config, TableType::POINT);
    Columns untagged_nodes_columns(config, TableType::UNTAGGED_POINT);
    Columns way_linear_columns(config, TableType::WAYS_LINEAR);
    Columns relation_columns(config, TableType::RELATION_OTHER);
    Table nodes_table ("nodes", config, node_columns);
    Table untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    Table ways_table ("ways", config, way_linear_columns);
    Table relations_table("relations", config, relation_columns);

    // create some nodes and add them to the location handler
    osmium::memory::Buffer buffer(buffer_size);
    std::map<std::string, std::string> node_tags;
    const osmium::NodeRef nd_ref1 (1, osmium::Location(9.0, 50.1));
    const osmium::NodeRef nd_ref2 (2, osmium::Location(9.1, 50.0));
    const osmium::NodeRef nd_ref3 (3, osmium::Location(9.2, 49.8));
    osmium::Node& node1 = test_utils::create_new_node_from_node_ref(buffer, nd_ref1, node_tags);
    buffer.commit();
    osmium::Node& node2 = test_utils::create_new_node_from_node_ref(buffer, nd_ref2,  node_tags);
    buffer.commit();
    osmium::Node& node3 = test_utils::create_new_node_from_node_ref(buffer, nd_ref3, node_tags);
    buffer.commit();

    // create some ways and add them to the buffer
    std::map<std::string, std::string> way_tags;
    way_tags.insert(std::pair<std::string, std::string>("highway", "track"));
    way_tags.insert(std::pair<std::string, std::string>("surface", "fine_gravel"));
    std::vector<const osmium::NodeRef*> node_refs1 {&nd_ref1, &nd_ref2};
    std::vector<const osmium::NodeRef*> node_refs2 {&nd_ref2, &nd_ref3};
    osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, way_tags);
    buffer.commit();
    osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, way_tags);
    buffer.commit();

    // create a relations and add it to the buffer
    std::map<std::string, std::string> relation_tags;
    relation_tags.insert(std::pair<std::string, std::string>("type", "route"));
    relation_tags.insert(std::pair<std::string, std::string>("route", "bicycle"));
    relation_tags.insert(std::pair<std::string, std::string>("name", "Neckarradweg"));
    std::vector<osmium::item_type> member_types {osmium::item_type::way, osmium::item_type::way};
    std::vector<osmium::object_id_type> member_ids {1, 2};
    std::vector<std::string> member_roles {"forward", "forward"};

    osmium::Relation& relation = test_utils::create_relation(buffer, 1, relation_tags, member_ids, member_types, member_roles);
    buffer.commit();

    // We use a DiffHandler2 to insert nodes and ways into the database
    ExpireTilesFactory expire_tiles_factory;
    config.m_expiry_type = "";
    ExpireTiles* expire_tiles = expire_tiles_factory.create_expire_tiles(config);
    config.m_append = true;
    DiffHandler2 handler(nodes_table, untagged_nodes_table, ways_table, relations_table, config, expire_tiles);
    handler.node(node1);
    handler.node(node2);
    handler.node(node3);
    handler.way(way1);
    handler.way(way2);
    end_copy_ways_tables(handler);
    std::string copy_buffer;
    handler.insert_relation(relation, copy_buffer);

    // split string at tabulators
    size_t current = 0;
    size_t next = copy_buffer.find("\t");
    size_t column_count = 0;
    while (next != std::string::npos) {
        // check if \t is not escaped
        if (copy_buffer.at(next - 1) == '\\') {
            continue;
        }
        column_count++;
        // look for next \t
        current = next;
        next = copy_buffer.find("\t", current + 1);
    }
    // Increase column count for last column because it does not end with \t.
    column_count++;

    SECTION("correct number of columns") {
        REQUIRE(column_count == 12);
    }
}

