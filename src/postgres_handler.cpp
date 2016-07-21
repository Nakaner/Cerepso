/*
 * postgres_handler.cpp
 *
 *  Created on: 12.07.2016
 *      Author: michael
 */


#include "postgres_handler.hpp"
#include <osmium/osm/tag.hpp>
#include <sstream>

void PostgresHandler::add_separator_to_stringstream(std::string& ss) {
    ss.push_back('\t');
}

void PostgresHandler::add_metadata_to_stringstream(std::string& ss, const osmium::OSMObject& object) {
    add_separator_to_stringstream(ss);
    ss.append(object.user());
    add_separator_to_stringstream(ss);
    static char idbuffer[20];
    sprintf(idbuffer, "%u", object.uid());
    ss.append(idbuffer);
    add_separator_to_stringstream(ss);
    sprintf(idbuffer, "%u", object.version());
    ss.append(idbuffer);
    add_separator_to_stringstream(ss);
    ss.append(object.timestamp().to_iso());
    add_separator_to_stringstream(ss);
    sprintf(idbuffer, "%u", object.changeset());
    ss.append(idbuffer);
    add_separator_to_stringstream(ss);
}

void PostgresHandler::add_tags(std::string& query, const osmium::OSMObject& object) {
    add_separator_to_stringstream(query);
    bool first_tag = true;
    for (const osmium::Tag& tag : object.tags()) {
        if (!first_tag) {
            query.push_back(',');
        }
        Table::escape4hstore(tag.key(), query);
        query.append("=>");
        Table::escape4hstore(tag.value(), query);
        first_tag = false;
    }
}

void PostgresHandler::prepare_node_query(const osmium::Node& node, std::string& query) {
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", node.id());
    query.append(idbuffer, strlen(idbuffer));
    if (node.tags().size() > 0) {
        // If the node has tags, it will be written to nodes, not untagged_nodes table.
        PostgresHandler::add_tags(query, node);
    }
    PostgresHandler::add_metadata_to_stringstream(query, node);
    query.append("SRID=4326;");
    query.append(wkb_factory.create_point(node).c_str());
    query.push_back('\n');
}
