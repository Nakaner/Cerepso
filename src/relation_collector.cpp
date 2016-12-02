/*
 * relation_collector.cpp
 *
 *  Created on: 01.07.2016
 *      Author: michael
 */

#include "relation_collector.hpp"
#include "postgres_handler.hpp"

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

void RelationCollector::process_relation(const osmium::Relation& relation) {
    std::string query;
    try {
        build_relation_query(relation, query);
        m_database_table.send_line(query);
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
}

void RelationCollector::build_relation_query(const osmium::Relation& relation, std::string& query) {
    std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
    std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
    for (const auto& member : relation.members()) {
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
    // convert to WKB
    // We need a stringstream because writeHEX() needs a stringstream
    std::stringstream multipoint_stream;
    m_geos_wkb_writer.writeHEX(*multipoints, multipoint_stream);
    // add multilinestring to query
    // convert to WKB
    std::stringstream multilinestring_stream;
    m_geos_wkb_writer.writeHEX(*multilinestrings, multilinestring_stream);
    PostgresHandler::prepare_relation_query(relation, query, multipoint_stream, multilinestring_stream, m_config);
    delete multipoints;
    delete multilinestrings;
}

void RelationCollector::complete_relation(osmium::relations::RelationMeta& relation_meta) {
    process_relation(this->get_relation(relation_meta));
}
