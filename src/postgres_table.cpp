/*
 * table.cpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#include <string.h>
#include <sstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <geos/io/WKBReader.h>
#include <geos/geom/Coordinate.h>
#include <osmium/tags/taglist.hpp>
#include "postgres_table.hpp"

void PostgresTable::escape4hstore(const char* source, std::string& destination) {
    /**
    * taken from osm2pgsql/table.cpp, void table_t::escape4hstore(const char *src, string& dst)
    */
    destination.push_back('"');
    for (size_t i = 0; i < strlen(source); ++i) {
        switch (source[i]) {
            case '\\':
                destination.append("\\\\\\\\");
                break;
            case '"':
                destination.append("\\\\\"");
                break;
            case '\t':
                destination.append("\\\t");
                break;
            case '\r':
                destination.append("\\\r");
                break;
            case '\n':
                destination.append("\\\n");
                break;
            default:
                destination.push_back(source[i]);
                break;
        }
    }
    destination.push_back('"');
}

void PostgresTable::escape4array(const char* source, std::string& destination) {
    /**
    * based on osm2pgsql/middle-pgsql.cpp, char *escape_tag(char *ptr, const std::string &in, bool escape)
    */
    for (size_t i = 0; i < strlen(source); ++i) {
        switch(source[i]) {
            case '\\':
                destination.append("\\\\\\\\");
                break;
            case '\"':
                destination.append("\\\\\"");
                break;
            case '\n':
                destination.append("\\\\n");
                break;
            case '\r':
                destination.append("\\\\r");
                break;
            case '\t':
                destination.append("\\\\t");
                break;
            default:
                destination.push_back(source[i]);
                break;
        }
    }
}

void PostgresTable::escape(const char* source, std::string& destination) {
    /**
    * copied (and modified) from osm2pgsql/pgsql.cpp, void escape(const std::string &src, std::string &dst)
    */
    if (!source) {
        destination.append("\\N");
        return;
    }
    for (size_t i = 0; i < strlen(source); ++i) {
        switch(source[i]) {
            case '\\':
                destination.append("\\\\");
                break;
            case 8:
                destination.append("\\\b");
                break;
            case 12:
                destination.append("\\\f");
                break;
            case '\n':
                destination.append("\\\n");
                break;
            case '\r':
                destination.append("\\\r");
                break;
            case '\t':
                destination.append("\\\t");
                break;
            case 11:
                destination.append("\\\v");
                break;
            default:
                destination.push_back(source[i]);
                break;
        }
    }
}

PostgresTable::PostgresTable(postgres_drivers::Columns& columns, CerepsoConfig& config) :
        postgres_drivers::Table(columns, config.m_driver_config),
        m_program_config(config) {}

PostgresTable::PostgresTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns) :
        postgres_drivers::Table(table_name, config.m_driver_config, columns),
        m_program_config(config),
        m_wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex) {
}

void PostgresTable::init() {
    // delete existing table
    std::string query;
    // TODO split Table class up into two classes – one for first import, one for append mode.
    if (!m_program_config.m_append) {
        query = "DROP TABLE IF EXISTS ";
        query.append(m_name);
        send_query(query.c_str());
        // create table
        query = "CREATE UNLOGGED TABLE ";
        query.append(m_name);
        query.push_back('(');
        for (postgres_drivers::ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
            query.push_back('"');
            query.append(it->name());
            query.append("\" ");
            query.append(it->pg_type());
            if (it != m_columns.end() - 1) {
                query.append(", ");
            }
        }
        query.push_back(')');
        send_query(query.c_str());
    }
    create_prepared_statements();
    if (!m_program_config.m_append) {
        start_copy();
    }
    m_initialized = true;
}

const CerepsoConfig& PostgresTable::config() const {
    return m_program_config;
}

osmium::geom::WKBFactory<>& PostgresTable::wkb_factory() {
    return m_wkb_factory;
}

bool PostgresTable::has_interesting_tags(const osmium::TagList& tags) {
    return (tags.size() > 0 && m_program_config.m_hstore_all)
            || osmium::tags::match_any_of(tags, m_columns.filter());
}

