/*
 * database_location_handler.cpp
 *
 *  Created on:  2018-10-29
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "database_location_handler.hpp"
#include <osmium/index/index.hpp>
#include <iostream>

DatabaseLocationHandler::DatabaseLocationHandler(NodeLocationsTable& untagged_nodes_table) :
    m_untagged_nodes_table(untagged_nodes_table),
    m_ignore_errors(false),
    m_location_cache() {
}

void DatabaseLocationHandler::ignore_errors() {
    m_ignore_errors = true;
}

void DatabaseLocationHandler::node(const osmium::Node& node) {
    if (node.visible()) {
        // We don't have to cache deleted nodes (they don't have a valid location at all).
        m_location_cache.emplace(node.id(), node.location());
    }
}

osmium::Location DatabaseLocationHandler::get_node_location_from_persisent(const osmium::object_id_type id) const {
    return m_untagged_nodes_table.get_location(id);
}

osmium::Location DatabaseLocationHandler::get_node_location(const osmium::object_id_type id) const {
    std::unordered_map<osmium::object_id_type, osmium::Location>::const_iterator it = m_location_cache.find(id);
    if (it != m_location_cache.end()) {
        return it->second;
    }
    // look into database tables
    return get_node_location_from_persisent(id);
}

void DatabaseLocationHandler::way(osmium::Way& way) {
    bool error = false;
    for (auto& node_ref : way.nodes()) {
        node_ref.set_location(get_node_location(node_ref.ref()));
        if (!node_ref.location()) {
            std::cerr << " missing location for node " << node_ref.ref() << '\n';
            error = true;
        }
    }
    if (!m_ignore_errors && error) {
        throw osmium::not_found{"location for one or more nodes not found in node location index"};
    } else if (m_ignore_errors && error) {
        std::cerr << "way " << way.id() << ": location for one or more nodes not found in node location index\n";
    }
}
