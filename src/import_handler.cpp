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
    if (!node.location().valid()) {
        return;
    }
    if (!m_config.m_driver_config.updateable && node.tags().empty()) {
        return;
    }
    std::string query;
    bool with_tags = m_nodes_table.has_interesting_tags(node.tags());
    if (with_tags) {
        const osmium::TagList* rel_tags_to_apply = get_relation_tags_to_apply(node.id(), osmium::item_type::node);
        std::string query = prepare_query(node, m_nodes_table, m_config, rel_tags_to_apply);
        m_nodes_table.send_line(query);
    }
    else if (m_config.m_driver_config.updateable) {
        std::string query = prepare_query(node, *m_untagged_nodes_table, m_config, nullptr);
        m_untagged_nodes_table->send_line(query);
    }
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
    std::string query = prepare_query(way, m_ways_linear_table, m_config, rel_tags_to_apply);
    m_ways_linear_table.send_line(query);
}

void ImportHandler::area(const osmium::Area& area) {
    if (!m_areas_table->has_interesting_tags(area.tags())) {
        return;
    }
    const osmium::TagList* rel_tags_to_apply;
    if (area.from_way()) {
        rel_tags_to_apply = get_relation_tags_to_apply(area.orig_id(), osmium::item_type::way);
    } else {
        rel_tags_to_apply = get_relation_tags_to_apply(area.orig_id(), osmium::item_type::relation);
    }
    std::string query = prepare_query(area, *m_areas_table, m_config, rel_tags_to_apply);
    m_areas_table->send_line(query);
}
