/*
 * test_diff_handler.cpp
 *
 *  Created on: 28.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <diff_handler1.hpp>
#include <table.hpp>
#include <columns.hpp>

TEST_CASE("inserting new way works") {
    static constexpr int buffer_size = 10 * 1000 * 1000;
    osmium::memory::Buffer g_way_buffer(buffer_size);

    osmium::object_id_type current_id = 1;
    osmium::builder::WayBuilder way_builder(g_way_buffer);
    static_cast<osmium::Way&>(way_builder.object()).set_id(current_id++);
    const osmium::NodeRef nd_ref1 (1, osmium::Location(9.0, 50.1));
    const osmium::NodeRef nd_ref2 (1, osmium::Location(9.1, 50.0));
    const osmium::NodeRef nd_ref3 (1, osmium::Location(9.2, 49.8));
    const std::initializer_list<osmium::NodeRef> node_refs = {nd_ref1, nd_ref2, nd_ref3};
    std::pair<const char*, const char*> mytag ("highway", "trunk");
    const std::initializer_list<std::pair<const char*, const char*>> tags = {mytag};
    way_builder.add_node_refs(node_refs);
    way_builder.add_tags(tags);
    way_builder.add_user("foo");
    osmium::Way& way = static_cast<osmium::Way&>(way_builder.object());
    way.set_version("2");
    way.set_changeset("820");
    way.set_uid("1");
    osmium::Timestamp timestamp ("2016-01-01T00:00:00Z");
    way.set_timestamp(timestamp);
    osmium::builder::TagListBuilder tl_builder(g_way_buffer, &way_builder);

    //TODO clean up by providing a simpler constructor of MyHandler
    Config config;
    Columns node_columns(config, TableType::POINT);
    Columns untagged_nodes_columns(config, TableType::UNTAGGED_POINT);
    Columns way_linear_columns(config, TableType::WAYS_LINEAR);
    Columns relations_columns(config, TableType::RELATION_OTHER);
    DiffHandler1 handler( node_columns, untagged_nodes_columns, way_linear_columns, relations_columns, config);

    std::string ways_table_copy_buffer;
    handler.insert_way(way, ways_table_copy_buffer);

    SECTION("check if last char is \\n") {
//        std::cout << handler.m_ways_table_copy_buffer << std::endl;
        REQUIRE(ways_table_copy_buffer.back() == '\n');
    }
}


