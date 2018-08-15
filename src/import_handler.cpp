/*
 * import_handler.cpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#include <osmium/osm/tag.hpp>
#include <sstream>
#include "import_handler.hpp"

void ImportHandler::node(const osmium::Node& node) {
    if (!node.location().valid()) {
        return;
    }
    std::string query;
    if (node.tags().size() > 0) {
        prepare_node_query(node, query);
        m_nodes_table.send_line(query);
    } else if (m_config.m_driver_config.updateable) { //no tags, usually a node of way
        prepare_node_query(node, query);
        m_untagged_nodes_table->send_line(query);
    }
}

void ImportHandler::way(const osmium::Way& way) {
    if (way.nodes().size() < 2) {
        // degenerated way (none or only one node)
        //TODO add logging
        return;
    }
    for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
        if (!i->location().valid()) {
            return; //incomplete way
            //TODO support partial ways
        }
    }
    std::string query;
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", way.id());
    query.append(idbuffer, strlen(idbuffer));
    add_tags(query, way);
    add_metadata_to_stringstream(query, way, m_config);
    std::string wkb = "010200000000000000"; // initalize with LINESTRING EMPTY
    // If creating a linestring fails (e.g. way with four nodes at the same location), we have to use an empty linestring.
    try {
        wkb = wkb_factory.create_linestring(way);
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
    query.append("SRID=4326;");
    query.append(wkb);
    if (m_config.m_driver_config.updateable) {
        add_separator_to_stringstream(query);
        query.append("{");
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            if (i != way.nodes().begin()) {
                query.append(", ");
            }
            sprintf(idbuffer, "%ld", i->ref());
            query.append(idbuffer);
        }
        query.push_back('}');
    }
    query.push_back('\n');
    m_ways_linear_table.send_line(query);
}

void ImportHandler::area(const osmium::Area& area) {
    std::string query;
    static char idbuffer[20];
    if (area.from_way()) {
        sprintf(idbuffer, "%ld", area.orig_id());
    } else {
        sprintf(idbuffer, "%ld", -area.orig_id());
    }
    query.append(idbuffer, strlen(idbuffer));
    add_tags(query, area);
    add_metadata_to_stringstream(query, area, m_config);
    std::string wkb = "0106000020E610000000000000"; // initalize with MULTIPOLYGON EMPTY
    // If creating an area fails, we have to use an empty multipolygon.
    try {
        wkb = wkb_factory.create_multipolygon(area);
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
    query.append("SRID=4326;");
    query.append(wkb);
    query.push_back('\n');
    m_areas_table->send_line(query);
}
