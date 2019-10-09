/*
 * import_handler.cpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#include <osmium/osm/tag.hpp>
#include <osmium/tags/taglist.hpp>
#include <sstream>
#include "import_handler.hpp"

void ImportHandler::node(const osmium::Node& node) {
    handle_node(node);
}

void ImportHandler::way(const osmium::Way& way) {
    if (way.nodes().size() < 2) {
        // degenerated way (none or only one node)
        //TODO add logging
        return;
    }
    for (osmium::WayNodeList::const_iterator i = way.nodes().begin(); i < way.nodes().end(); i++) {
        if (!i->location().valid()) {
            return; //incomplete way
            //TODO support partial ways
        }
    }
    // Check if there is any interesting tag.
    if (!m_config.m_driver_config.updateable
            && !m_ways_linear_table.has_interesting_tags(way.tags())) {
        return;
    }
    const osmium::TagList* rel_tags_to_apply = get_relation_tags_to_apply(way.id(), osmium::item_type::way);
    std::string query = prepare_query(way, m_ways_linear_table, rel_tags_to_apply);
    m_ways_linear_table.send_line(query);
    if (m_config.m_driver_config.updateable) {
        query = prepare_node_way_query(way);
        m_node_ways_table->send_line(query);
    }
}

void ImportHandler::area(const osmium::Area& area) {
    handle_area(area);
}

void ImportHandler::relation(const osmium::Relation& relation) {
    if (!m_config.m_driver_config.updateable) {
        return;
    }
    std::string query = prepare_node_relation_query(relation);
    if (query.c_str()[0] != '\0') {
        m_node_relations_table->send_line(query);
    }
    query = prepare_way_relation_query(relation);
    if (query.c_str()[0] != '\0') {
        m_way_relations_table->send_line(query);
    }
    query = prepare_relation_relation_query(relation);
    if (query.c_str()[0] != '\0') {
        m_relation_relations_table->send_line(query);
    }
}
