/*
 * addr_interpolation_handler.cpp
 *
 *  Created on:  2018-09-03
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "addr_interpolation_handler.hpp"
#include <osmium/geom/haversine.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/osm/node.hpp>

AddrInterpolationHandler::SecondPassHandler::SecondPassHandler(AddrInterpolationHandler& interpolation_handler) noexcept :
    m_interpolation_handler(interpolation_handler) {
}

void AddrInterpolationHandler::SecondPassHandler::node(const osmium::Node& node) {
    m_interpolation_handler.handle_node(node);
}

void AddrInterpolationHandler::SecondPassHandler::way(const osmium::Way& way) {
    m_interpolation_handler.handle_way(way);
}

void AddrInterpolationHandler::handle_node(const osmium::Node& node) {
    if (!m_required_nodes.node_needed(node.id())) {
        return;
    }
    m_tags_storage.add(node.id(), node.tags());
}

void AddrInterpolationHandler::handle_way(const osmium::Way& way) {
    // check if valid
    if (!valid_interpolation(way.get_value_by_key("addr:interpolation"))) {
        return;
    }
    std::string query;
    try {
        interpolate(way, query);
        if (query.size() > 0) {
            m_table.send_line(query);
        }
    } catch (std::runtime_error& err) {
        std::cerr << err.what() << '\n';
    }
}

void AddrInterpolationHandler::interpolate(const osmium::Way& way, std::string& query) {
    const char* interpolation = way.get_value_by_key("addr:interpolation", "");
    InterpolationType type;
    if (!strcmp(interpolation, "odd")) {
        type = InterpolationType::ODD;
    } else if (!strcmp(interpolation, "even")) {
        type = InterpolationType::EVEN;
    } else if (!strcmp(interpolation, "all")) {
        type = InterpolationType::ALL;
    } else if (!strcmp(interpolation, "alphabetic")) {
        type = InterpolationType::ALPHABETIC;
    } else {
        std::cerr << "Unsupported interpolation type '" << interpolation << "'\n";
        return;
    }
    NodeConstIterator it1 = way.nodes().cbegin();
    const osmium::TagList* tags1 = m_tags_storage.get(way.nodes().front().ref());
    NodeConstIterator it2 = it1 + 1;
    const osmium::TagList* tags2 = m_tags_storage.get(it2->ref());

    // find next node with addr:housenumber
    if (!tags2 || !tags2->has_key("addr:housenumber")) {
        while (it2 + 1 != way.nodes().cend() && (!tags2 || !tags2->has_key("addr:housenumber"))) {
            ++it2;
            tags2 = m_tags_storage.get(it2->ref());
        }
        if (it2 == way.nodes().cend()) {
            throw std::runtime_error{"invalid interpolation, failed to find next point with house number"};
        }
    }

    add_points_to_str(query, it1, it2, tags1, tags2, way, type);
    it1 = it2;
    tags1 = tags2;
    tags2 = nullptr;
    ++it2;
    while (true) {
        while (it2 != way.nodes().cend() && (!tags2 || !(tags2->has_key("addr:housenumber")))) {
            tags2 = m_tags_storage.get(it2->ref());
            ++it2;
        }
        if (!tags2) {
            // last node of the way has not house number or we have reached the end
            return;
        }
        --it2;

        add_points_to_str(query, it1, it2, tags1, tags2, way, type);
        it1 = it2;
        tags1 = tags2;
        tags2 = nullptr;
        ++it2;
    }
}

void AddrInterpolationHandler::add_points_to_str(std::string& query, NodeConstIterator it1, NodeConstIterator it2,
        const osmium::TagList* tags1, const osmium::TagList* tags2, const osmium::Way& way, InterpolationType type) {
    // get direction
    HouseNumber hn1 = HouseNumber::parse_number(tags1->get_value_by_key("addr:housenumber"));
    HouseNumber hn2 = HouseNumber::parse_number(tags2->get_value_by_key("addr:housenumber"));
    if (!hn1.valid() || !hn2.valid()) {
        std::cerr << "One house number is not valid\n";
        return;
    }
    int diff = ::abs(hn2.interpolation_difference(type, hn1));
    if ((type == InterpolationType::ODD || type == InterpolationType::EVEN) && diff % 2 != 0) {
        throw std::runtime_error{"Odd or even interpolation with odd difference"};
    }
    int inc = 1;
    if (type == InterpolationType::ODD || type == InterpolationType::EVEN) {
        inc = 2;
    }
    for (int i = inc; i < diff; i += inc) {
        osmium::Location remote_loc = get_remote_point(it1, it2, i / static_cast<double>(diff));
        HouseNumber new_hn = hn1;
        if (type == InterpolationType::ALPHABETIC) {
            new_hn.increment_alphabetic(i);
        } else {
            new_hn.increment_numeric(i);
        }
        for (auto it = m_table.get_columns().begin(); it != m_table.get_columns().end(); it++) {
            if (it != m_table.get_columns().begin()) {
                PostgresTable::add_separator_to_stringstream(query);
            }
            if (it->column_class() == postgres_drivers::ColumnClass::OSM_ID) {
                static char idbuffer[20];
                sprintf(idbuffer, "%ld", way.id());
                query.append(idbuffer);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_HOUSENUMBER) {
                query.append(std::to_string(new_hn.number));
                if (new_hn.suffix != '\0') {
                    query.push_back(new_hn.suffix);
                }
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_STREET) {
                PostgresTable::escape(tags1->get_value_by_key("addr:street"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_PLACE) {
                PostgresTable::escape(tags1->get_value_by_key("addr:place"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_SUBURB) {
                PostgresTable::escape(tags1->get_value_by_key("addr:suburb"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_POSTCODE) {
                PostgresTable::escape(tags1->get_value_by_key("addr:postcode"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_CITY) {
                PostgresTable::escape(tags1->get_value_by_key("addr:city"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_PROVINCE) {
                PostgresTable::escape(tags1->get_value_by_key("addr:province"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_STATE) {
                PostgresTable::escape(tags1->get_value_by_key("addr:state"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::INTERPOLATION_COUNTRY) {
                PostgresTable::escape(tags1->get_value_by_key("addr:country"), query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::GEOMETRY) {
                // If creating a geometry fails, we have to use an empty multipolygon.
                std::string wkb = "010400000000000000";
                try {
                    wkb = m_table.wkb_factory().create_point(remote_loc);
                } catch (osmium::geometry_error& e) {
                    std::cerr << e.what() << "\n";
                }
                query.append("SRID=4326;");
                query.append(wkb);
            } else {
                query.append("\\N");
            }
        }
        query.push_back('\n');
    }
}

osmium::Location AddrInterpolationHandler::get_remote_point(NodeConstIterator it1, NodeConstIterator it2, double fraction) {
    assert(fraction > 0.0 && fraction < 1.0);
    double total_length = 0.0;
    for (auto it = it1; it < it2; ++it) {
        total_length += osmium::geom::haversine::distance(it->location(), (it + 1)->location());
    }
    double interpol_length = total_length * fraction;
    // go along the line until interpol_length is reached
    double traversed_length = 0;
    double last_lenght = 0;
    NodeConstIterator current = it1;
    NodeConstIterator last_vertex = current;
    double whole_segment;
    do {
        last_lenght = traversed_length;
        whole_segment = osmium::geom::haversine::distance(current->location(), (current + 1)->location());
        traversed_length += whole_segment;
        last_vertex = current;
        current = last_vertex + 1;
    } while (traversed_length < interpol_length);
    double remaining = interpol_length - last_lenght;
    // transform the two neighbouring vertexes into EPSG:3857
    osmium::geom::Coordinates c1 = osmium::geom::lonlat_to_mercator(last_vertex->location());
    osmium::geom::Coordinates c2 = osmium::geom::lonlat_to_mercator(current->location());
    // Due to the intercept theorem, it is sufficient to look at one of the two axis only.
    // Check which axis is nearer (based on angle) to avoid division by zero or very large numerators.
    double dy = c2.y - c1.y;
    double dx = c2.x - c1.x;
    const osmium::geom::Coordinates result_merc {c1.x + (remaining / whole_segment) * dx, c1.y + (remaining / whole_segment) * dy};
    osmium::geom::Coordinates result = osmium::geom::mercator_to_lonlat(result_merc);
    return osmium::Location{result.x, result.y};
}


HouseNumber::HouseNumber() :
    number(0),
    suffix('\0') {

}

HouseNumber::HouseNumber(int base, char alpha) :
    number(base),
    suffix(alpha) {
}

bool HouseNumber::valid() const {
    return number > 0;
}

int HouseNumber::interpolation_difference(InterpolationType type, HouseNumber& other) const noexcept {
    if (type == InterpolationType::ALPHABETIC) {
        // make suffix case insensitive
        return ::tolower(suffix) - ::tolower(other.suffix);
    }
    return number - other.number;
}

/*static*/ HouseNumber HouseNumber::parse_number(const char* start) {
    if (!start) {
        return HouseNumber{};
    }
    char* end_ptr;
    int number = std::strtol(start, &end_ptr, 10);
    if (end_ptr == start) {
        return HouseNumber{};
    }
    if (number <= 0) {
        throw std::runtime_error{"Failed to parse house number. Numeric part is invalid or negative."};
    }
    if (std::isspace(*end_ptr)) {
        ++end_ptr;
    }
    if (*end_ptr == '\0') {
        return HouseNumber{number, '\0'};
    }
    if (!std::isalpha(*end_ptr)) {
        throw std::runtime_error{"Failed to parse house number. Non-alphabetic character after number."};
    }
    char suffix = *end_ptr;
    ++end_ptr;
    if (*end_ptr != '\0') {
        throw std::runtime_error{"Failed to parse house number. More than one character after number."};
    }
    return HouseNumber{number, suffix};
}

