/*
 * columns.cpp
 *
 *  Created on: 24.06.2016
 *      Author: michael
 */
#include "columns.hpp"

Columns::Columns(Config& config, TableType type):
    m_columns(),
    m_type(type) {
    m_columns.push_back(std::make_pair("osm_id", "bigint"));
    if (config.tags_hstore && type != TableType::UNTAGGED_POINT) {
        m_columns.push_back(std::make_pair("tags", "hstore"));
    }
    if (config.metadata) {
        if (config.m_usernames) {
            m_columns.push_back(std::make_pair("osm_user", "text"));
        }
        m_columns.push_back(std::make_pair("osm_uid", "bigint"));
        m_columns.push_back(std::make_pair("osm_version", "integer"));
        m_columns.push_back(std::make_pair("osm_lastmodified", "char(23)"));
        m_columns.push_back(std::make_pair("osm_changeset", "bigint"));
    }
    switch (type) {
    case TableType::POINT :
        m_columns.push_back(std::make_pair("geom", "geometry(Point,4326)"));
        break;
    case TableType::UNTAGGED_POINT :
        m_columns.push_back(std::make_pair("geom", "geometry(Point,4326)"));
        break;
    case TableType::WAYS_LINEAR :
        m_columns.push_back(std::make_pair("geom", "geometry(LineString,4326)"));
        m_columns.push_back(std::make_pair("way_nodes", "bigint[]"));
        break;
    case TableType::WAYS_POLYGON :
        m_columns.push_back(std::make_pair("geom", "geometry(MultiPolygon,4326)"));
        m_columns.push_back(std::make_pair("way_nodes", "bigint[]"));
        break;
    case TableType::RELATION_POLYGON :
        m_columns.push_back(std::make_pair("geom", "geometry(MultiPolygon,4326)"));
//        m_columns.push_back(std::make_pair("member_ids", "bigint[]"));
//        m_columns.push_back(std::make_pair("member_types", "char[]"));
        break;
    case TableType::RELATION_OTHER :
        m_columns.push_back(std::make_pair("geom", "geometry(GeometryCollection,4326)"));
        m_columns.push_back(std::make_pair("member_ids", "bigint[]"));
        m_columns.push_back(std::make_pair("member_types", "char[]"));
    }
}

Columns::Columns(Config& config, ColumnsVector& additional_columns, TableType type) :
    Columns::Columns(config, type) {
    for (Column& col : additional_columns) {
        m_columns.push_back(col);
    }
}
