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
    m_expire_tiles->output_and_destroy();
    delete m_expire_tiles;
//    m_relations_table.start_copy();
//    m_relations_table.send_line(m_relations_table_copy_buffer);
    if (m_relations_table.get_copy()) {
        m_relations_table.end_copy();
    }
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
        //TODO support expiry mark for deleted nodes
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
    m_expire_tiles->expire_from_point(node.location());
}

void DiffHandler1::insert_way(const osmium::Way& way, std::string& ways_table_copy_buffer) {
    try {
        char idbuffer[20];
        sprintf(idbuffer, "%ld", way.id());
        //TODO Rewrite this part. Diff import should have to passes, one to delete old object, one to reimport the new ones.
        std::string copy_buffer;
        copy_buffer.append(idbuffer, strlen(idbuffer));
        add_tags(copy_buffer, way);
        add_metadata_to_stringstream(copy_buffer, way, m_config);
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
                    coord_sequence->add(*(coord.get()));
                }
                else {
                    throw std::runtime_error((boost::format("Node %1% not found. \n") % i->ref()).str());
                }
            } catch (std::runtime_error& e) {
                std::cerr << e.what();
            }
        }
        if (coord_sequence->size() < 2) {
            //TODO clean up memory
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
        ways_table_copy_buffer.append(copy_buffer);
        //TODO clean up memory
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void DiffHandler1::insert_relation(const osmium::Relation& relation) {
    try {
        char idbuffer[20];
        sprintf(idbuffer, "%ld", relation.id());
        //TODO Rewrite this part. Diff import should have to passes, one to delete old object, one to reimport the new ones.
        std::string copy_buffer;
        copy_buffer.append(idbuffer, strlen(idbuffer));
        add_tags(copy_buffer, relation);
        add_metadata_to_stringstream(copy_buffer, relation, m_config);
        geos::geom::GeometryFactory gf;
        std::vector<geos::geom::Geometry*>* geometries = new std::vector<geos::geom::Geometry*>();
        std::vector<osmium::object_id_type> object_ids;
        std::vector<osmium::item_type> object_types;
        for (const auto& member : relation.members()) {
            object_ids.push_back(member.ref());
            if ((member.type() == osmium::item_type::node)) {
                std::unique_ptr<const geos::geom::Coordinate> coord = m_untagged_nodes_table.get_point(member.ref());
                if (!coord) {
                    coord = m_nodes_table.get_point(member.ref());
                }
                if (coord) {
                    m_expire_tiles->expire_from_point(coord->x, coord->y);
                    std::unique_ptr<geos::geom::Point> point (gf.createPoint(*(coord.get())));
                    geometries->push_back(point.release());
                }
                object_types.push_back(osmium::item_type::node);
            }
            else if ((member.type() == osmium::item_type::way)) {
                std::unique_ptr<geos::geom::Geometry> linestring = m_ways_linear_table.get_linestring(member.ref(), gf);
                if (linestring) {
                    geos::geom::CoordinateSequence* coord_sequence = linestring->getCoordinates();
                    m_expire_tiles->expire_from_coord_sequence(coord_sequence);
                    delete coord_sequence;
                    geometries->push_back(linestring.release());
                }
                object_types.push_back(osmium::item_type::way);
            }
            else if ((member.type() == osmium::item_type::relation)) {
                // We do not add the geometry of this relation to the GeometryCollection.
                // TODO support one level of nested relations
                object_types.push_back(osmium::item_type::relation);
            }
        }
        // create GeometryCollection
        geos::geom::GeometryCollection* geom_collection = gf.createGeometryCollection(geometries);
        copy_buffer.append("SRID=4326;");
        // convert to WKB
        std::stringstream query_stream;
        geos::io::WKBWriter wkb_writer;
        wkb_writer.writeHEX(*geom_collection, query_stream);
        copy_buffer.append(query_stream.str());
        delete geom_collection;
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
        copy_buffer.append("}\n");
        m_relations_table.send_line(copy_buffer);
    }
    catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void DiffHandler1::write_new_nodes() {
    // don't try to write a empty string via COPY (happens if diff contains no nodes)
    if (m_nodes_table_copy_buffer.length() > 0) {
        m_nodes_table.start_copy();
        m_nodes_table.send_line(m_nodes_table_copy_buffer);
        m_nodes_table.end_copy();
    }
    if (m_untagged_nodes_table_copy_buffer.length() > 0) {
        m_untagged_nodes_table.start_copy();
        m_untagged_nodes_table.send_line(m_untagged_nodes_table_copy_buffer);
        m_untagged_nodes_table.end_copy();
    }
    m_progress = TypeProgress::WAY;
}

void DiffHandler1::write_new_ways() {
    if (m_ways_table_copy_buffer.length() > 0) {
        m_ways_linear_table.start_copy();
        m_ways_linear_table.send_line(m_ways_table_copy_buffer);
        m_ways_linear_table.end_copy();
    }
    m_progress = TypeProgress::RELATION;
}

void DiffHandler1::way(const osmium::Way& way) {
    if (m_progress == TypeProgress::POINT) {
        write_new_nodes();
    }
    if (way.version() > 1) {
        m_ways_linear_table.delete_object(way.id());
    }
    if (way.deleted()) {
        return;
    }
    try {
        insert_way(way, m_ways_table_copy_buffer);
    } catch (osmium::geometry_error& err) {
        std::cerr << err.what();
    }
}


void DiffHandler1::relation(const osmium::Relation& relation) {
    if (m_progress == TypeProgress::WAY) {
        write_new_ways();
        m_relations_table.start_copy();
    }
    if (relation.version() > 1) {
        m_relations_table.delete_object(relation.id());
    }
    if (relation.deleted()) {
        return;
    }
    insert_relation(relation);
}

