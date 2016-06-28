/*
 * myhandler.cpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#include "myhandler.hpp"
#include <sstream>

void MyHandler::node(const osmium::Node& node) {
    if (!node.location().valid()) {
        return;
    }
    std::stringstream query;
    query << node.id();
    add_separator_to_stringstream(query);
    add_metadata_to_stringstream(query, node);
    query << "SRID=4326;" << wkb_factory.create_point(node);
    query << '\n';
    if (node.tags().size() == 0) { //no tags, usually a node of way
        m_untagged_nodes_table.send_line(query.str());
    } else {
        m_nodes_table.send_line(query.str());
    }
}

void MyHandler::way(const osmium::Way& way) {
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
    std::stringstream query;
    query << way.id();
    add_separator_to_stringstream(query);
    add_metadata_to_stringstream(query, way);
    query << "SRID=4326;" << wkb_factory.create_linestring(way);
    query << '\n';
    m_ways_linear_table.send_line(query.str());
}

void MyHandler::area(const osmium::Area& area) {
    std::stringstream query;
    query << area.orig_id();
    add_separator_to_stringstream(query);
    add_metadata_to_stringstream(query, area);
    query << "SRID=4326;" << wkb_factory.create_multipolygon(area);
    query << '\n';
    if (area.from_way()) {
        m_ways_polygon_table.send_line(query.str());
    } else {
        m_relations_polygon_table.send_line(query.str());
    }
}

void MyHandler::add_separator_to_stringstream(std::stringstream& ss) {
    ss << '\t';
}

void MyHandler::add_metadata_to_stringstream(std::stringstream& ss, const osmium::OSMObject& object) {
    add_separator_to_stringstream(ss);
    ss << object.user();
    add_separator_to_stringstream(ss);
    ss << object.uid();
    add_separator_to_stringstream(ss);
    ss << object.version();
    add_separator_to_stringstream(ss);
    ss << object.timestamp().to_iso();
    add_separator_to_stringstream(ss);
    ss << object.changeset();
    add_separator_to_stringstream(ss);
}
