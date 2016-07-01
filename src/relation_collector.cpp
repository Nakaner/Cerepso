/*
 * relation_collector.cpp
 *
 *  Created on: 01.07.2016
 *      Author: michael
 */

#include "relation_collector.hpp"
#include "myhandler.hpp"
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
    std::stringstream query;
    query << relation.id();
    MyHandler::prepare_query(query, relation);

    std::vector<geos::geom::Geometry*>* geometries = new std::vector<geos::geom::Geometry*>();
    for (const auto& member : relation.members()) {
        //if (!member_missing(member, complete)) { // only if member was present in file
            if ((member.type() == osmium::item_type::way)) {
                osmium::Way& way = this->get_member_way(this->get_offset(member.type(), member.ref()));
                std::unique_ptr<geos::geom::LineString> linestring = m_geos_factory.create_linestring(way);
                geometries->push_back(linestring.release());
            }
            else if ((member.type() == osmium::item_type::node)) {
                osmium::Node& node =this->get_member_node(this->get_offset(member.type(), member.ref()));
                std::unique_ptr<geos::geom::Point> point = m_geos_factory.create_point(node);
                geometries->push_back(point.release());
            }
        //}
    }
    // create GeometryCollection
    geos::geom::GeometryCollection* geom_collection = m_geos_geom_factory.createGeometryCollection(geometries);
    query << "SRID=4326;";
    // convert to WKB
    m_geos_wkb_writer.writeHEX(*geom_collection, query);
    query << '\n';
    m_database_table.send_line(query.str());
    delete geometries;
}
