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
    handle_node(node);
}

osmium::Location DiffHandler2::get_point_from_tables(osmium::object_id_type id) {
    return m_location_index.get_noexcept(id);
}

void DiffHandler2::insert_relation(const osmium::Relation& relation, std::string& copy_buffer) {
    try {
        std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
        std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
        // check if this relation should trigger a tile expiration
        bool trigger_tile_expiry = m_config.expire_this_relation(relation.tags());
        for (const auto& member : relation.members()) {
            if ((member.type() == osmium::item_type::node)) {
                osmium::Location loc = get_point_from_tables(member.ref());
                if (loc.valid()) {
                    if (trigger_tile_expiry) {
                        m_expire_tiles->expire_from_point(loc);
                    }
                    std::unique_ptr<geos::geom::Point> point (m_geom_factory.createPoint(geos::geom::Coordinate(loc.lon(), loc.lat())));
                    points->push_back(point.release());
                }
            }
            else if ((member.type() == osmium::item_type::way)) {
                std::unique_ptr<geos::geom::Geometry> linestring = m_ways_linear_table.get_linestring(member.ref(), m_geom_factory);
                if (linestring) {
                    if (trigger_tile_expiry) {
                        geos::geom::CoordinateSequence* coord_sequence = linestring->getCoordinates();
                        m_expire_tiles->expire_from_coord_sequence(coord_sequence);
                        delete coord_sequence;
                    }
                    linestrings->push_back(linestring.release());
                }
            }
            // We do not add the geometry of this relation to the GeometryCollection.
            /// \todo support one level of nested relations
        }
        // create GeometryCollection
        geos::geom::MultiPoint* multipoints = m_geom_factory.createMultiPoint(points);
        geos::geom::MultiLineString* multilinestrings = m_geom_factory.createMultiLineString(linestrings);
        // add multipoints to query string
        // convert to WKB
        std::stringstream multipoint_stream;
        geos::io::WKBWriter wkb_writer;
        wkb_writer.writeHEX(*multipoints, multipoint_stream);
        delete multipoints;
        // add multilinestrings to query string
        // convert to WKB
        std::stringstream multilinestring_stream;
        wkb_writer.writeHEX(*multilinestrings, multilinestring_stream);
        delete multilinestrings;
        PostgresHandler::prepare_relation_query(relation, copy_buffer, multipoint_stream, multilinestring_stream, m_config,
                m_relations_table);
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
    bool with_tags = m_ways_linear_table.has_interesting_tags(way.tags());
    if (with_tags) {
        m_ways_linear_table.send_line(prepare_query(way, m_ways_linear_table, m_config, nullptr));
    }
    m_expire_tiles->expire_from_coord_sequence(way.nodes());
}


void DiffHandler2::relation(const osmium::Relation& relation) {
    if (m_progress != TypeProgress::RELATION) {
        write_new_ways();
    }
    if (relation.deleted()) {
        return;
    }
    std::string copy_buffer;
    insert_relation(relation, copy_buffer);
}

void DiffHandler2::write_new_nodes() {
    m_nodes_table.end_copy();
    m_untagged_nodes_table->end_copy();
    m_ways_linear_table.start_copy();
    m_progress = TypeProgress::WAY;
}

void DiffHandler2::write_new_ways() {
    m_ways_linear_table.end_copy();
    m_relations_table.start_copy();
    m_progress = TypeProgress::RELATION;
}


