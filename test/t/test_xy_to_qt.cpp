/*
 * test_xy_to_qt.cpp
 *
 *  Created on: 08.09.2016
 *      Author: michael
 */

#include "catch.hpp"
#include <expire_tiles_quadtree.hpp>
#include <postgres_drivers/columns.hpp>

TEST_CASE("convert XY coordinates to quadtree IDs") {
    CerepsoConfig config;
    ExpireTilesQuadtree etq(config);
    SECTION("zoom level 0") {
        int qt = etq.xy_to_quadtree(0, 0, 0);
        REQUIRE(qt == 0b0);
    }
    SECTION("zoom level 1") {
        int qt = etq.xy_to_quadtree(1, 0, 1);
        REQUIRE(qt == 0b01);
    }
    SECTION("zoom level 2") {
        int qt = etq.xy_to_quadtree(0, 3, 2);
        REQUIRE(qt == 0b1010);
    }
    SECTION("zoom level 3") {
        int qt = etq.xy_to_quadtree(3, 5, 3);
        REQUIRE(qt == 0b100111);
    }
}


