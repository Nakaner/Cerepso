/*
 * way_nodes_table.cpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "way_nodes_table.hpp"
#include <boost/format.hpp>

WayNodesTable::WayNodesTable(const char* table_name, CerepsoConfig& config,
        postgres_drivers::Columns columns) :
    PostgresTable(table_name, config, columns) {
    request_prepared_statements();
}

WayNodesTable::~WayNodesTable() {
    stop();
}

void WayNodesTable::request_prepared_statements() {
    std::string query;
    query = (boost::format("SELECT way_id FROM %1% WHERE node_id = $1") % m_name).str();
    register_prepared_statement("get_way_ids", query, 1);
    query = (boost::format("SELECT node_id, position FROM %1% WHERE way_id = $1") % m_name).str();
    register_prepared_statement("get_nodes", query, 1);
    query = (boost::format("DELETE FROM %1% WHERE way_id = $1") % m_name).str();
    register_prepared_statement("delete_way_node_list", query, 1);
}

void WayNodesTable::delete_way_node_list(const osmium::object_id_type id) {
    assert(!m_copy_mode);
    char const *paramValues[1];
    static char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "delete_way_node_list", 1, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("Deleting object %1% from %2% failed: %3%\n") % id % m_name % PQresultErrorMessage(result)).str());
    }
    PQclear(result);
}

std::vector<osmium::object_id_type> WayNodesTable::get_way_ids(const osmium::object_id_type node_id) {
    assert(m_database_connection);
    std::vector<osmium::object_id_type> ids;
    char const *paramValues[1];
    static char buffer[64];
    sprintf(buffer, "%ld", node_id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "get_way_ids", 1, paramValues, nullptr, nullptr, 0);
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

std::vector<MemberNode> WayNodesTable::get_way_nodes(const osmium::object_id_type way_id) {
    assert(m_database_connection);
    std::vector<MemberNode> nodes;
    char const *paramValues[1];
    static char buffer[64];
    sprintf(buffer, "%ld", way_id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "get_nodes", 1, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
        return nodes;
    }
    if (PQntuples(result) == 0) {
        PQclear(result);
        return nodes;
    }
    nodes.reserve(PQntuples(result));
    for (int i = 0; i < PQntuples(result); ++i) {
        nodes.emplace_back(strtoll(PQgetvalue(result, i, 0), nullptr, 10), strtol(PQgetvalue(result, i, 1), nullptr, 10));
    }
    PQclear(result);
    std::sort(nodes.begin(), nodes.end());
    return nodes;
}
