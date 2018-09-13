/*
 * test_addr_interpolation_handler.cpp
 *
 *  Created on:  2018-09-06
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "catch.hpp"
#include <osmium/builder/attr.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/osm/node.hpp>
#include <addr_interpolation_handler.hpp>

using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

std::string get_copy_data(osmium::memory::Buffer& buffer) {
    CerepsoConfig config;
    postgres_drivers::Columns interpolation_columns(config.m_driver_config,
            postgres_drivers::Columns::addr_interpolation_columns());
    PostgresTable interpolated_table {"interpolated_addresses", config,
        std::move(interpolation_columns)};
    AddrInterpolationHandler handler {interpolated_table};

    index_type location_index;
    osmium::handler::NodeLocationsForWays<index_type> location_handler(location_index);

    // first pass
    for (const auto& item : buffer) {
        if (item.type() == osmium::item_type::way) {
            handler.way(static_cast<const osmium::Way&>(item));
        }
    }
    handler.after_pass1();
    std::string query;
    // second pass
    for (const auto& item : buffer) {
        if (item.type() == osmium::item_type::node) {
            location_handler.node(static_cast<const osmium::Node&>(item));
            handler.handler().node(static_cast<const osmium::Node&>(item));
        } else if (item.type() == osmium::item_type::way) {
            location_handler.way(const_cast<osmium::Way&>(static_cast<const osmium::Way&>(item)));
            handler.interpolate(static_cast<const osmium::Way&>(item), query);
        }
    }
    return query;
}

TEST_CASE("Test tracking of required nodes") {
    RequiredNodesTracking tracking;
    std::vector<osmium::object_id_type> expected {1, 2, 3, 4, 7, 9, 10, 16,  20, 23, 24, 25, 26};
    for (auto id : expected) {
        tracking.track(id);
    }
    tracking.prepare_for_lookup();
    // check tracked nodes in correct order
    const auto max_id = std::max_element(expected.begin(), expected.end());

    SECTION("Query all nodes in the vector") {
        for (osmium::object_id_type id = 0; id <= *max_id; ++id) {
            bool should_find = (std::find(expected.begin(), expected.end(), id) != expected.end());
            REQUIRE(tracking.node_needed(id) == should_find);
        }
    }

    tracking.prepare_for_lookup();

    SECTION("Query some nodes in the vector only") {
        for (osmium::object_id_type id = 0; id <= *max_id; id += 4) {
            bool should_find = (std::find(expected.begin(), expected.end(), id) != expected.end());
            REQUIRE(tracking.node_needed(id) == should_find);
        }
    }
}

TEST_CASE("Valid addr:interpolation values") {
    CHECK(AddrInterpolationHandler::valid_interpolation("odd"));
    CHECK(AddrInterpolationHandler::valid_interpolation("even"));
    CHECK(AddrInterpolationHandler::valid_interpolation("alphabetic"));
    CHECK(AddrInterpolationHandler::valid_interpolation("all"));
    CHECK_FALSE(AddrInterpolationHandler::valid_interpolation("every_second"));
}

TEST_CASE("Parse house numbers and check validity") {
    CHECK((HouseNumber::parse_number("15")).number == 15);
    HouseNumber hn = HouseNumber::parse_number("15a");
    CHECK(hn.number == 15);
    CHECK(hn.suffix == 'a');
    CHECK(hn.valid());
    hn = HouseNumber::parse_number("15 a");
    CHECK(hn.number == 15);
    CHECK(hn.suffix == 'a');
    CHECK(hn.valid());
    hn = HouseNumber::parse_number("  15 a");
    CHECK(hn.number == 15);
    CHECK(hn.suffix == 'a');
    CHECK(hn.valid());
    hn = HouseNumber::parse_number("15A");
    CHECK(hn.number == 15);
    CHECK(hn.suffix == 'A');
    CHECK(hn.valid());
    CHECK_THROWS_AS(HouseNumber::parse_number("15aa"), std::runtime_error);
    CHECK_THROWS_AS(HouseNumber::parse_number("15  a"), std::runtime_error);
    CHECK_THROWS_AS(HouseNumber::parse_number("15/1"), std::runtime_error);
    CHECK_THROWS_AS(HouseNumber::parse_number("15-19"), std::runtime_error);
    CHECK_FALSE((HouseNumber::parse_number("Aa")).valid());
    CHECK_FALSE((HouseNumber::parse_number("a")).valid());
    CHECK_THROWS_AS(HouseNumber::parse_number("-45a"), std::runtime_error);
    CHECK_THROWS_AS(HouseNumber::parse_number("-45"), std::runtime_error);
    CHECK_THROWS_AS(HouseNumber::parse_number(" -45"), std::runtime_error);
}

TEST_CASE("Interpolation difference") {
    HouseNumber hn1 = HouseNumber::parse_number("15");
    HouseNumber hn2 = HouseNumber::parse_number("25");
    CHECK(hn1.interpolation_difference(InterpolationType::ODD, hn2) == -10);
    CHECK(hn2.interpolation_difference(InterpolationType::ODD, hn1) == 10);
    CHECK(hn1.interpolation_difference(InterpolationType::EVEN, hn2) == -10);
    CHECK(hn2.interpolation_difference(InterpolationType::EVEN, hn1) == 10);
    CHECK(hn1.interpolation_difference(InterpolationType::ALL, hn2) == -10);
    CHECK(hn2.interpolation_difference(InterpolationType::ALL, hn1) == 10);
    hn1 = HouseNumber::parse_number("15a");
    hn2 = HouseNumber::parse_number("15f");
    CHECK(hn1.interpolation_difference(InterpolationType::ALPHABETIC, hn2) == -5);
    CHECK(hn2.interpolation_difference(InterpolationType::ALPHABETIC, hn1) == 5);
    // check automatic conversion to lowercase
    hn2 = HouseNumber::parse_number("15F");
    CHECK(hn1.interpolation_difference(InterpolationType::ALPHABETIC, hn2) == -5);
    CHECK(hn2.interpolation_difference(InterpolationType::ALPHABETIC, hn1) == 5);
}

TEST_CASE("Incrementing house numbers") {
    HouseNumber hn = HouseNumber::parse_number("15");
    hn.increment_numeric(5);
    CHECK(hn.number == 20);
    hn = HouseNumber::parse_number("15");
    hn.increment_numeric(-5);
    CHECK(hn.number == 10);
    hn = HouseNumber::parse_number("15f");
    hn.increment_alphabetic(5);
    CHECK(hn.number == 15);
    CHECK(hn.suffix == 'k');
    hn = HouseNumber::parse_number("15f");
    hn.increment_alphabetic(-5);
    CHECK(hn.number == 15);
    CHECK(hn.suffix == 'a');
    // check end of range
    hn = HouseNumber::parse_number("15b");
    CHECK_THROWS_AS(hn.increment_alphabetic(-2), std::runtime_error);
    CHECK_THROWS_AS(hn.increment_alphabetic(+30), std::runtime_error);
}

#ifndef TEST_DATA_DIR
#error "TEST_DATA_DIR is not defined!"
#endif

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

TEST_CASE("address interpolation, simple example") {
    std::string input_path = STRINGIZE_VALUE_OF(TEST_DATA_DIR);
    input_path += "/address-interpolations/simple.osm";
    osmium::memory::Buffer buffer = osmium::io::read_file(input_path);

    std::string query = get_copy_data(buffer);
    std::string expected = "39097\t15\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000F475CF70B910214022E9899DDF984A40\n" \
        "39097\t17\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000A4E2FF8EA81021401173A48EE9984A40\n" \
        "39097\t19\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000A64D8BB0971021406C3DE87EF3984A40\n" \
        "39097\t21\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;010100000056BABBCE861021405BC70270FD984A40\n" \
        "39097\t25\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;01010000005DF8664062102140C36E8E290E994A40\n" \
        "39097\t27\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;010100000061CB86904E1021403D8CFFF114994A40\n" \
        "39097\t29\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000659EA6E03A10214021EA99B91B994A40\n" \
        "39097\t31\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;01010000006971C630271021409B070B8222994A40\n";
    REQUIRE(query == expected);
}

TEST_CASE("address interpolation, alphabetic") {
    std::string input_path = STRINGIZE_VALUE_OF(TEST_DATA_DIR);
    input_path += "/address-interpolations/alphabetic.osm";
    osmium::memory::Buffer buffer = osmium::io::read_file(input_path);

    std::string query = get_copy_data(buffer);
    std::string expected = "40716\t26b\tRichard-Taylor-Straße\t\\N\t\\N\t987654\tBremen" \
            "\tIrgendprovinz\tIrgendland\tDE\tSRID=4326;0101000000C12ED4AB1212214052184ADC19994A40\n" \
        "40716\t26c\tRichard-Taylor-Straße\t\\N\t\\N\t987654\tBremen" \
            "\tIrgendprovinz\tIrgendland\tDE\tSRID=4326;0101000000091456CFEE1121402FEC7AB317994A40\n" \
        "40716\t26d\tRichard-Taylor-Straße\t\\N\t\\N\t987654\tBremen" \
            "\tIrgendprovinz\tIrgendland\tDE\tSRID=4326;010100000051F9D7F2CA112140A27F828B15994A40\n";
    REQUIRE(query == expected);
}

TEST_CASE("address interpolation, middle nodes without tags") {
    std::string input_path = STRINGIZE_VALUE_OF(TEST_DATA_DIR);
    input_path += "/address-interpolations/middle-nodes-no-tags.osm";
    osmium::memory::Buffer buffer = osmium::io::read_file(input_path);

    std::string query = get_copy_data(buffer);
    std::string expected = "40669\t3\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;01010000009ECD4F1663112140BEDEFDF15E994A40\n" \
        "40669\t5\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000C36169E047112140654E3C0C52994A40\n" \
        "40669\t7\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;01010000009DF122B8361121402A334A3C45994A40\n" \
        "40669\t9\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000B53E9B0B12112140E8209EDB3A994A40\n" \
        "40669\t11\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000EDD286C3D21021404CF1023631994A40\n" \
        "40669\t13\tFarger Straße\t\\N\t\\N\t987654\tBremen\tIrgendprovinz" \
            "\tIrgendland\tDE\tSRID=4326;0101000000D268177893102140B0C1679027994A40\n";
    REQUIRE(query == expected);
}
