/*
 * relation_table.hpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef TABLE_RELATION_TABLE_HPP_
#define TABLE_RELATION_TABLE_HPP_

#include "features_table.hpp"

/**
 * Tables which contain the geometries of member nodes and member ways.
 */
class RelationsTable : public FeaturesTable {
protected:
    void request_prepared_statements();

public:
    RelationsTable() = delete;

    /**
     * \brief constructor for production, establishes database connection
     */
    RelationsTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns);

    /**
     * Not implemented in this class
     *
     * \throws std::logic_error always
     */
    void update_geometry(const osmium::object_id_type id, const char* geometry);

    /**
     * \brief Update geometry collection of point and line members of a relation.
     *
     * \param id OSM object ID (column osm_id)
     * \param points MultiPoint WKB string
     * \param lines MultiLineString WKB string
     * \throws std::runtime_error if query execution fails
     */
    void update_geometry(const osmium::object_id_type id, const char* points, const char* lines);
};

#endif /* TABLE_RELATION_TABLE_HPP_ */
