/*
 * table.cpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#include "table.hpp"
#include <string.h>
#include <sstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <geos/io/WKBReader.h>
#include <geos/geom/Coordinate.h>

void Table::escape4hstore(const char* source, std::string& destination) {
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

void Table::escape(const char* source, std::string& destination) {
    /**
    * copied (and modified) from osm2pgsql/pgsql.cpp, void escape(const std::string &src, std::string &dst)
    */
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

Table::Table(Columns& columns, Config& config) :
        m_copy_mode(false),
        m_columns(columns),
        m_config(config) { }

Table::Table(const char* table_name, Config& config, Columns& columns) :
        m_name(table_name),
        m_copy_mode(false),
        m_columns(columns),
        m_config(config) {
    std::string connection_params = "dbname=";
    connection_params.append(m_config.m_database_name);
    m_database_connection = PQconnectdb(connection_params.c_str());
    if (PQstatus(m_database_connection) != CONNECTION_OK) {
        throw std::runtime_error((boost::format("Cannot establish connection to database: %1%\n")
            %  PQerrorMessage(m_database_connection)).str());
    }
    // delete existing table
    std::string query;
    // TODO split Table class up into two classes – one for first import, one for append mode.
    if (!m_config.m_append) {
        query = "DROP TABLE IF EXISTS ";
        query.append(m_name);
        send_query(query.c_str());
        // create table
        query = "CREATE UNLOGGED TABLE ";
        query.append(m_name);
        query.push_back('(');
        for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
            query.append(it->first);
            query.push_back(' ');
            query.append(it->second);
            if (it != m_columns.end() - 1) {
                query.append(", ");
            }
        }
        query.push_back(')');
        send_query(query.c_str());
    }
    if (!m_config.m_append) {
        start_copy();
    }
}

Table::~Table() {
    if (m_name != "") {
        if (!m_config.m_append) {
            end_copy();
        }
        if (m_config.m_geom_indexes && !m_config.m_append) {
            if (m_columns.get_type() != TableType::UNTAGGED_POINT
                    || (m_columns.get_type() == TableType::UNTAGGED_POINT && m_config.m_all_geom_indexes)) {
                if (m_config.m_order_by_geohash) {
                    order_by_geohash();
                }
                create_geom_index();
            }
        }
        if (m_config.m_id_index && !m_config.m_append) {
            create_id_index();
        }
        PQfinish(m_database_connection);
    }
}

void Table::order_by_geohash() {
    if (!m_database_connection) {
        return;
    }
    for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (it->second.compare(0, 8, "geometry") == 0) {
           time_t ts = time(NULL);
           std::cerr << " and ordering it by ST_Geohash …";
           std::stringstream query;
           //TODO add ST_Transform to EPSG:4326 once pgimporter supports other coordinate systems.
           query << "CREATE TABLE " << m_name << "_tmp" <<  " AS SELECT * from " << m_name << " ORDER BY ST_GeoHash";
           query << "(ST_Envelope(" << it->first << "),10) COLLATE \"C\"";
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

void Table::create_geom_index() {
    if (!m_database_connection) {
        return;
    }
    // pick out geometry column
    for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (it->second.compare(0, 8, "geometry") == 0) {
            time_t ts = time(NULL);
            std::cerr << "Creating geometry index on table " << m_name << " …";
            std::stringstream query;
            query << "CREATE INDEX " << m_name << "_index" << " ON " << m_name << " USING GIST (" << it->first << ")";
            send_query(query.str().c_str());
            std::cerr << " took " << static_cast<int>(time(NULL) - ts) << " seconds." << std::endl;
            break;
        }
    }
}

void Table::create_id_index() {
    if (!m_database_connection) {
        return;
    }
    // pick out geometry column
    for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (it->first.compare(0, 8, "osm_id") == 0) {
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

void Table::end_copy() {
    if (!m_database_connection) {
        return;
    }
    if (PQputCopyEnd(m_database_connection, nullptr) != 1) {
        throw std::runtime_error(PQerrorMessage(m_database_connection));
    }
    PGresult *result = PQgetResult(m_database_connection);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        throw std::runtime_error((boost::format("COPY END command failed: %1%\n") %  PQerrorMessage(m_database_connection)).str());
        PQclear(result);
    }
    m_copy_mode = false;
    PQclear(result);
}

bool Table::get_copy() {
    return m_copy_mode;
}

void Table::send_begin() {
    send_query("BEGIN");
    m_begin = true;
}

void Table::commit() {
    send_query("COMMIT");
    m_begin = false;
}

void Table::intermediate_commit() {
    if (m_copy_mode) {
        throw std::runtime_error((boost::format("secure COMMIT failed: You are in COPY mode.\n")).str());
    }
    assert(m_begin);
    commit();
}

void Table::send_query(const char* query) {
    if (!m_database_connection) {
        return;
    }
    if (m_copy_mode) {
        throw std::runtime_error((boost::format("%1% failed: You are in COPY mode.\n") % query % PQerrorMessage(m_database_connection)).str());
    }
    PGresult *result = PQexec(m_database_connection, query);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("%1% failed: %2%\n") % query % PQerrorMessage(m_database_connection)).str());
    }
    PQclear(result);
}

