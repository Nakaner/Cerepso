/*
 * append_handler.cpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#include "diff_handler1.hpp"

//DiffHandler1::~DiffHandler1() {
//    m_nodes_table.delete_from_list(m_delete_nodes);
//    m_nodes_table.start_copy();
//    m_nodes_table.send_line(m_nodes_table_copy_buffer);
//    m_nodes_table.end_copy();
//    m_untagged_nodes_table.delete_from_list(m_delete_nodes);
//    m_untagged_nodes_table.start_copy();
//    m_untagged_nodes_table.send_line(m_untagged_nodes_table_copy_buffer);
//    m_untagged_nodes_table.end_copy();
//}

void DiffHandler1::node(const osmium::Node& node) {
    // if node has version 1, we don't have to check if it already exists
    if (node.version() > 1) {
        // delete old node, try first untagged nodes table
        m_nodes_table.delete_object(node.id());
    }
    if (node.deleted()) { // we are finish now
        return;
    }
    if (!node.location().valid()) {
        return;
    }
    if (node.tags().size() > 0) {
        prepare_node_query(node, m_nodes_table_copy_buffer);
    }
    else {
        prepare_node_query(node, m_untagged_nodes_table_copy_buffer);
    }
}


void DiffHandler1::way(const osmium::Way& way) {
    if (way.version() > 1) {
        m_ways_linear_table.delete_object(way.id());
    }
    if (way.deleted()) {
        return;
    }
    try {
        std::string query;
        char idbuffer[20];
        sprintf(idbuffer, "%ld", way.id());
        query.append(idbuffer, strlen(idbuffer));
        add_tags(query, way);
        add_metadata_to_stringstream(query, way);
        //TODO add geometry
        std::vector<geos::geom::Coordinate> way_node_locations;
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            geos::geom::GeometryFactory gf;
            try {
                geos::geom::Coordinate coord = m_ways_linear_table.get_point(i->ref());
            } catch ()

            way_node_locations.push_back();
        }
//        query.append("SRID=4326;");
//        query.append(wkb_factory.create_linestring(way));
        add_separator_to_stringstream(query);
        query.append("{");
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            if (i != way.nodes().begin()) {
                query.append(", ");
            }
            sprintf(idbuffer, "%ld", i->ref());
            query.append(idbuffer, strlen(idbuffer));
        }
        query.append("}");
        query.push_back('\n');
        m_ways_linear_table.send_line(query);
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}


void DiffHandler1::relation(const osmium::Relation& relation) {
    if (relation.version() > 1) {
        m_relations_table.delete_object(relation.id());
    }
}

