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

#include <array_parser.hpp>

#include "../item_type_conversion.hpp"
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
        m_program_config(config) {
}

PostgresTable::PostgresTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns) :
        postgres_drivers::Table(table_name, config.m_driver_config, columns),
        m_program_config(config) {
}

void PostgresTable::register_prepared_statement(std::string name, std::string query, int param_count) {
    m_prepared_statements.emplace_back(name, query, param_count);
}

void PostgresTable::discard_all_prepared_statements() {
    m_prepared_statements.clear();
}

void PostgresTable::create_prepared_statements() {
    for (auto& s : m_prepared_statements) {
        create_prepared_statement(s.name.c_str(), s.query, s.param_count);
    }
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

/*static*/ void PostgresTable::add_separator_to_stringstream(std::string& ss) {
    ss.push_back('\t');
}

void PostgresTable::stop() {
    if (m_copy_mode) {
        end_copy();
    }
    if (m_begin) {
        commit();
    }
}

bool PostgresTable::initialized() {
    return m_initialized;
}

const CerepsoConfig& PostgresTable::config() {
    return m_program_config;
}

PostgresTable::~PostgresTable() {
    if (!m_initialized) {
        return;
    }
    if (m_name != "") {
        stop();
        if (m_program_config.m_id_index && !m_program_config.m_append) {
            create_id_index();
        }
    }
}

void PostgresTable::create_id_index() {
    if (!m_database_connection) {
        return;
    }
    // pick out geometry column
    int index_count = 1;
    for (postgres_drivers::ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
        if (it->column_class() == postgres_drivers::ColumnClass::OSM_ID
                || it->column_class() == postgres_drivers::ColumnClass::NODE_ID
                || it->column_class() == postgres_drivers::ColumnClass::WAY_ID
                || it->column_class() == postgres_drivers::ColumnClass::RELATION_ID) {
            time_t ts = time(NULL);
            std::cerr << "Creating ID index on table " << m_name << ", column " << it->name() << " …";
            std::stringstream query;
            query << "CREATE INDEX " << m_name << "_pkey_" << index_count << " ON " << m_name << " USING BTREE (\"" << it->name() << "\");";
            send_query(query.str().c_str());
            std::cerr << " took " << static_cast<int>(time(NULL) - ts) << " seconds." << std::endl;
            ++index_count;
        }
    }
}
