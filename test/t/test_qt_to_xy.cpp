/*
 * test_qt_to_xy.cpp
 *
 *
 *  Created on: 08.09.2016
 *      Author: michael
 */
#include "catch.hpp"
#include <expire_tiles_quadtree.hpp>
#include <columns.hpp>

TEST_CASE("convert quadtree IDs to XY coordinates") {
    Config config;
    ExpireTilesQuadtree etq(config);
    SECTION("zoom level 0") {
        int qt = 0b0;
        xy_coord_t xy = etq.quadtree_to_xy(qt, 0);
        REQUIRE((xy.x == 0 && xy.y == 0));
    }
    SECTION("zoom level 1") {
        int qt = 0b01;
        xy_coord_t xy = etq.quadtree_to_xy(qt, 1);
        REQUIRE((xy.x == 1 && xy.y == 0));
    }
    SECTION("zoom level 2") {
        int qt = 0b1010;
        xy_coord_t xy = etq.quadtree_to_xy(qt, 2);
        REQUIRE((xy.x == 0 && xy.y == 3));
    }
    SECTION("zoom level 3") {
        int qt = 0b100111;
        xy_coord_t xy = etq.quadtree_to_xy(qt, 3);
        REQUIRE((xy.x == 3 && xy.y == 5));
    }
}