void HouseNumber::increment_numeric(const int increment) {
    number = number + increment;
}

void HouseNumber::increment_alphabetic(const int increment) {
    int new_suffix = suffix + increment;
    if (new_suffix < 0x41 || (new_suffix > 0x5a && new_suffix < 0x61) || new_suffix > 0x7a) {
        throw std::runtime_error{"Alphabetic increment beyond range [A-Za-z]"};
    }
    suffix = static_cast<char>(new_suffix);
}

AddrInterpolationHandler::AddrInterpolationHandler(PostgresTable& interpolated_nodes_table) :
    m_required_nodes(),
    m_handler_pass2(*this),
    m_table(interpolated_nodes_table) {
}

RequiredNodesTracking::RequiredNodesTracking() :
    m_vector() {
    m_last_node_iter = m_vector.begin();
}


/*static*/ bool AddrInterpolationHandler::valid_interpolation(const char* value) {
    if (!value) {
        return false;
    }
    return !(strcmp(value, "odd") && strcmp(value, "even") && strcmp(value, "all") && strcmp(value, "alphabetic"));
}

void AddrInterpolationHandler::way(const osmium::Way& way) {
    // check if valid
    if (!valid_interpolation(way.get_value_by_key("addr:interpolation"))) {
        return;
    }
    if (way.nodes().size() == 1 || way.nodes().ends_have_same_id()) {
        return;
    }
    for (const auto& nr : way.nodes()) {
        m_required_nodes.track(nr.ref());
    }
}

