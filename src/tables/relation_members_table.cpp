/*
 * relation_member_table.cpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "relation_members_table.hpp"

void RelationMembersTable::request_prepared_statements() {
    std::string query;
    query = (boost::format("SELECT relation_id FROM %1% WHERE member_id = $1") % m_name).str();
    register_prepared_statement("get_relation_ids_by_member", query, 1);
    query = (boost::format("DELETE FROM %1% WHERE relation_id = $1") % m_name).str();
    register_prepared_statement("delete_relation_members", query, 1);
    query = (boost::format("SELECT member_id, position FROM %1% WHERE relation_id = $1") % m_name).str();
    register_prepared_statement("get_members_by_relation_id", query, 1);
}

RelationMembersTable::RelationMembersTable(const char* table_name, CerepsoConfig& config,
        postgres_drivers::Columns columns) :
    PostgresTable(table_name, config, columns) {
    request_prepared_statements();
}

RelationMembersTable::~RelationMembersTable() {
    stop();
}

void RelationMembersTable::delete_members_by_object_id(const osmium::object_id_type id) {
    assert(!m_copy_mode);
    char const *paramValues[1];
    char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "delete_relation_members", 1, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("Deleting object %1% from %2% failed: %3%\n") % id % m_name % PQresultErrorMessage(result)).str());
    }
    PQclear(result);
}

std::vector<osmium::object_id_type> RelationMembersTable::get_relation_ids(const osmium::object_id_type member_id) {
    assert(m_database_connection);
    std::vector<osmium::object_id_type> ids;
    char const *paramValues[1];
    static char buffer[64];
    sprintf(buffer, "%ld", member_id);
    paramValues[0] = buffer;
    PGresult *result;
    result = PQexecPrepared(m_database_connection, "get_relation_ids_by_member", 1, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
        return ids;
    }
    if (PQntuples(result) == 0) {
        PQclear(result);
        return ids;
    }
    ids.reserve(PQntuples(result));
    for (int i = 0; i < PQntuples(result); ++i) {
        ids.push_back(strtoll(PQgetvalue(result, i, 0), nullptr, 10));
    }
    PQclear(result);
    return ids;
}
