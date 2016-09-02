/*
 * test_node_handler.cpp
 *
 *  Created on: 13.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <myhandler.hpp>
#include <table.hpp>
#include <columns.hpp>

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
    osmium::memory::Buffer g_node_buffer(buffer_size);
    osmium::object_id_type current_id = 1;

    osmium::builder::NodeBuilder node_builder(g_node_buffer);
    static_cast<osmium::Node&>(node_builder.object()).set_id(current_id++);
    osmium::Node& node = static_cast<osmium::Node&>(node_builder.object());
    node.set_version(1);
    node.set_changeset(1);
    node.set_uid(1);
    node.set_timestamp(1);
    node_builder.add_user("foo");
    osmium::Location location (9.0, 49.0);
    node.set_location(location);
    osmium::builder::TagListBuilder tl_builder(g_node_buffer, &node_builder);

    //TODO clean up by providing a simpler constructor of MyHandler
    Config config;
    Columns node_columns(config, TableType::POINT);
    Columns untagged_nodes_columns(config, TableType::UNTAGGED_POINT);
    Columns way_columns(config, TableType::WAYS_LINEAR);
    Table nodes_table ("nodes", config, node_columns);
    Table untagged_nodes_table ("untagged_nodes", config, untagged_nodes_columns);
    Table ways_table ("ways", config, way_columns);
    MyHandler handler(nodes_table, untagged_nodes_table, ways_table, config);

    tl_builder.add_tag("amenity", "restaurant");
    tl_builder.add_tag("name", "Gasthof Hirsch");
    std::string query_str;
    handler.prepare_node_query(node, query_str);

    SECTION("check number of column separators") {
        REQUIRE(count_occurence_of_substring(query_str, '\t') == node_columns.size()-1);
    }

    SECTION("check if query ends with \\n") {
        REQUIRE(query_str[query_str.size()-1] == '\n');
    }

}