PostgresTable::~PostgresTable() {
    if (!m_initialized) {
        return;
    }
    if (m_name != "") {
        if (m_copy_mode) {
            end_copy();
        }
        if (m_begin) {
            commit();
        }
        if (m_program_config.m_geom_indexes && !m_program_config.m_append) {
            if (m_columns.get_type() != postgres_drivers::TableType::UNTAGGED_POINT
                    || (m_columns.get_type() == postgres_drivers::TableType::UNTAGGED_POINT && m_program_config.m_all_geom_indexes)) {
                if (m_program_config.m_order_by_geohash) {
                    order_by_geohash();
                }
                create_geom_index();
            }
        }
        if (m_program_config.m_id_index && !m_program_config.m_append) {
            create_id_index();
        }
    }
}

void PostgresTable::order_by_geohash() {
    if (!m_database_connection) {
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

void PostgresTable::create_geom_index() {
    if (!m_database_connection) {
        return;
    }
    // pick out geometry column
    for (postgres_drivers::ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (static_cast<char>(it->type()) >= static_cast<char>(postgres_drivers::ColumnType::GEOMETRY)) {
            time_t ts = time(NULL);
            std::cerr << "Creating geometry index on table " << m_name << " …";
            std::stringstream query;
            query << "CREATE INDEX " << m_name << "_index_" << it->name() << " ON " << m_name << " USING GIST (" << it->name() << ")";
            send_query(query.str().c_str());
            std::cerr << " took " << static_cast<int>(time(NULL) - ts) << " seconds." << std::endl;
        }
    }
}

void PostgresTable::create_id_index() {
    if (!m_database_connection) {
        return;
    }
    // pick out geometry column
    for (postgres_drivers::ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (it->name() == "osm_id") {
            time_t ts = time(NULL);
            std::cerr << "Creating ID index on table " << m_name << " …";
            std::stringstream query;
            query << "CREATE INDEX " << m_name << "_pkey" << " ON " << m_name << " USING BTREE (osm_id)";
            send_query(query.str().c_str());
            std::cerr << " took " << static_cast<int>(time(NULL) - ts) << " seconds." << std::endl;
            break;
        }
    }
}

void PostgresTable::delete_from_list(std::vector<osmium::object_id_type>& list) {
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

void PostgresTable::delete_object(const osmium::object_id_type id) {
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

std::unique_ptr<geos::geom::Coordinate> PostgresTable::get_point(const osmium::object_id_type id) {
    assert(m_database_connection);
    assert(!m_copy_mode);
//    assert(!m_begin);
    std::unique_ptr<geos::geom::Coordinate> coord;
    char const *paramValues[1];
    char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "get_point", 1, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
        return coord;
    }
    if (PQntuples(result) == 0) {
//        throw std::runtime_error(((boost::format("Node %1% not found. ") % id)).str());
        PQclear(result);
        return coord;
    }
    coord = std::unique_ptr<geos::geom::Coordinate>(new geos::geom::Coordinate(atof(PQgetvalue(result, 0, 0)), atof(PQgetvalue(result, 0, 1))));
    PQclear(result);
    return coord;
}

std::unique_ptr<geos::geom::Geometry> PostgresTable::get_linestring(const osmium::object_id_type id, geos::geom::GeometryFactory& geometry_factory) {
    assert(m_database_connection);
    assert(!m_copy_mode);
//    assert(!m_begin);
    std::unique_ptr<geos::geom::Geometry> linestring;
    char const *paramValues[1];
    char buffer[64];
    sprintf(buffer, "%ld", id);
    paramValues[0] = buffer;
    PGresult *result = PQexecPrepared(m_database_connection, "get_linestring", 1, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
        return linestring;
    }
    if (PQntuples(result) == 0) {
//        throw std::runtime_error(((boost::format("Node %1% not found. ") % id)).str());
        PQclear(result);
        return linestring;
    }
    std::stringstream geom_stream;
    geos::io::WKBReader wkb_reader (geometry_factory);
    geom_stream.str(PQgetvalue(result, 0, 0));
    linestring = std::unique_ptr<geos::geom::Geometry>(wkb_reader.readHEX(geom_stream));
    PQclear(result);
    if (linestring->getGeometryTypeId() != geos::geom::GeometryTypeId::GEOS_LINESTRING) {
        // if we got no linestring, we will return a unique_ptr which manages nothing (i.e. nullptr equivalent)
        linestring.reset();
    }
    return linestring;
}
