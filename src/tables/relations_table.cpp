/*
 * relation_table.cpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "relations_table.hpp"

#include <boost/format.hpp>

RelationsTable::RelationsTable(const char* table_name, CerepsoConfig& config,
        postgres_drivers::Columns columns) :
    FeaturesTable(table_name, config, columns, osmium::osm_entity_bits::relation) {
    request_prepared_statements();
}

void RelationsTable::request_prepared_statements() {
    discard_all_prepared_statements();
    std::string query;
    query = (boost::format("SELECT 1 FROM %1% WHERE osm_id = $1") % m_name).str();
    register_prepared_statement("count_osm_id", query, 1);
    query = (boost::format("UPDATE %1% SET geom_points = $1, geom_lines = $2 WHERE osm_id = $3") % m_name).str();
    register_prepared_statement("update_relation_member_geometry", query, 3);
    query= (boost::format("DELETE FROM %1% WHERE osm_id = $1") % m_name).str();
    register_prepared_statement("delete_statement", query, 1);
}

void RelationsTable::update_geometry(const osmium::object_id_type /*id*/, const char* /*geometry*/) {
    assert(false && "RelationTable::update_geometry needs to be called with three arguments.");
}

void RelationsTable::update_geometry(const osmium::object_id_type id, const char* points, const char* lines) {
    assert(m_database_connection);
    assert(!m_copy_mode);
    char const *paramValues[3];
    static char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = points;
    paramValues[1] = lines;
    paramValues[2] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "update_relation_member_geometry", 3, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("Updating member geometry collection of relation %1% from %2% failed: %3%\n") % id % m_name % PQresultErrorMessage(result)).str());
    }
    PQclear(result);
}
