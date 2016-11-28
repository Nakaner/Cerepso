/*
 * diff_handler2.cpp
 *
 *  Created on: 04.10.2016
 *      Author: michael
 */

#include <geos/geom/CoordinateArraySequenceFactory.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/io/WKBWriter.h>
#include <geos/geom/LineString.h>
#include <osmium/osm/relation.hpp>
#include "diff_handler2.hpp"

DiffHandler2::~DiffHandler2() {
    m_expire_tiles->output_and_destroy();
    delete m_expire_tiles;
}

void DiffHandler2::node(const osmium::Node& node) {
    if (node.deleted()) { // we are finish now
        return;
    }
    if (!node.location().valid()) {
        return;
    }
    std::string query;
    prepare_node_query(node, query);
    if (node.tags().size() > 0) {
        m_nodes_table.send_line(query);
    }
    else {
        m_untagged_nodes_table.send_line(query);
    }
    m_expire_tiles->expire_from_point(node.location());
}

std::unique_ptr<const geos::geom::Coordinate> DiffHandler2::get_point_from_tables(osmium::object_id_type id) {
    // first check untagged nodes because most nodes do not have tags
    std::unique_ptr<const geos::geom::Coordinate> coord = m_untagged_nodes_table.get_point(id);
    if (!coord) { //node not found in untagged_nodes table
        coord = m_nodes_table.get_point(id);
    }
    if (!coord) { // node not found
        throw std::runtime_error((boost::format("Node %1% not found. \n") % id).str());
    }
    return coord;
}

void DiffHandler2::insert_way(const osmium::Way& way, std::string& copy_buffer) {
    char idbuffer[20];
    sprintf(idbuffer, "%ld", way.id());
    copy_buffer.append(idbuffer, strlen(idbuffer));
    add_tags(copy_buffer, way);
    add_metadata_to_stringstream(copy_buffer, way, m_config);
    geos::geom::GeometryFactory gf;
    geos::geom::CoordinateSequence* coord_sequence = gf.getCoordinateSequenceFactory()->create((size_t)0, (size_t)2);
    for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
        geos::geom::GeometryFactory gf;
        try {
            std::unique_ptr<const geos::geom::Coordinate> coord = get_point_from_tables(i->ref());
            if (coord) {
                coord_sequence->add(*(coord.get()));
            }
        } catch (std::runtime_error& e) {
            std::cerr << e.what();
        }
    }
    if (coord_sequence->size() < 2) {
        /// \todo clean up memory
        throw osmium::geometry_error((boost::format("Too few points for way %1%.") % way.id()).str());
    }
    m_expire_tiles->expire_from_coord_sequence(coord_sequence);
    std::unique_ptr<geos::geom::Geometry> linestring (gf.createLineString(coord_sequence));
    geos::io::WKBWriter wkb_writer;
    std::stringstream stream(std::ios_base::out);
    wkb_writer.writeHEX(*(linestring.get()), stream);
    copy_buffer.append("SRID=4326;");
    copy_buffer.append(stream.str());
    add_separator_to_stringstream(copy_buffer);
    copy_buffer.append("{");
    for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
        if (i != way.nodes().begin()) {
            copy_buffer.append(", ");
        }
        sprintf(idbuffer, "%ld", i->ref());
        copy_buffer.append(idbuffer, strlen(idbuffer));
    }
    copy_buffer.append("}");
    copy_buffer.push_back('\n');
    /// \todo clean up memory
}

