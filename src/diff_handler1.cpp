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
    // if node has version 1, we don't have to check if it already exists
    if (node.version() > 1) {
        // expire tiles where the node has been before
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
        // delete old node, try first untagged nodes table
        // TODO untagged_nodes table maybe not necessary any more
        if (m_config.m_driver_config.untagged_nodes) {
            m_untagged_nodes_table->delete_object(node.id());
        }
        m_nodes_table.delete_object(node.id());
    }
}


void DiffHandler1::way(const osmium::Way& way) {
    if (way.version() > 1) {
        // expire all tiles which have been crossed by the linestring before
        //TODO read locations from location cache instead parsing WKB using GEOS
        std::unique_ptr<geos::geom::Geometry> old_geom = m_ways_linear_table.get_linestring(way.id(), m_geom_factory);
        if (old_geom) {
            // nullptr will be returned if there is no old version in the database (happens if importing diffs of extracts)
            m_expire_tiles->expire_from_geos_linestring(old_geom.get());
        }
        m_ways_linear_table.delete_object(way.id());
        if (m_config.m_driver_config.updateable) {
            m_node_ways_table->delete_way_node_list(way.id());
        }
    }
}


void DiffHandler1::relation(const osmium::Relation& relation) {
    if (relation.version() > 1) {
        m_relations_table.delete_object(relation.id());
        m_node_relations_table->delete_relation_member_nodes_list(relation.id());
        m_way_relations_table->delete_relation_member_ways_list(relation.id());
    }
}

