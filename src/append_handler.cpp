/*
 * append_handler.cpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#include "append_handler.hpp"

AppendHandler::~AppendHandler() {
    m_nodes_table.delete_from_list(m_delete_nodes);
    m_nodes_table.start_copy();
    m_nodes_table.send_line(m_nodes_table_copy_buffer);
    m_nodes_table.end_copy();
    m_untagged_nodes_table.delete_from_list(m_delete_nodes);
    m_untagged_nodes_table.start_copy();
    m_untagged_nodes_table.send_line(m_untagged_nodes_table_copy_buffer);
    m_untagged_nodes_table.end_copy();
}

void AppendHandler::node(const osmium::Node& node) {
    // if node has version 1, we don't have to check if it already exists
    if (node.version() > 1) {
        // delete old node, try first untagged nodes table
        m_delete_nodes.push_back(node.id());
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


void AppendHandler::way(const osmium::Way& way) {
//    // if way has version 1, we don't have to check if it already exists
//    if (way.version() > 1) {
//        std::stringstream query;
//        // delete old node, try first untagged nodes table
//        query << "DELETE FROM " << m_ways_linear_table.get_name() << " WHERE osm_id = " << way.id();
//        m_untagged_nodes_table.send_query(query.str().c_str());
//        query << "DELETE FROM " << m_ways_polygon_table.get_name() << " WHERE osm_id = " << way.id();
//        m_nodes_table.send_query(query.str().c_str());
//    }
//    if (way.deleted()) {
//        return;
//    }
//    try {
//        std::stringstream query;
//        m_ways_linear_table_buffer << way.id();
//        add_tags(m_ways_linear_table_buffer, way);
//        add_metadata_to_stringstream(m_ways_linear_table_buffer, way);
//        m_ways_linear_table_buffer << "SRID=4326;" << wkb_factory.create_linestring(way);
//        add_separator_to_stringstream(m_ways_linear_table_buffer);
//        m_ways_linear_table_buffer << "{";
//        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
//            if (i != way.nodes().begin()) {
//                m_ways_linear_table_buffer << ", ";
//            }
//            m_ways_linear_table_buffer << i->ref();
//        }
//        m_ways_linear_table_buffer << "}";
//        m_ways_linear_table_buffer << '\n';
//        if (m_ways_linear_table_buffer.str().size() > BUFFER_SEND_SIZE) {
//            m_ways_linear_table.start_copy();
//            m_ways_linear_table.send_line(m_ways_linear_table_buffer.str());
//            m_ways_linear_table.end_copy();
//        }
//    } catch (osmium::geometry_error& e) {
//        std::cerr << e.what() << "\n";
//    }
}


void AppendHandler::relation(const osmium::Relation& relation) {

}

