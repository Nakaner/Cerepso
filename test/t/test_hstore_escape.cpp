/*
 * test_hstore_escape.cpp
 *
 *  Created on: 28.06.2016
 *      Author: michael
 */

#include "catch.hpp"
#include <postgres_table.hpp>


TEST_CASE( "escape4hstore(), escape()") {

    std::string destination_str;

    SECTION("escaping normal string for hstore columns") {
        PostgresTable::escape4hstore("highway", destination_str);
        REQUIRE(destination_str.compare("\"highway\"") == 0);
    }

    SECTION("escaping quotation mark for hstore columns") {
        PostgresTable::escape4hstore("disused:rai\"lway", destination_str);
        REQUIRE(destination_str.compare("\"disused:rai\\\\\"lway\"") == 0);
    }

    SECTION("escaping colon and tab for hstore columns") {
        PostgresTable::escape4hstore("disused:rail\tway", destination_str);
        REQUIRE(destination_str.compare("\"disused:rail\\\tway\"") == 0);
    }

    SECTION("escaping colon and tab for non-hstore columns") {
        PostgresTable::escape("disused:rail\tway", destination_str);
        REQUIRE(destination_str.compare("disused:rail\\\tway") == 0);
    }

    SECTION("escaping backslash for hstore columns") {
        PostgresTable::escape4hstore("this_\\begin", destination_str);
        REQUIRE(destination_str.compare("\"this_\\\\\\\\begin\"") == 0);
    }

    SECTION("escaping backslash for non-hstore columns") {
        PostgresTable::escape("this_\\begin\\", destination_str);
        REQUIRE(destination_str.compare("this_\\\\begin\\\\") == 0);
    }
}