void RequiredNodesTracking::track(osmium::object_id_type id) {
    m_vector.push_back(id);
}

void RequiredNodesTracking::prepare_for_lookup() {
    std::sort(m_vector.begin(), m_vector.end());
    std::unique(m_vector.begin(), m_vector.end());
    m_last_node_iter = m_vector.begin();
}

void AddrInterpolationHandler::after_pass1() {
    m_required_nodes.prepare_for_lookup();
}

bool RequiredNodesTracking::node_needed(const osmium::object_id_type id) {
    if (m_last_node_iter == m_vector.end()) {
        return false;
    }
    std::vector<osmium::object_id_type>::iterator it = m_last_node_iter;
    if (id == *it) {
        m_last_node_iter = it + 1;
        return true;
    }
    // Try to go forward.
    while (it != m_vector.end() && id > *it) {
        ++it;
        if (id == *it) {
            m_last_node_iter = it + 1;
            return true;
        }
    }
    it = m_last_node_iter;

    // If going forward did not have an effect, try to go backward.
    while (it != m_vector.begin() && id < *it) {
        ++it;
        if (it != m_vector.end() && id == *it) {
            m_last_node_iter = it + 1;
            return true;
        }
    }
    return false;
}

AddrInterpolationHandler::SecondPassHandler& AddrInterpolationHandler::handler() {
    return m_handler_pass2;
}