void DiffHandler2::insert_relation(const osmium::Relation& relation) {
    try {
        char idbuffer[20];
        sprintf(idbuffer, "%ld", relation.id());
        std::string copy_buffer;
        copy_buffer.append(idbuffer, strlen(idbuffer));
        add_tags(copy_buffer, relation);
        add_metadata_to_stringstream(copy_buffer, relation, m_config);
        std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
        std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
        std::vector<osmium::object_id_type> object_ids;
        std::vector<osmium::item_type> object_types;
        std::vector<std::string> object_roles;
        for (const auto& member : relation.members()) {
            object_ids.push_back(member.ref());
            object_roles.push_back(member.role());
            object_types.push_back(member.type());
            if ((member.type() == osmium::item_type::node)) {
                std::unique_ptr<const geos::geom::Coordinate> coord = m_untagged_nodes_table.get_point(member.ref());
                if (!coord) {
                    coord = m_nodes_table.get_point(member.ref());
                }
                if (coord) {
                    m_expire_tiles->expire_from_point(coord->x, coord->y);
                    std::unique_ptr<geos::geom::Point> point (m_geom_factory.createPoint(*(coord.get())));
                    points->push_back(point.release());
                }
            }
            else if ((member.type() == osmium::item_type::way)) {
                std::unique_ptr<geos::geom::Geometry> linestring = m_ways_linear_table.get_linestring(member.ref(), m_geom_factory);
                if (linestring) {
                    geos::geom::CoordinateSequence* coord_sequence = linestring->getCoordinates();
                    m_expire_tiles->expire_from_coord_sequence(coord_sequence);
                    delete coord_sequence;
                    linestrings->push_back(linestring.release());
                }
            }
            else if ((member.type() == osmium::item_type::relation)) {
                // We do not add the geometry of this relation to the GeometryCollection.
                /// \todo support one level of nested relations
            }
        }
        // create GeometryCollection
        geos::geom::MultiPoint* multipoints = m_geom_factory.createMultiPoint(points);
        geos::geom::MultiLineString* multilinestrings = m_geom_factory.createMultiLineString(linestrings);
        // add multipoints to query string
        copy_buffer.append("SRID=4326;");
        // convert to WKB
        std::stringstream query_stream;
        geos::io::WKBWriter wkb_writer;
        wkb_writer.writeHEX(*multipoints, query_stream);
        copy_buffer.append(query_stream.str());
        delete multipoints;
        add_separator_to_stringstream(copy_buffer);
        // add multilinestrings to query string
        copy_buffer.append("SRID=4326;");
        // convert to WKB
        query_stream.str("");
        wkb_writer.writeHEX(*multilinestrings, query_stream);
        copy_buffer.append(query_stream.str());
        delete multilinestrings;
        add_separator_to_stringstream(copy_buffer);
        copy_buffer.push_back('{');
        for (std::vector<osmium::object_id_type>::const_iterator id = object_ids.begin(); id < object_ids.end(); id++) {
            if (id != object_ids.begin()) {
                copy_buffer.append(", ");
            }
            sprintf(idbuffer, "%ld", *id);
            copy_buffer.append(idbuffer);
        }
        copy_buffer.push_back('}');
        add_separator_to_stringstream(copy_buffer);
        copy_buffer.push_back('{');
        for (std::vector<osmium::item_type>::const_iterator type = object_types.begin(); type < object_types.end(); type++) {
            if (type != object_types.begin()) {
                copy_buffer.append(", ");
            }
            if (*type == osmium::item_type::node) {
                copy_buffer.push_back('n');
            } else if (*type == osmium::item_type::way) {
                copy_buffer.push_back('w');
            } else if (*type == osmium::item_type::relation) {
                copy_buffer.push_back('r');
            }
        }
        copy_buffer.push_back('}');
        add_separator_to_stringstream(copy_buffer);
        copy_buffer.push_back('{');
        for (std::vector<std::string>::const_iterator role = object_roles.begin(); role < object_roles.end(); role++) {
            if (role != object_roles.begin()) {
                copy_buffer.append(", ");
            }
            copy_buffer.push_back('\'');
            copy_buffer.append(*role);
            copy_buffer.push_back('\'');
        }
        copy_buffer.append("}\n");
        m_relations_table.send_line(copy_buffer);
    }
    catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void DiffHandler2::way(const osmium::Way& way) {
    if (m_progress != TypeProgress::WAY) {
        write_new_nodes();
    }
    if (way.deleted()) {
        return;
    }
    std::string copy_buffer;
    try {
        insert_way(way, copy_buffer);
    } catch (osmium::geometry_error& err) {
        std::cerr << err.what();
        return;
    }
    m_ways_linear_table.send_line(copy_buffer);
}


void DiffHandler2::relation(const osmium::Relation& relation) {
    if (m_progress != TypeProgress::RELATION) {
        write_new_ways();
    }
    if (relation.deleted()) {
        return;
    }
    insert_relation(relation);
}

void DiffHandler2::write_new_nodes() {
    m_nodes_table.end_copy();
    m_untagged_nodes_table.end_copy();
    m_ways_linear_table.start_copy();
    m_progress = TypeProgress::WAY;
}

void DiffHandler2::write_new_ways() {
    m_ways_linear_table.end_copy();
    m_relations_table.start_copy();
    m_progress = TypeProgress::RELATION;
}


