/*
 * postgres_handler.cpp
 *
 *  Created on: 12.07.2016
 *      Author: michael
 */


#include <sstream>
#include "postgres_handler.hpp"

void PostgresHandler::add_separator_to_stringstream(std::string& ss) {
    ss.push_back('\t');
}

void PostgresHandler::add_metadata_to_stringstream(std::string& ss, const osmium::OSMObject& object, CerepsoConfig& config) {
    if (config.m_driver_config.m_usernames) {
        add_separator_to_stringstream(ss);
        PostgresTable::escape(object.user(), ss);
    }
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
        PostgresTable::escape4hstore(tag.key(), query);
        query.append("=>");
        PostgresTable::escape4hstore(tag.value(), query);
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
    PostgresHandler::add_metadata_to_stringstream(query, node, m_config);
    query.append("SRID=4326;");
    std::string wkb = "010400000000000000"; // POINT EMPTY
    try {
        wkb = wkb_factory.create_point(node);
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
    query.append(wkb);
    query.push_back('\n');
}

/*static*/ void PostgresHandler::prepare_relation_query(const osmium::Relation& relation, std::string& query,
        std::stringstream& multipoint_wkb, std::stringstream& multilinestring_wkb, CerepsoConfig& config) {
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", relation.id());
    query.append(idbuffer);
    add_tags(query, relation);
    add_metadata_to_stringstream(query, relation, config);
    std::vector<osmium::object_id_type> object_ids;
    std::vector<osmium::item_type> object_types;
    std::vector<const char*> object_roles;
    for (const auto& member : relation.members()) {
        object_ids.push_back(member.ref());
        object_roles.push_back(member.role());
        object_types.push_back(member.type());
    }
    // add multipoint to query
    query.append("SRID=4326;");
    query.append(multipoint_wkb.str());
    add_separator_to_stringstream(query);
    // add multilinestring to query
    query.append("SRID=4326;");
    query.append(multilinestring_wkb.str());
    add_separator_to_stringstream(query);
    query.push_back('{');
    for (std::vector<osmium::object_id_type>::const_iterator id = object_ids.begin(); id < object_ids.end(); id++) {
        if (id != object_ids.begin()) {
            query.append(", ");
        }
        sprintf(idbuffer, "%ld", *id);
        query.append(idbuffer);
    }
    query.push_back('}');
    add_separator_to_stringstream(query);
    query.push_back('{');
    for (std::vector<osmium::item_type>::const_iterator type = object_types.begin(); type < object_types.end(); type++) {
        if (type != object_types.begin()) {
            query.append(", ");
        }
        if (*type == osmium::item_type::node) {
            query.push_back('n');
        } else if (*type == osmium::item_type::way) {
            query.push_back('w');
        } else if (*type == osmium::item_type::relation) {
            query.push_back('r');
        }
    }
    query.push_back('}');
    add_separator_to_stringstream(query);
    query.push_back('{');
    for (std::vector<const char*>::const_iterator it = object_roles.begin(); it < object_roles.end(); it++) {
        if (it != object_roles.begin()) {
            query.append(", ");
        }
        query.push_back('"');
        PostgresTable::escape4array(*it, query);
        query.push_back('"');
    }
    query.append("}\n");
}
