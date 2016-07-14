/*
 * postgres_handler.cpp
 *
 *  Created on: 12.07.2016
 *      Author: michael
 */


#include "postgres_handler.hpp"
#include <osmium/osm/tag.hpp>
#include <sstream>

void PostgresHandler::add_separator_to_stringstream(std::stringstream& ss) {
    ss << '\t';
}

void PostgresHandler::add_metadata_to_stringstream(std::stringstream& ss, const osmium::OSMObject& object) {
    add_separator_to_stringstream(ss);
    ss << object.user();
    add_separator_to_stringstream(ss);
    ss << object.uid();
    add_separator_to_stringstream(ss);
    ss << object.version();
    add_separator_to_stringstream(ss);
    ss << object.timestamp().to_iso();
    add_separator_to_stringstream(ss);
    ss << object.changeset();
    add_separator_to_stringstream(ss);
}

void PostgresHandler::add_tags(std::stringstream& query, const osmium::OSMObject& object) {
    add_separator_to_stringstream(query);
    bool first_tag = true;
    for (const osmium::Tag& tag : object.tags()) {
        if (!first_tag) {
            query.put(',');
        }
        Table::escape4hstore(tag.key(), query);
        query << "=>";
        Table::escape4hstore(tag.value(), query);
        first_tag = false;
    }
}

