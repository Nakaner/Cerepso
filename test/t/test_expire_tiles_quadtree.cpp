/*
 * test_expire_tiles_quadtree.cpp
 *
 *  Created on: 09.09.2016
 *      Author: michael
 */

#include <fstream>
#include <sys/stat.h>
#include "catch.hpp"
#include <expire_tiles_quadtree.hpp>
#include <columns.hpp>

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
    for (int i=0; i < vec1.size(); i++) {
        if (vec1.at(i) != vec2.at(i)) {
            return false;
        }
    }
    if (vec1.size() != vec2.size()) {
        return false;
    }
    return true;
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

TEST_CASE("Expire Tiles Quadtree") {
    Config config;
    config.m_expire_tiles = "/tmp/etq-test.list";
    if (file_exists(config.m_expire_tiles)) {
        // This test should only run if it can create the file if it does not exist already because we will delete it at the end.
        std::cerr << "ERROR: File " << config.m_expire_tiles << " alread exists, stopping this test.";
        exit(1);
    }

    SECTION("zoom level 5 only") {
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
    }

    SECTION("zoom level 5 and 6") {
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
    }

    SECTION("zoom level 5 to 7") {
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
    }

    // cleanup: delete /tmp/etq-test.list
    if( remove(config.m_expire_tiles.c_str()) != 0 ) {
        std::cerr << "Error deleting file " << config.m_expire_tiles << std::endl;
    }

}


