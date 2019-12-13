/*
 * node_locations_table.cpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "node_locations_table.hpp"

void NodeLocationsTable::request_prepared_statements() {
    std::string query;
    query= (boost::format("DELETE FROM %1% WHERE osm_id = $1") % m_name).str();
    register_prepared_statement("delete_statement", query, 1);
    query = (boost::format("SELECT x, y FROM %1% WHERE osm_id = $1") % m_name).str();
    register_prepared_statement("get_location_from_untagged_nodes_table", query, 1);
}

NodeLocationsTable::NodeLocationsTable(const char* table_name, CerepsoConfig& config,
        postgres_drivers::Columns columns) :
    PostgresTable(table_name, config, columns) {
    request_prepared_statements();
}

NodeLocationsTable::NodeLocationsTable(postgres_drivers::Columns& columns, CerepsoConfig& config) :
    PostgresTable(columns, config) {
}

NodeLocationsTable::~NodeLocationsTable() {
    stop();
}

void NodeLocationsTable::delete_object(const osmium::object_id_type id) {
    assert(!m_copy_mode);
    char const *paramValues[1];
    char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "delete_statement", 1, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("Deleting object %1% from %2% failed: %3%\n") % id % m_name % PQresultErrorMessage(result)).str());
    }
    PQclear(result);
}

osmium::Location NodeLocationsTable::get_location(const osmium::object_id_type id) {
    assert(m_database_connection);
    assert(!m_copy_mode);
    char const *paramValues[1];
    static char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "get_location_from_untagged_nodes_table", 1, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
    }
    if (PQntuples(result) == 0) {
        PQclear(result);
        return osmium::Location{};
    }
    osmium::Location coord;
    // TODO this method should be split up once PostgresTable is split up into one class per table type
    if (m_columns.get_type() == postgres_drivers::TableType::POINT) {
        coord = osmium::Location{atof(PQgetvalue(result, 0, 0)), atof(PQgetvalue(result, 0, 1))};
    } else {
        coord = osmium::Location{atoi(PQgetvalue(result, 0, 0)), atoi(PQgetvalue(result, 0, 1))};
    }
    PQclear(result);
    return coord;
}
