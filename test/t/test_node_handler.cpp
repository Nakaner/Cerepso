/*
 * test_node_handler.cpp
 *
 *  Created on: 13.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"
#include <postgres_table.hpp>
#include <postgres_drivers/columns.hpp>
#include "../../src/import_handler.hpp"

int count_occurence_of_substring(std::string& string, char to_find) {
    int count = 0;
    for (auto c : string) {
        if (c == to_find) {
            count++;
        }
    }
    return count;
}

TEST_CASE("node handler produces good lines for COPY") {
    static constexpr int buffer_size = 10 * 1000 * 1000;
    osmium::memory::Buffer node_buffer(buffer_size);

    std::map<std::string, std::string> tags;
    tags.insert(std::pair<std::string, std::string>("amenity", "restaurant"));
    tags.insert(std::pair<std::string, std::string>("name", "Gasthof Hirsch"));

    osmium::Node& node = test_utils::create_new_node(node_buffer, 1, 9.1, 49.1, tags);

    //TODO clean up by providing a simpler constructor of MyHandler
    CerepsoConfig config;
    postgres_drivers::Columns node_columns(config.m_driver_config, postgres_drivers::TableType::POINT);
    postgres_drivers::Columns untagged_nodes_columns(config.m_driver_config, postgres_drivers::TableType::UNTAGGED_POINT);
    postgres_drivers::Columns way_columns(config.m_driver_config, postgres_drivers::TableType::WAYS_LINEAR);
    PostgresTable nodes_table ("nodes", config, node_columns);
    PostgresTable untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    PostgresTable ways_table ("ways", config, way_columns);
    ImportHandler handler(nodes_table, untagged_nodes_table, ways_table, config);

    std::string query_str;
    handler.prepare_node_query(node, query_str);

    SECTION("check number of column separators") {
        REQUIRE(count_occurence_of_substring(query_str, '\t') == node_columns.size()-1);
    }

    SECTION("check if query ends with \\n") {
        REQUIRE(query_str[query_str.size()-1] == '\n');
    }

}
