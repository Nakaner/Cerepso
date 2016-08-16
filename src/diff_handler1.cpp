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
#include <geos/io/WKBWriter.h>
#include <geos/geom/LineString.h>
#include <osmium/osm/relation.hpp>

DiffHandler1::~DiffHandler1() {
    m_ways_linear_table.start_copy();
    m_ways_linear_table.send_line(m_ways_table_copy_buffer);
    m_ways_linear_table.end_copy();
}

void DiffHandler1::node(const osmium::Node& node) {
    // if node has version 1, we don't have to check if it already exists
    if (node.version() > 1) {
        // delete old node, try first untagged nodes table
        //TODO add untagged_nodes table
        m_untagged_nodes_table.delete_object(node.id());
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

void DiffHandler1::insert_way(const osmium::Way& way, std::string& ways_table_copy_buffer) {
    try {
        char idbuffer[20];
        sprintf(idbuffer, "%ld", way.id());
        ways_table_copy_buffer.append(idbuffer, strlen(idbuffer));
        add_tags(ways_table_copy_buffer, way);
        add_metadata_to_stringstream(ways_table_copy_buffer, way);
        geos::geom::GeometryFactory gf;
        geos::geom::CoordinateSequence* coord_sequence = gf.getCoordinateSequenceFactory()->create((size_t)0, (size_t)2);
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            geos::geom::GeometryFactory gf;
            try {
                // first check untagged nodes because most nodes do not have tags
                std::unique_ptr<const geos::geom::Coordinate> coord = m_untagged_nodes_table.get_point(i->ref());
                if (!coord) {
                    coord = m_nodes_table.get_point(i->ref());
                }
                if (coord) {
                    //TODO check memory leak?
                    coord_sequence->add(*(coord.release()));
                }
                else {
                    throw std::runtime_error((boost::format("Node %1% not found. \n") % i->ref()).str());
                }
            } catch (std::runtime_error& e) {
                std::cerr << e.what();
            }
        }
        std::unique_ptr<geos::geom::Geometry> linestring (gf.createLineString(coord_sequence));
        geos::io::WKBWriter wkb_writer;
        std::stringstream stream(std::ios_base::out);
        //TODO check if memory leak
        wkb_writer.writeHEX(*(linestring.get()), stream);
        ways_table_copy_buffer.append("SRID=4326;");
        ways_table_copy_buffer.append(stream.str());
        add_separator_to_stringstream(ways_table_copy_buffer);
        ways_table_copy_buffer.append("{");
        for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
            if (i != way.nodes().begin()) {
                ways_table_copy_buffer.append(", ");
            }
            sprintf(idbuffer, "%ld", i->ref());
            ways_table_copy_buffer.append(idbuffer, strlen(idbuffer));
        }
        ways_table_copy_buffer.append("}");
        ways_table_copy_buffer.push_back('\n');
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void DiffHandler1::write_new_nodes() {
    m_nodes_table.start_copy();
    m_nodes_table.send_line(m_nodes_table_copy_buffer);
    m_nodes_table.end_copy();
    m_nodes_table.intermediate_commit();
    m_untagged_nodes_table.start_copy();
    m_untagged_nodes_table.send_line(m_untagged_nodes_table_copy_buffer);
    m_untagged_nodes_table.end_copy();
    m_untagged_nodes_table.intermediate_commit();
    m_progress = TypeProgress::WAY;
}

void DiffHandler1::way(const osmium::Way& way) {
    if (m_progress == TypeProgress::POINT) {
        write_new_nodes();
    }
    if (m_progress == TypeProgress::POINT) {
        std::runtime_error("We are still in POINT mode.");
    }
    if (way.version() > 1) {
        m_ways_linear_table.delete_object(way.id());
    }
    if (way.deleted()) {
        return;
    }
    insert_way(way, m_ways_table_copy_buffer);
}


void DiffHandler1::relation(const osmium::Relation& relation) {
    if (relation.version() > 1) {
        m_relations_table.delete_object(relation.id());
    }
}

