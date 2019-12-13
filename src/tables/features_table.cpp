/*
 * feature_table.cpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "features_table.hpp"

#include <iostream>
#include <boost/format.hpp>
#include <osmium/tags/taglist.hpp>

void FeaturesTable::create_geom_index() {
    if (!m_database_connection || !initialized()) {
        return;
    }
    // pick out geometry column
    for (postgres_drivers::ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (static_cast<char>(it->type()) >= static_cast<char>(postgres_drivers::ColumnType::GEOMETRY)) {
            time_t ts = time(NULL);
            std::cerr << "Creating geometry index on table " << m_name << ", field " << it->name() << " …";
            std::stringstream query;
            query << "CREATE INDEX " << m_name << "_index_" << it->name() << " ON " << m_name << " USING GIST (" << it->name() << ")";
            send_query(query.str().c_str());
            std::cerr << " took " << static_cast<int>(time(NULL) - ts) << " seconds." << std::endl;
        }
    }
}

void FeaturesTable::delete_object(const osmium::object_id_type id) {
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

void FeaturesTable::delete_from_list(std::vector<osmium::object_id_type>& list) {
    assert(!m_copy_mode);
    for (osmium::object_id_type id : list) {
        PGresult *result = PQexec(m_database_connection, (boost::format("DELETE FROM %1% WHERE osm_id = %2%") % m_name % id).str().c_str());
        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            PQclear(result);
            throw std::runtime_error((boost::format("%1% failed: %2%\n") % (boost::format("DELETE FROM %1% WHERE osm_id = %2%") % m_name % id).str().c_str() % PQerrorMessage(m_database_connection)).str());
        }
        PQclear(result);
    }
}

void FeaturesTable::order_by_geohash() {
    if (!m_database_connection || !initialized()) {
        return;
    }
    for (postgres_drivers::ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (static_cast<char>(it->type()) >= static_cast<char>(postgres_drivers::ColumnType::GEOMETRY)) {
           time_t ts = time(NULL);
           std::cerr << " and ordering it by ST_Geohash …";
           std::stringstream query;
           //TODO add ST_Transform to EPSG:4326 once pgimporter supports other coordinate systems.
           query << "CREATE TABLE " << m_name << "_tmp" <<  " AS SELECT * from " << m_name << " ORDER BY ST_GeoHash";
           query << "(ST_Envelope(" << it->name() << "),10) COLLATE \"C\"";
           send_query(query.str().c_str());
           query.str("");
           query << "DROP TABLE " << m_name;
           send_query(query.str().c_str());
           query.str("");
           query << "ALTER TABLE " << m_name << "_tmp RENAME TO " << m_name;
           send_query(query.str().c_str());
           std::cerr << " took " << static_cast<int>(time(NULL) - ts) << " seconds." << std::endl;
           break;
       }
    }
}

void FeaturesTable::request_prepared_statements() {
    std::string query;
    query = (boost::format("SELECT 1 FROM %1% WHERE osm_id = $1") % m_name).str();
    register_prepared_statement("count_osm_id", query, 1);
    query = (boost::format("UPDATE %1% SET geom = $1 WHERE osm_id = $2") % m_name).str();
    register_prepared_statement("update_geometry", query, 2);
    query= (boost::format("DELETE FROM %1% WHERE osm_id = $1") % m_name).str();
    register_prepared_statement("delete_statement", query, 1);
}

FeaturesTable::FeaturesTable(const char* table_name, CerepsoConfig& config,
        postgres_drivers::Columns columns, osmium::osm_entity_bits::type types) :
        PostgresTable(table_name, config, columns),
        m_wkb_factory(wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex),
        m_bitmask(types) {
    request_prepared_statements();
}

FeaturesTable::FeaturesTable(postgres_drivers::Columns& columns, CerepsoConfig& config,
        osmium::osm_entity_bits::type types) :
        PostgresTable(columns, config),
        m_wkb_factory(wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex),
        m_bitmask(types) {
}

FeaturesTable::~FeaturesTable() {
    stop();
    if (config().m_order_by_geohash && !config().m_append) {
        order_by_geohash();
    }
    if (config().m_geom_indexes && !config().m_append) {
        create_geom_index();
    }
}

wkbhpp::full_wkb_factory<>& FeaturesTable::wkb_factory() {
    return m_wkb_factory;
}

bool FeaturesTable::has_interesting_tags(const osmium::TagList& tags) {
    return (tags.size() > 0 && config().m_hstore_all)
            || osmium::tags::match_any_of(tags, m_columns.filter());
}

int FeaturesTable::count_osm_id(const int64_t id) {
    assert(m_database_connection);
    char const *paramValues[1];
    static char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result;
    result = PQexecPrepared(m_database_connection, "count_osm_id", 1, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
    }
    int count = PQntuples(result);
    PQclear(result);
    return count;
}

void FeaturesTable::update_geometry(const osmium::object_id_type id, const char* geometry) {
    assert(m_database_connection);
    assert(!m_copy_mode);
    char const *paramValues[2];
    static char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = geometry;
    paramValues[1] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "update_geometry", 2, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("Updating geometry of object %1% from %2% failed: %3%\n") % id % m_name % PQresultErrorMessage(result)).str());
    }
    PQclear(result);
}
