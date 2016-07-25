/*
 * append_handler.cpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#include "diff_handler1.hpp"
#include <geos/geom/CoordinateArraySequenceFactory.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/GeometryFactory.h>

DiffHandler1::~DiffHandler1() {
    m_nodes_table.start_copy();
    m_nodes_table.send_line(m_nodes_table_copy_buffer);
    m_nodes_table.end_copy();
    m_untagged_nodes_table.start_copy();
    m_untagged_nodes_table.send_line(m_untagged_nodes_table_copy_buffer);
    m_untagged_nodes_table.end_copy();
    m_ways_linear_table.start_copy();
    m_ways_linear_table.send_line(m_ways_table_copy_buffer);
    m_ways_linear_table.end_copy();
}

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
        char idbuffer[20];
        sprintf(idbuffer, "%ld", way.id());
        m_ways_table_copy_buffer.append(idbuffer, strlen(idbuffer));
        add_tags(m_ways_table_copy_buffer, way);
        add_metadata_to_stringstream(m_ways_table_copy_buffer, way);
        //TODO add geometry
        std::vector<geos::geom::Coordinate> way_node_locations;
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            geos::geom::GeometryFactory gf;
            geos::geom::Coordinate* coord;
            try {
                coord = m_ways_linear_table.get_point(i->ref());
            } catch (std::runtime_error& e) {
                std::cerr << e.what();
            }
            way_node_locations.push_back(*coord);
        }
        const geos::geom::CoordinateArraySequenceFactory* coord_arr_seq_factory = geos::geom::CoordinateArraySequenceFactory::instance();
        geos::geom::CoordinateArraySequence* coord_sequence = static_cast<geos::geom::CoordinateArraySequence*>(coord_arr_seq_factory->create(&way_node_locations));
        geos::geom::GeometryFactory gf;
        geos::geom::LineString* linestring = gf.createLineString(coord_sequence);
        std::stringstream geom_stream;
        geos::io::WKBWriter wkb_writer;
        wkb_writer.write(*linestring, geom_stream);
        m_ways_table_copy_buffer.append("SRID=4326;");
        m_ways_table_copy_buffer.append(geom_stream.str());
        add_separator_to_stringstream(query);
        m_ways_table_copy_buffer.append("{");
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            if (i != way.nodes().begin()) {
                m_ways_table_copy_buffer.append(", ");
            }
            sprintf(idbuffer, "%ld", i->ref());
            m_ways_table_copy_buffer.append(idbuffer, strlen(idbuffer));
        }
        m_ways_table_copy_buffer.append("}");
        m_ways_table_copy_buffer.push_back('\n');
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}


void DiffHandler1::relation(const osmium::Relation& relation) {
    if (relation.version() > 1) {
        m_relations_table.delete_object(relation.id());
    }
}

