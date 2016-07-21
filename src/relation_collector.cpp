/*
 * relation_collector.cpp
 *
 *  Created on: 01.07.2016
 *      Author: michael
 */

#include "relation_collector.hpp"
#include "postgres_handler.hpp"
#include <geos/geom/GeometryFactory.h>

RelationCollector::RelationCollector(std::shared_ptr<osmium::area::MultipolygonCollector<osmium::area::Assembler>> multipolygon_collector,
        Config& config, Columns& node_columns) :
    m_multipolygon_collector(multipolygon_collector),
    m_output_buffer(initial_output_buffer_size, osmium::memory::Buffer::auto_grow::yes),
    m_database_table("relations_other", config, node_columns)
    { }

bool RelationCollector::keep_relation(const osmium::Relation& relation) const {
    if (m_multipolygon_collector->keep_relation(relation)) {
        return false;
    }
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

void RelationCollector::complete_relation(osmium::relations::RelationMeta& relation_meta) {
    const osmium::Relation& relation = this->get_relation(relation_meta);
    // We need a stringstream because writeHEX() needs a stringstream
    std::string query;
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", relation.id());
    query.append(idbuffer);
    PostgresHandler::add_tags(query, relation);
    PostgresHandler::add_metadata_to_stringstream(query, relation);

    std::vector<geos::geom::Geometry*>* geometries = new std::vector<geos::geom::Geometry*>();
    std::vector<osmium::object_id_type> object_ids;
    std::vector<osmium::item_type> object_types;
    try {
        for (const auto& member : relation.members()) {
            //if (!member_missing(member, complete)) { // only if member was present in file
                if ((member.type() == osmium::item_type::way)) {
                    osmium::Way& way = this->get_member_way(this->get_offset(member.type(), member.ref()));
                    std::unique_ptr<geos::geom::LineString> linestring = m_geos_factory.create_linestring(way);
                    geometries->push_back(linestring.release());
                    object_ids.push_back(member.ref());
                    object_types.push_back(osmium::item_type::way);
                }
                else if ((member.type() == osmium::item_type::node)) {
                    osmium::Node& node =this->get_member_node(this->get_offset(member.type(), member.ref()));
                    std::unique_ptr<geos::geom::Point> point = m_geos_factory.create_point(node);
                    geometries->push_back(point.release());
                    object_ids.push_back(member.ref());
                    object_types.push_back(osmium::item_type::node);
                }
            //}
        }
        // create GeometryCollection
        geos::geom::GeometryCollection* geom_collection = m_geos_geom_factory.createGeometryCollection(geometries);
        query.append("SRID=4326;");
        // convert to WKB
        std::stringstream query_stream;
        m_geos_wkb_writer.writeHEX(*geom_collection, query_stream);
        query.append(query_stream.str());
        MyHandler::add_separator_to_stringstream(query);
        query.push_back('{');
        for (std::vector<osmium::object_id_type>::const_iterator id = object_ids.begin(); id < object_ids.end(); id++) {
            if (id != object_ids.begin()) {
                query.append(", ");
            }
            sprintf(idbuffer, "%ld", *id);
            query.append(idbuffer);
        }
        query.push_back('}');
        MyHandler::add_separator_to_stringstream(query);
        query.push_back('{');
        for (std::vector<osmium::item_type>::const_iterator type = object_types.begin(); type < object_types.end(); type++) {
            if (type != object_types.begin()) {
                query.append(", ");
            }
            if (*type == osmium::item_type::node) {
                query.push_back('n');
            } else if (*type == osmium::item_type::way) {
                query.push_back('w');
            }
        }
        query.append("}\n");
        m_database_table.send_line(query);
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
    delete geometries;
}
