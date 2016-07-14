/*
 * append_handler.cpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#include "append_handler.hpp"

void AppendHandler::node(const osmium::Node& node) {
    // if node has version 1, we don't have to check if it already exists
    if (node.version() > 1) {
        // delete old node, try first untagged nodes table
        m_untagged_nodes_table_copy_buffer << "DELETE FROM " << m_untagged_nodes_table.get_name() << " WHERE osm_id = " << node.id() << ";";
        if (m_untagged_nodes_table_sql_buffer.str().size()  > BUFFER_SEND_SIZE) {
            m_untagged_nodes_table.send_query(m_untagged_nodes_table_sql_buffer.str().c_str());
            m_untagged_nodes_table_sql_buffer.str("");
        }
        m_nodes_table_sql_buffer << "DELETE FROM " << m_nodes_table.get_name() << " WHERE osm_id = " << node.id() << ";";
        if (m_nodes_table_sql_buffer.str().size()  > BUFFER_SEND_SIZE) {
            m_nodes_table.send_query(m_nodes_table_sql_buffer.str().c_str());
            m_nodes_table_sql_buffer.str("");
        }
    }
    if (node.deleted()) { // we are finish now
        return;
    }
    if (node.tags().size() > 0) {
        m_nodes_table_copy_buffer << node.id();
        if (node.tags().size() > 0) {
            // If the node has tags, it will be written to nodes, not untagged_nodes table.
            add_tags(m_nodes_table_copy_buffer, node);
        }
        add_metadata_to_stringstream(m_nodes_table_copy_buffer, node);
        m_nodes_table_copy_buffer << "SRID=4326;" << wkb_factory.create_point(node);
        m_nodes_table_copy_buffer << '\n';
        if (m_nodes_table_copy_buffer.str().size() > BUFFER_SEND_SIZE) {
            // delete buffer has to be sent and cleared beforehand
            m_nodes_table.send_query(m_nodes_table_sql_buffer.str().c_str());
            m_nodes_table_sql_buffer.str("");
            m_nodes_table.start_copy();
            m_nodes_table.send_line(m_nodes_table_copy_buffer.str());
            m_nodes_table.end_copy();
            // clean buffer
            m_nodes_table_copy_buffer.str("");
        }
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

