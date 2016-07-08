/*
 * table.cpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#include "table.hpp"
#include <string.h>
#include <sstream>

void Table::escape4hstore(const char* source, std::stringstream& destination) {
    /**
    * taken from osm2pgsql/table.cpp, void table_t::escape4hstore(const char *src, string& dst)
    */
    destination.put('"');
    for (size_t i = 0; i < strlen(source); ++i) {
        switch (source[i]) {
            case '\\':
                destination << "\\\\\\\\";
                break;
            case '"':
                destination << "\\\\\"";
                break;
            case '\t':
                destination << "\\\t";
                break;
            case '\r':
                destination << "\\\r";
                break;
            case '\n':
                destination << "\\\n";
                break;
            default:
                destination.put(source[i]);
                break;
        }
    }
    destination.put('"');
}

Table::Table(const char* table_name, Config& config, Columns& columns) :
        m_name(table_name),
        m_copy_mode(false),
        m_config(config),
        m_columns(columns) {
    std::string connection_params = "dbname=";
    connection_params.append(m_config.m_database_name);
    m_database_connection = PQconnectdb(connection_params.c_str());
    if (PQstatus(m_database_connection) != CONNECTION_OK) {
        throw std::runtime_error((boost::format("Cannot establish connection to database: %1%\n")
            %  PQerrorMessage(m_database_connection)).str());
    }
    // create table
    std::string create_query = "CREATE UNLOGGED TABLE ";
    create_query.append(m_name);
    create_query.push_back('(');
    for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        create_query.append(it->first);
        create_query.push_back(' ');
        create_query.append(it->second);
        if (it != m_columns.end() - 1) {
            create_query.append(", ");
        }
    }
    create_query.push_back(')');
    send_query(create_query.c_str());
    send_begin();
    start_copy();
}

Table::~Table() {
    end_copy();
    std::cerr << "committing table " << m_name << " …";
    commit();
    if (m_columns.get_type() != TableType::UNTAGGED_POINT
            || (m_columns.get_type() == TableType::UNTAGGED_POINT && m_config.m_all_geom_indexes)) {
        if (m_config.m_order_by_geohash) {
            order_by_geohash();
        }
        create_geom_index();
    }
    PQfinish(m_database_connection);
}

void Table::order_by_geohash() {
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

void Table::end_copy() {
    if (PQputCopyEnd(m_database_connection, nullptr) != 1) {
        throw std::runtime_error(PQerrorMessage(m_database_connection));
    }
    PGresult *result = PQgetResult(m_database_connection);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("COPY END command failed: %1%\n") %  PQerrorMessage(m_database_connection)).str());
    }
    else {
        std::cerr << "COPY END " << m_name << std::endl;
    }
    m_copy_mode = false;
}

void Table::send_begin() {
    send_query("BEGIN");
}

void Table::commit() {
    send_query("COMMIT");
}

void Table::send_query(const char* query) {
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
    std::string copy_command = "COPY ";
    copy_command.append(m_name);
    copy_command.append(" (");
    for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        copy_command.append(it->first);
        if (it != m_columns.end() - 1) {
            copy_command.append(", ");
        }
    }
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
    if (!m_copy_mode) {
        throw std::runtime_error((boost::format("Insertion via COPY \"%1%\" failed: You are not in COPY mode!\n") % line).str());
    }
    if (line[line.size()-1] != '\n') {
        throw std::runtime_error((boost::format("Insertion via COPY \"%1%\" failed: Line does not end with \\n\n") % line).str());
    }
    if (PQputCopyData(m_database_connection, line.c_str(), line.size()) != 1) {
        throw std::runtime_error((boost::format("Insertion via COPY \"%1%\" failed: %2%\n") % line % PQerrorMessage(m_database_connection)).str());
    }
}

Columns& Table::get_columns() {
    return m_columns;
}
