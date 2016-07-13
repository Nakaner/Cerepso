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
    std::stringstream query;
    prepare_node_query(node, query);
    if (node.tags().size() == 0) { //no tags, usually a node of way
        m_untagged_nodes_table.send_line(query.str());
    } else {
        m_nodes_table.send_line(query.str());
    }
}

void MyHandler::prepare_node_query(const osmium::Node& node, std::stringstream& query) {
    query << node.id();
    if (node.tags().size() > 0) {
        // If the node has tags, it will be written to nodes, not untagged_nodes table.
        PostgresHandler::add_tags(query, node);
    }
    PostgresHandler::add_metadata_to_stringstream(query, node);
    query << "SRID=4326;" << wkb_factory.create_point(node);
    query << '\n';
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