void Table::start_copy() {
    if (!m_database_connection) {
        std::runtime_error("No database connection.");
    }
    std::string copy_command = "COPY ";
    copy_command.append(m_name);
    copy_command.append(" (");
    for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        copy_command.append(it->first);
        copy_command.push_back(',');
    }
    copy_command.pop_back();
    copy_command.append(") FROM STDIN");
    PGresult *result = PQexec(m_database_connection, copy_command.c_str());
    if (PQresultStatus(result) != PGRES_COPY_IN) {
        PQclear(result);
        throw std::runtime_error((boost::format("%1% failed: %2%\n") % copy_command % PQerrorMessage(m_database_connection)).str());
    }
    PQclear(result);
    m_copy_mode = true;
}

void Table::send_line(const std::string& line) {
    if (!m_database_connection) {
        return;
    }
    if (!m_copy_mode) {
        throw std::runtime_error((boost::format("Insertion via COPY \"%1%\" failed: You are not in COPY mode!\n") % line).str());
    }
    if (line[line.size()-1] != '\n') {
        throw std::runtime_error((boost::format("Insertion via COPY into %1% failed: Line does not end with \\n\n%2%") % m_name % line).str());
    }
    if (PQputCopyData(m_database_connection, line.c_str(), line.size()) != 1) {
        throw std::runtime_error((boost::format("Insertion via COPY \"%1%\" failed: %2%\n") % line % PQerrorMessage(m_database_connection)).str());
    }
}

Columns& Table::get_columns() {
    return m_columns;
}

void Table::delete_from_list(std::vector<osmium::object_id_type>& list) {
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

void Table::delete_object(const osmium::object_id_type id) {
    assert(!m_copy_mode);
    PGresult *result = PQexec(m_database_connection, (boost::format("DELETE FROM %1% WHERE osm_id = %2%") % m_name % id).str().c_str());
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("Deleting object %1% from %2% failed: %3%\n") % id % m_name % PQresultErrorMessage(result)).str());
    }
    PQclear(result);
}

std::unique_ptr<geos::geom::Coordinate> Table::get_point(const osmium::object_id_type id) {
    assert(m_database_connection);
    assert(!m_copy_mode);
    assert(!m_begin);
    std::unique_ptr<geos::geom::Coordinate> coord;
    PGresult *result = PQexec(m_database_connection, (boost::format("SELECT ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = %2%") % m_name % id).str().c_str());
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

std::unique_ptr<geos::geom::Geometry> Table::get_linestring(const osmium::object_id_type id, geos::geom::GeometryFactory& geometry_factory) {
    assert(m_database_connection);
    assert(!m_copy_mode);
    assert(!m_begin);
    std::unique_ptr<geos::geom::Geometry> linestring;
    PGresult *result = PQexec(m_database_connection, (boost::format("SELECT geom FROM %1% WHERE osm_id = %2%") % m_name % id).str().c_str());
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
    return linestring;
}

