/*
 * test_expire_tiles_quadtree.cpp
 *
 *  Created on: 09.09.2016
 *      Author: michael
 */

#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include "catch.hpp"
#include <expire_tiles_quadtree.hpp>
#include <postgres_drivers/columns.hpp>

using stringvector_t = std::vector<std::string>;

stringvector_t get_file_content(std::string& filename) {
    stringvector_t vec;
    // read from /tmp/etq-test.list
    std::ifstream expire_file ("/tmp/etq-test.list");
    std::string line;
    while (std::getline(expire_file,line)) {
        vec.push_back(line);
    }
    expire_file.close();
    return vec;
}

bool compare_vectors(stringvector_t& vec1, stringvector_t& vec2) {
    int matches = 0;
    for (std::string& str1 : vec1) {
        for (std::string& str2 : vec2) {
            if (str1 == str2) {
                matches++;
                break;
            }
        }
    }
    if (matches == vec1.size()) {
        return true;
    }
    return false;
}

bool file_exists (const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

// for debugging
void print_vectors(stringvector_t& vec) {
    std::cout << "CONTENT" << std::endl;
    for (auto v : vec) {
        std::cout << v << std::endl;
    }
}

void cleanup(std::string& filename) {
}

TEST_CASE("Expire Tiles Quadtree") {
    CerepsoConfig config;
    config.m_expire_tiles = "/tmp/etq-test.list";
    if (file_exists(config.m_expire_tiles)) {
        // This test should only run if it can create the file if it does not exist already because we will delete it at the end.
        std::cerr << "ERROR: File " << config.m_expire_tiles << " already exists, stopping this test.";
        exit(1);
    }

    SECTION("expire point on zoom level 5 only") {
        config.m_min_zoom = 5;
        config.m_max_zoom = 5;
        ExpireTilesQuadtree etq(config);
        // just a single point
        etq.expire_from_point(5.9, 52.1);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("5/16/10");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("expire point on zoom level 5 and 6") {
        config.m_min_zoom = 5;
        config.m_max_zoom = 6;
        ExpireTilesQuadtree etq(config);
        // just a single point
        etq.expire_from_point(5.9, 52.1);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("6/33/21");
        expected.push_back("5/16/10");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("expire point on zoom level 5 to 7") {
        config.m_min_zoom = 5;
        config.m_max_zoom = 7;
        ExpireTilesQuadtree etq(config);
        // just a single point
        etq.expire_from_point(5.9, 52.1);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("7/66/42");
        expected.push_back("6/33/21");
        expected.push_back("5/16/10");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("expire vertical line on zoom level 12 and 13") {
        config.m_min_zoom = 12;
        config.m_max_zoom = 13;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(11.78, 51.61, 11.78, 51.74);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("12/2182/1357");
        expected.push_back("12/2182/1358");
        expected.push_back("12/2182/1359");
        expected.push_back("12/2182/1360");
        expected.push_back("13/4364/2715");
        expected.push_back("13/4364/2716");
        expected.push_back("13/4364/2717");
        expected.push_back("13/4364/2718");
        expected.push_back("13/4364/2719");
        expected.push_back("13/4364/2720");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("horizontal line on zoom level 7") {
        config.m_min_zoom = 7;
        config.m_max_zoom = 7;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(-8.1, 22.2, -2.0, 22.2);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("7/61/55");
        expected.push_back("7/62/55");
        expected.push_back("7/63/55");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("line crossing 180th meridian from eastern to western hemisphere on zoom level 7") {
        config.m_min_zoom = 7;
        config.m_max_zoom = 7;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(173.9, 54.48, -172.67, 50.48);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("7/125/40");
        expected.push_back("7/126/40");
        expected.push_back("7/126/41");
        expected.push_back("7/127/41");
        expected.push_back("7/0/41");
        expected.push_back("7/0/42");
        expected.push_back("7/1/42");
        expected.push_back("7/2/42");
        expected.push_back("7/2/43");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("line crossing 180th meridian from western to eastern hemisphere on zoom level 7") {
        config.m_min_zoom = 7;
        config.m_max_zoom = 7;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(-172.67, 50.48, 173.9, 54.48);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("7/125/40");
        expected.push_back("7/126/40");
        expected.push_back("7/126/41");
        expected.push_back("7/127/41");
        expected.push_back("7/0/41");
        expected.push_back("7/0/42");
        expected.push_back("7/1/42");
        expected.push_back("7/2/42");
        expected.push_back("7/2/43");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
    cleanup(config.m_expire_tiles);
    }

    SECTION("180th meridian on zoom level 6") {
        config.m_min_zoom = 6;
        config.m_max_zoom = 6;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(-180, -16, -180, -18);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("6/0/34");
        expected.push_back("6/0/35");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("180th meridian on zoom level 6 (other variant)") {
        config.m_min_zoom = 6;
        config.m_max_zoom = 6;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(180, -16, -180, -18);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("6/0/34");
        expected.push_back("6/0/35");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("line almost parallel to a meridian on zoom level 14") {
        config.m_min_zoom = 14;
        config.m_max_zoom = 14;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(8.417, 49.313, 8.418, 49.338);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("14/8575/5604");
        expected.push_back("14/8575/5603");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);

    }

    SECTION("line parallel to a meridian on zoom level 14") {
        config.m_min_zoom = 14;
        config.m_max_zoom = 14;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(8.417, 49.313, 8.417, 49.338);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("14/8575/5604");
        expected.push_back("14/8575/5603");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }

    SECTION("any line on zoom level 14") {
        config.m_min_zoom = 14;
        config.m_max_zoom = 14;
        ExpireTilesQuadtree etq(config);
        etq.expire_line_segment_180secure(8.417, 49.313, 8.464, 49.352);
        etq.output_and_destroy();

        // read from /tmp/etq-test.list
        stringvector_t expired_tiles = get_file_content(config.m_expire_tiles);
        stringvector_t expected;
        expected.push_back("14/8575/5604");
        expected.push_back("14/8575/5603");
        expected.push_back("14/8576/5603");
        expected.push_back("14/8576/5602");
        expected.push_back("14/8577/5602");
        REQUIRE(compare_vectors(expired_tiles, expected) == true);
        cleanup(config.m_expire_tiles);
    }
    // cleanup: delete /tmp/etq-test.list
    if( remove(config.m_expire_tiles.c_str()) != 0 ) {
        std::cerr << "Error deleting file " << config.m_expire_tiles << std::endl;
    }

}


