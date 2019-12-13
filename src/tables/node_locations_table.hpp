/*
 * node_locations_table.hpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef TABLE_NODE_LOCATIONS_TABLE_HPP_
#define TABLE_NODE_LOCATIONS_TABLE_HPP_

#include "postgres_table.hpp"

/**
 * Database table storing node locations.
 */
//TODO Derive from postgres_drivers::Table and remove usage of postgres_drivers::Columns completely.
class NodeLocationsTable : public PostgresTable {
    /**
     * Register all the prepared statements we need.
     */
    void request_prepared_statements();

public:
    NodeLocationsTable() = delete;

    /**
     * \brief constructor for production, establishes database connection
     */
    NodeLocationsTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns);

    /**
     * \brief constructor for testing purposes
     */
    NodeLocationsTable(postgres_drivers::Columns& columns, CerepsoConfig& config);

    ~NodeLocationsTable();

    /**
     * \brief delete object with given ID
     *
     * This method executes the prepared statement `delete_statement`.
     *
     * \param id ID of the object
     *
     * \throws std::runtime_error
     */
    void delete_object(const osmium::object_id_type id);

    /**
     * \brief get the longitude and latitude of a node
     *
     * \param id OSM ID
     * \throws std::runtime_error If SQL query execution fails.
     * \returns unique_ptr to coordinate or empty unique_ptr otherwise
    */
    osmium::Location get_location(const osmium::object_id_type id);
};


#endif /* TABLE_NODE_LOCATIONS_TABLE_HPP_ */
