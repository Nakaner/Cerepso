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
    std::string query;
    prepare_node_query(node, query);
    if (node.tags().size() == 0) { //no tags, usually a node of way
        m_untagged_nodes_table.send_line(query);
    } else {
        m_nodes_table.send_line(query);
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
        std::string query;
        static char idbuffer[20];
        sprintf(idbuffer, "%ld", way.id());
        query.append(idbuffer, strlen(idbuffer));
        add_tags(query, way);
        add_metadata_to_stringstream(query, way);
        query.append("SRID=4326;");
        query.append(wkb_factory.create_linestring(way));
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
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void MyHandler::area(const osmium::Area& area) {
    std::string query;
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", area.orig_id());
    query.append(idbuffer);
    add_tags(query, area);
    add_metadata_to_stringstream(query, area);
    query.append("SRID=4326;");
    query.append(wkb_factory.create_multipolygon(area));
    try {
        if (area.from_way()) {
            add_separator_to_stringstream(query);
            query.push_back('{');
            osmium::memory::ItemIteratorRange<const osmium::OuterRing>::iterator nodes = area.outer_rings().begin();
            for (osmium::NodeRefList::const_iterator i = nodes->begin(); i < nodes->end(); i++) {
                if (i != nodes->begin()) {
                    query.append(", ");
                }
                sprintf(idbuffer, "%ld", i->ref());
                query.append(idbuffer);
            }
            query.push_back('}');
            query.push_back('\n');
            m_ways_polygon_table.send_line(query);
        } else {
            query.push_back('\n');
            assert(true); // We will not reach this branch here.
        }
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}
