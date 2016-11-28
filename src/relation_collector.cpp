/*
 * relation_collector.cpp
 *
 *  Created on: 01.07.2016
 *      Author: michael
 */

#include "relation_collector.hpp"
#include "postgres_handler.hpp"
#include <geos/geom/GeometryFactory.h>

RelationCollector::RelationCollector(Config& config, Columns& node_columns) :
    m_config(config),
    m_output_buffer(initial_output_buffer_size, osmium::memory::Buffer::auto_grow::yes),
    m_database_table("relations", config, node_columns)
    { }

bool RelationCollector::keep_relation(const osmium::Relation& relation) const {
    return true;
}

bool RelationCollector::keep_member(const osmium::relations::RelationMeta& relation_meta, const osmium::RelationMember& member) const {
    return true;
}

/** Helper to retrieve relation member */
osmium::Node& RelationCollector::get_member_node(size_t offset) const {
    return static_cast<osmium::Node &>(this->get_member(offset));
}

/** Helper to retrieve relation member */
osmium::Way& RelationCollector::get_member_way(size_t offset) const {
    return static_cast<osmium::Way &>(this->get_member(offset));
}

/** Helper to retrieve relation member */
osmium::Relation& RelationCollector::get_member_relation(size_t offset) const {
    return static_cast<osmium::Relation&>(this->get_member(offset));
}

void RelationCollector::complete_relation(osmium::relations::RelationMeta& relation_meta) {
    const osmium::Relation& relation = this->get_relation(relation_meta);
    // We need a stringstream because writeHEX() needs a stringstream
    std::string query;
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", relation.id());
    query.append(idbuffer);
    PostgresHandler::add_tags(query, relation);
    PostgresHandler::add_metadata_to_stringstream(query, relation, m_config);

    std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
    std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
    std::vector<osmium::object_id_type> object_ids;
    std::vector<osmium::item_type> object_types;
    std::vector<std::string> object_roles;
    try {
        std::vector<osmium::item_type> object_types;
        for (const auto& member : relation.members()) {
            object_ids.push_back(member.ref());
            object_roles.push_back(member.role());
            object_types.push_back(member.type());
            // get the offset and check the availability, needs libosmium >= 2.10.2
            std::pair<bool, size_t> available_and_offset;
            available_and_offset = this->get_availability_and_offset(member.type(), member.ref());
            if (!available_and_offset.first) {
                // object is not available in the input OSM file, therefore we skip it
                continue;
            }
            if ((member.type() == osmium::item_type::way)) {
                osmium::Way& way = this->get_member_way(available_and_offset.second);
                std::unique_ptr<geos::geom::LineString> linestring = m_geos_factory.create_linestring(way);
                linestrings->push_back(linestring.release());
            }
            else if ((member.type() == osmium::item_type::node)) {
                osmium::Node& node = this->get_member_node(available_and_offset.second);
                std::unique_ptr<geos::geom::Point> point = m_geos_factory.create_point(node);
                points->push_back(point.release());
            }
            else if ((member.type() == osmium::item_type::relation)) {
//                osmium::Relation& relation =this->get_member_relation(available_and_offset.second);
                // We do not add the geometry of this relation to the GeometryCollection.
                // TODO support one level of nested relations
            }
        }
        // create GeometryCollection
        geos::geom::MultiPoint* multipoints = m_geos_geom_factory.createMultiPoint(points);
        geos::geom::MultiLineString* multilinestrings = m_geos_geom_factory.createMultiLineString(linestrings);
        // add multipoint to query
        query.append("SRID=4326;");
        // convert to WKB
        std::stringstream query_stream;
        m_geos_wkb_writer.writeHEX(*multipoints, query_stream);
        query.append(query_stream.str());
        ImportHandler::add_separator_to_stringstream(query);
        // add multilinestring to query
        query.append("SRID=4326;");
        // convert to WKB
        query_stream.str("");
        m_geos_wkb_writer.writeHEX(*multilinestrings, query_stream);
        query.append(query_stream.str());
        ImportHandler::add_separator_to_stringstream(query);
        query.push_back('{');
        for (std::vector<osmium::object_id_type>::const_iterator id = object_ids.begin(); id < object_ids.end(); id++) {
            if (id != object_ids.begin()) {
                query.append(", ");
            }
            sprintf(idbuffer, "%ld", *id);
            query.append(idbuffer);
        }
        query.push_back('}');
        ImportHandler::add_separator_to_stringstream(query);
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
        ImportHandler::add_separator_to_stringstream(query);
        query.push_back('{');
        for (std::vector<std::string>::const_iterator role = object_roles.begin(); role < object_roles.end(); role++) {
            if (role != object_roles.begin()) {
                query.append(", ");
            }
            query.push_back('"');
            query.append(*role);
            query.push_back('"');
        }
        query.append("}\n");
        m_database_table.send_line(query);
        delete multipoints;
        delete multilinestrings;
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}
