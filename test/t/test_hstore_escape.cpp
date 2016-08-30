/*
 * test_hstore_escape.cpp
 *
 *  Created on: 28.06.2016
 *      Author: michael
 */

#include "catch.hpp"
#include <table.hpp>


TEST_CASE( "escape4hstore(), escape()") {

    std::string destination_str;

    SECTION("escaping normal string for hstore columns") {
        Table::escape4hstore("highway", destination_str);
        REQUIRE(destination_str.compare("\"highway\"") == 0);
    }

    SECTION("escaping quotation mark for hstore columns") {
        Table::escape4hstore("disused:rai\"lway", destination_str);
        REQUIRE(destination_str.compare("\"disused:rai\\\\\"lway\"") == 0);
    }

    SECTION("escaping colon and tab for hstore columns") {
        Table::escape4hstore("disused:rail\tway", destination_str);
        REQUIRE(destination_str.compare("\"disused:rail\\\tway\"") == 0);
    }

    SECTION("escaping colon and tab for non-hstore columns") {
        Table::escape("disused:rail\tway", destination_str);
        REQUIRE(destination_str.compare("disused:rail\\\tway") == 0);
    }

    SECTION("escaping backslash for hstore columns") {
        Table::escape4hstore("this_\\begin", destination_str);
        REQUIRE(destination_str.compare("\"this_\\\\\\\\begin\"") == 0);
    }

    SECTION("escaping backslash for non-hstore columns") {
        Table::escape("this_\\begin\\", destination_str);
        REQUIRE(destination_str.compare("this_\\\\begin\\\\") == 0);
    }
}
