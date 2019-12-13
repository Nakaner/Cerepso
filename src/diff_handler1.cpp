/*
 * diff_handler1.cpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#include <geos/geom/CoordinateArraySequenceFactory.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/io/WKBWriter.h>
#include <geos/geom/LineString.h>
#include <osmium/osm/relation.hpp>
#include "diff_handler1.hpp"


void DiffHandler1::node(const osmium::Node& node) {
    // If node has version 1, we don't have to check if it already exists.
    // The latter check is necessary to get along with somehow broken diff files (or handcrafted diffs).
    if (node.version() == 1 && !node.deleted()) {
        return;
    }
    // expire tiles where the node has been before
    if (m_config.m_expiry_enabled) {
        try {
            // Get old location from database before it is deleted.
            osmium::Location loc = m_location_index.get_node_location_from_persisent(node.id());
            if (loc.valid()) {
                // Locations are often not found although the node has version > 1 if you import
                // a diff of an extract of OSM data which was created by comparing to extract files.
                // If a node appears in the new but not in the old file, it will appear in the diff
                // but has version > 1. Those nodes don't have to be expired in pass 1 of the diff
                // import.
                m_expire_tiles->expire_from_point(loc);
            }
        } catch (std::runtime_error& e) {
            // An exception is thrown if we import a diff which is the diff of two extracts.
            // If a node is moved from outside the extract into the extract, it will have version>1 but
            // will be contained in the "create" block.
            // Because this happens quite often, we will do nothing.
        }
    }
    // delete old node, try first untagged nodes table
    if (m_config.m_driver_config.untagged_nodes) {
        m_untagged_nodes_table->delete_object(node.id());
    }
    m_nodes_table.delete_object(node.id());
}


void DiffHandler1::way(const osmium::Way& way) {
    // The latter check is necessary to get along with somehow broken diff files (or handcrafted diffs).
    if (way.version() == 1 && !way.deleted()) {
        return;
    }
    if (m_config.m_expiry_enabled) {
        std::vector<osmium::Location> locations;
        locations.reserve(way.nodes().size());
        bool valid = true;
        for (auto& n : way.nodes()) {
            osmium::Location loc = m_location_index.get_node_location(n.ref());
            valid &= loc.valid();
            locations.push_back(std::move(loc));
        }
        if (valid) {
            m_expire_tiles->expire_from_coord_sequence(locations);
        }
    }
    m_ways_linear_table.delete_object(way.id());
    // A polygon might have been written to both the way and the polygon table. Therefore we have to check both.
    if (m_config.m_areas) {
        m_areas_table->delete_object(way.id());
    }
    if (m_config.m_driver_config.updateable) {
        m_node_ways_table->delete_way_node_list(way.id());
    }
}


void DiffHandler1::relation(const osmium::Relation& relation) {
    // The latter check is necessary to get along with somehow broken diff files (or handcrafted diffs).
    if (relation.version() > 1 || relation.deleted()) {
        m_relations_table.delete_object(relation.id());
        // A polygon might have been written to both the relations and the polygon table. Therefore we have to check both.
        if (m_config.m_areas) {
            m_areas_table->delete_object(relation.id());
        }
        m_node_relations_table->delete_members_by_object_id(relation.id());
        m_way_relations_table->delete_members_by_object_id(relation.id());
        m_relation_relations_table->delete_members_by_object_id(relation.id());
    }
}

void DiffHandler1::area(const osmium::Area&) {
}
