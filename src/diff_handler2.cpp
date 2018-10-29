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
    // check if ways have to be updated
    std::vector<osmium::object_id_type> new_way_ids = m_node_ways_table->get_way_ids(node.id());
    for (auto id : new_way_ids) {
        m_pending_ways.push_back(id);
    }
    // check if relations have to be updated
    std::vector<osmium::object_id_type> rel_ids = m_node_relations_table->get_relation_ids(node.id());
    for (auto id : rel_ids) {
        m_pending_relations.push_back(id);
    }
}

osmium::Location DiffHandler2::get_point_from_tables(osmium::object_id_type id) {
    return m_location_index.get_noexcept(id);
}

void DiffHandler2::update_relation(const osmium::object_id_type id) {
    // get relation members from relations table
    std::vector<osmium::item_type> member_types = m_relations_table.get_member_types(id);
    std::vector<osmium::object_id_type> member_ids = m_relations_table.get_member_ids(id);
    assert(member_types.size() == member_ids.size());
    std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
    std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
    std::vector<osmium::item_type>::iterator it_type = member_types.begin();
    std::vector<osmium::object_id_type>::iterator it_id = member_ids.begin();
    for (; it_type != member_types.end(), it_id != member_ids.end(); ++it_type, ++it_id) {
        if (*it_type == osmium::item_type::node) {
            osmium::Location loc = get_point_from_tables(*it_id);
            std::unique_ptr<geos::geom::Point> point (m_geom_factory.createPoint(geos::geom::Coordinate(loc.lon(), loc.lat())));
            points->push_back(point.release());
        } else if (*it_type == osmium::item_type::way) {
            std::unique_ptr<geos::geom::Geometry> linestring = m_ways_linear_table.get_linestring(*it_id, m_geom_factory);
            if (linestring) {
                linestrings->push_back(linestring.release());
            }
        }
        // We do not add the geometry of this relation to the GeometryCollection.
        /// \todo support one level of nested relations
    }
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
    m_relations_table.update_relation_member_geometry(id, multipoint_stream.str().c_str(), multilinestring_stream.str().c_str());
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
    // check if relations have to be updated
    std::vector<osmium::object_id_type> rel_ids = m_way_relations_table->get_relation_ids(way.id());
    for (auto id : rel_ids) {
        m_pending_relations.push_back(id);
    }
    // update list of member nodes
    m_node_ways_table->send_line(prepare_node_way_query(way));
    // expire tiles
    m_expire_tiles->expire_from_coord_sequence(way.nodes());
    // remove from list of pending ways
    std::vector<osmium::object_id_type>::size_type found = m_pending_ways_idx;
    for (; found != m_pending_ways.size(); ++found) {
        if (m_pending_ways.at(found) == way.id()) {
            // set to zero to indicate that the way has been processed.
            // The zero will be removed by the next call of sort() and unique().
            m_pending_ways.at(found) = 0;
            m_pending_ways_idx = found;
            break;
        }
        if (m_pending_ways.at(found) > way.id()) {
            break;
        }
    }
}

void DiffHandler2::update_way(const osmium::object_id_type id) {
    // get node list of that way
    std::vector<MemberNode> member_nodes = m_node_ways_table->get_way_nodes(id);
    // add locations
    std::string wkb;
    try {
        m_ways_linear_table.wkb_factory().linestring_start();
        for (auto& n : member_nodes) {
            n.node_ref.set_location(m_location_index.get(n.node_ref.ref()));
        }
        // build a lightweight wrapper container to avoid copying the member node list
        std::vector<osmium::NodeRef> node_refs;
        for (auto& n : member_nodes) {
            node_refs.push_back(std::move(n.node_ref));
        }
        size_t points = m_ways_linear_table.wkb_factory().fill_linestring(node_refs.begin(), node_refs.end());
        wkb = m_ways_linear_table.wkb_factory().linestring_finish(points);

        // This point is only reached if a valid geometry could be build. If so,
        // trigger a geometry update of all relations using this way.
        // Check if relations have to be updated:
        std::vector<osmium::object_id_type> rel_ids = m_way_relations_table->get_relation_ids(id);
        for (auto id : rel_ids) {
            m_pending_relations.push_back(id);
        }
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
        wkb = "010200000000000000";
    } catch (osmium::not_found& e) {
        std::cerr << e.what() << "\n";
        wkb = "010200000000000000";
    }
    m_ways_linear_table.update_geometry(id, wkb.c_str());
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
    // remove from list of pending relations
    std::vector<osmium::object_id_type>::size_type found = m_pending_relations_idx;
    for (; found != m_pending_relations.size(); ++found) {
        if (m_pending_relations.at(found) == relation.id()) {
            // set to zero to indicate that the way has been processed.
            // The zero will be removed by the next call of sort() and unique().
            m_pending_relations.at(found) = 0;
            m_pending_relations_idx = found;
            break;
        }
        if (m_pending_relations.at(found) > relation.id()) {
            break;
        }
    }
}

void DiffHandler2::write_new_nodes() {
    m_nodes_table.end_copy();
    m_untagged_nodes_table->end_copy();
    m_ways_linear_table.start_copy();
    m_node_ways_table->start_copy();
    m_progress = TypeProgress::WAY;
    // sort list of pending ways
    std::cerr << "sorting list of pending ways ...";
    std::sort(m_pending_ways.begin(), m_pending_ways.end());
    std::unique(m_pending_ways.begin(), m_pending_ways.end());
    std::cerr << " done\n";
}

void DiffHandler2::write_new_ways() {
    m_ways_linear_table.end_copy();
    m_node_ways_table->end_copy();
    work_on_pending_ways();
    // sort list of pending relations
    std::cerr << "sorting list of pending relations ...";
    std::sort(m_pending_relations.begin(), m_pending_relations.end());
    std::unique(m_pending_relations.begin(), m_pending_relations.end());
    std::cerr << " done\n";

    m_relations_table.start_copy();
    m_progress = TypeProgress::RELATION;
}

void DiffHandler2::after_relations() {
    m_relations_table.end_copy();
    std::cerr << "sorting list of pending relations ...";
    std::sort(m_pending_relations.begin(), m_pending_relations.end());
    std::unique(m_pending_relations.begin(), m_pending_relations.end());
    std::cerr << " done\n";
    std::cerr << "working on list of pending relations (" << m_pending_relations.size() << ") ...";
    for (auto id : m_pending_relations) {
        // update way
        update_relation(id);
    }
    std::cerr << " done\n";
}

void DiffHandler2::work_on_pending_ways() {
    std::cerr << "sorting list of pending ways ...";
    std::sort(m_pending_ways.begin(), m_pending_ways.end());
    std::unique(m_pending_ways.begin(), m_pending_ways.end());
    std::cerr << " done\n";
    std::cerr << "working on list of pending ways (" << m_pending_ways.size() << ") ...";
    for (auto id : m_pending_ways) {
        // update way
        update_way(id);
    }
    std::cerr << " done\n";
}
