/*
 * myhandler.cpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#include "myhandler.hpp"
#include <osmium/osm/tag.hpp>
#include <sstream>

void MyHandler::node(const osmium::Node& node) {
    if (!node.location().valid()) {
        return;
    }
    if (node.tags().size() == 0) { //no tags, usually a node of way
        prepare_node_query(node, untagged_nodes_copy_buffer);
        if (untagged_nodes_copy_buffer.size() > 1024) {
            m_untagged_nodes_table.send_line(untagged_nodes_copy_buffer);
            untagged_nodes_copy_buffer.clear();
        }
    } else {
        prepare_node_query(node, nodes_copy_buffer);
        if (nodes_copy_buffer.size() > 1024) {
            m_nodes_table.send_line(nodes_copy_buffer);
            nodes_copy_buffer.clear();
        }
    }
}

void MyHandler::way(const osmium::Way& way) {
    if (way.id() == 106273106) {
        std::cout << "test" << std::endl;
        std::cout << ways_copy_buffer << std::endl;
    }
    if (way.nodes().size() < 3) {
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
    query.push_back('\n');
    m_ways_linear_table.send_line(query);
}
