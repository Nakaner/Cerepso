/*
 * myhandler.cpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#include "myhandler.hpp"
#include <osmium/osm/tag.hpp>
#include <sstream>

void MyHandler::add_tags(std::stringstream& query, const osmium::OSMObject& object) {
    add_separator_to_stringstream(query);
    bool first_tag = true;
    for (const osmium::Tag& tag : object.tags()) {
        if (!first_tag) {
            query.put(',');
        }
        Table::escape4hstore(tag.key(), query);
        query << "=>";
        Table::escape4hstore(tag.value(), query);
        first_tag = false;
    }
}

void MyHandler::node(const osmium::Node& node) {
    if (!node.location().valid()) {
        return;
    }
    std::stringstream query;
    query << node.id();
    if (node.tags().size() > 0) {
        // If the node has tags, it will be written to nodes, not untagged_nodes table.
        add_tags(query, node);
    }
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
    try {
        std::stringstream query;
        query << way.id();
        add_tags(query, way);
        add_metadata_to_stringstream(query, way);
        query << "SRID=4326;" << wkb_factory.create_linestring(way);
        add_separator_to_stringstream(query);
        query << "{";
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            if (i != way.nodes().begin()) {
                query << ", ";
            }
            query << i->ref();
        }
        query << "}";
        query << '\n';
        m_ways_linear_table.send_line(query.str());
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void MyHandler::area(const osmium::Area& area) {
    std::stringstream query;
    query << area.orig_id();
    add_tags(query, area);
    add_metadata_to_stringstream(query, area);
    query << "SRID=4326;" << wkb_factory.create_multipolygon(area);
    try {
        if (area.from_way()) {
            add_separator_to_stringstream(query);
            query << "{";
            osmium::memory::ItemIteratorRange<const osmium::OuterRing>::iterator nodes = area.outer_rings().begin();
            for (osmium::NodeRefList::const_iterator i = nodes->begin(); i < nodes->end(); i++) {
                if (i != nodes->begin()) {
                    query << ", ";
                }
                query << i->ref();
            }
            query << "}";
            query << '\n';
            m_ways_polygon_table.send_line(query.str());
        } else {
            query << '\n';
            m_relations_polygon_table.send_line(query.str());
        }
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
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
