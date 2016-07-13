/*
 * test_hstore_escape.cpp
 *
 *  Created on: 28.06.2016
 *      Author: michael
 */

#include "catch.hpp"
#include <table.hpp>


TEST_CASE( "escape4hstore()") {

    std::stringstream destination_str;

    SECTION("convert normal string") {
        Table::escape4hstore("highway", destination_str);
        REQUIRE(destination_str.str().compare("\"highway\"") == 0);
    }

    SECTION("convert quotation mark") {
        Table::escape4hstore("disused:rai\"lway", destination_str);
        REQUIRE(destination_str.str().compare("\"disused:rai\\\\\"lway\"") == 0);
    }

    SECTION("convert colon and tab") {
        Table::escape4hstore("disused:rail\tway", destination_str);
        REQUIRE(destination_str.str().compare("\"disused:rail\\\tway\"") == 0);
    }

    SECTION("convert backslash") {
        Table::escape4hstore("this_\\begin", destination_str);
        REQUIRE(destination_str.str().compare("\"this_\\\\\\\\begin\"") == 0);
    }
}
