/*
 * test_prepare_relation_query.cpp
 *
 *  Created on:  2016-11-30
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <postgres_table.hpp>
#include <postgres_drivers/columns.hpp>
#include <diff_handler2.hpp>
#include <expire_tiles_factory.hpp>
#include <update_location_handler_factory.hpp>

using sparse_mmap_array_t = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

void end_copy_ways_tables(DiffHandler2& handler) {
    handler.write_new_ways();
}

TEST_CASE("check if preparing a query to insert a relation works") {
    static constexpr int buffer_size = 10 * 1000 * 1000;

    CerepsoConfig config;
    config.m_append = true;
    postgres_drivers::Columns node_columns(config.m_driver_config, postgres_drivers::TableType::POINT);
    postgres_drivers::Columns untagged_nodes_columns(config.m_driver_config, postgres_drivers::TableType::UNTAGGED_POINT);
    postgres_drivers::Columns way_linear_columns(config.m_driver_config, postgres_drivers::TableType::WAYS_LINEAR);
    postgres_drivers::Columns relation_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_OTHER);
    postgres_drivers::Columns node_ways_columns(config.m_driver_config, postgres_drivers::TableType::NODE_WAYS);
    postgres_drivers::Columns node_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_NODES);
    postgres_drivers::Columns way_relations_columns(config.m_driver_config, postgres_drivers::TableType::RELATION_MEMBER_WAYS);
    PostgresTable nodes_table ("nodes", config, node_columns);
    PostgresTable untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    PostgresTable ways_table ("ways", config, way_linear_columns);
    PostgresTable relations_table("relations", config, relation_columns);
    PostgresTable node_ways_table("node_ways", config, node_ways_columns);
    node_ways_table.init();
    PostgresTable node_relations_table("node_relations", config, node_relations_columns);
    node_relations_table.init();
    PostgresTable way_relations_table("way_relations", config, way_relations_columns);
    way_relations_table.init();

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

    std::unique_ptr<sparse_mmap_array_t> index {new sparse_mmap_array_t()};
    index->set(node1.id(), node1.location());
    index->set(node2.id(), node2.location());
    index->set(node3.id(), node3.location());

    std::unique_ptr<UpdateLocationHandler> location_handler = make_handler<sparse_mmap_array_t>(nodes_table, untagged_nodes_table,
            std::move(index));
    DiffHandler2 handler(nodes_table, &untagged_nodes_table, ways_table, relations_table, node_ways_table,
            node_relations_table, way_relations_table, config, expire_tiles, *location_handler);
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
        REQUIRE(column_count == 7);
    }
}

