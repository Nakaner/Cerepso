/*
 * table.hpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <libpq-fe.h>
#include <boost/format.hpp>
#include "columns.hpp"
#include <sstream>
#include <osmium/osm/types.hpp>
#include <string.h>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>

namespace postgres_drivers {

    struct MemberIdPos {
        osmium::object_id_type id;
        int pos;

        MemberIdPos(osmium::object_id_type id, int pos) :
            id(id),
            pos(pos) {
        }

        bool operator<(const MemberIdPos& other) const {
            return pos < other.pos;
        }
    };

    struct MemberIdTypePos : MemberIdPos {
        osmium::item_type type;

        MemberIdTypePos(osmium::object_id_type id, osmium::item_type type, int pos) :
            MemberIdPos(id, pos),
            type(type) {
        }
    };

    /**
     * This class manages connection to a database table. We have one connection per table,
     * therefore this class is called Table, not DBConnection.
     */
    class Table {
    protected:
        /**
         * name of the table
         */
        std::string m_name = "";

        /**
         * configuration (especially name of the database to connect to)
         */
        Config& m_config;

        /**
         * track if COPY mode has been entered
         */
        bool m_copy_mode = false;

        /**
         * track if a BEGIN COMMIT block has been opened
         */
        bool m_begin = false;

        Columns m_columns;

        /**
         * connection to database
         *
         * This pointer is a nullpointer if this table is used in demo mode (for testing purposes).
         */
        PGconn *m_database_connection;

        /**
         * maximum size of copy buffer
         */
        static const int BUFFER_SEND_SIZE = 10000;

        /**
         * create all necessary prepared statements for this table
         *
         * This method chooses the suitable prepared statements which are dependend from the table type (point vs. way vs. …).
         */
        //TODO move to pgimporter
        void create_prepared_statements() {
            // create delete statement
            std::string query;
            if (is_osm_object_table_type(m_columns.get_type())) {
                query= (boost::format("DELETE FROM %1% WHERE osm_id = $1") % m_name).str();
                create_prepared_statement("delete_statement", query, 1);
            }
            if (m_columns.get_type() == TableType::POINT) {
                query = (boost::format("SELECT ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_name).str();
                create_prepared_statement("get_location_from_point_table", query, 1);
            } else if (m_columns.get_type() == TableType::UNTAGGED_POINT) {
                query = (boost::format("SELECT x, y FROM %1% WHERE osm_id = $1") % m_name).str();
                create_prepared_statement("get_location_from_untagged_nodes_table", query, 1);
            } else if (m_columns.get_type() == TableType::WAYS_LINEAR) {
                query = (boost::format("SELECT geom FROM %1% WHERE osm_id = $1") % m_name).str();
                create_prepared_statement("get_linestring", query, 1);
                query = (boost::format("UPDATE %1% SET geom = $1 WHERE osm_id = $2") % m_name).str();
                create_prepared_statement("update_geometry", query, 2);
            } else if (m_columns.get_type() == TableType::NODE_WAYS) {
                query = (boost::format("SELECT way_id FROM %1% WHERE node_id = $1") % m_name).str();
                create_prepared_statement("get_way_ids", query, 1);
                query = (boost::format("SELECT node_id, position FROM %1% WHERE way_id = $1") % m_name).str();
                create_prepared_statement("get_nodes", query, 1);
                query = (boost::format("DELETE FROM %1% WHERE way_id = $1") % m_name).str();
                create_prepared_statement("delete_way_node_list", query, 1);
            } else if (m_columns.get_type() == TableType::RELATION_MEMBER_NODES
                    || m_columns.get_type() == TableType::RELATION_MEMBER_WAYS
                    || m_columns.get_type() == TableType::RELATION_MEMBER_RELATIONS) {
                query = (boost::format("SELECT relation_id FROM %1% WHERE member_id = $1") % m_name).str();
                create_prepared_statement("get_relation_ids_by_member", query, 1);
                query = (boost::format("DELETE FROM %1% WHERE relation_id = $1") % m_name).str();
                create_prepared_statement("delete_relation_members", query, 1);
                query= (boost::format("DELETE FROM %1% WHERE member_id = $1") % m_name).str();
                create_prepared_statement("delete_statement", query, 1);
                query = (boost::format("SELECT member_id, position FROM %1% WHERE relation_id = $1") % m_name).str();
                create_prepared_statement("get_members_by_relation_id", query, 1);
            } else if (m_columns.get_type() == TableType::RELATION_OTHER) {
                query = (boost::format("UPDATE %1% SET geom_points = $1, geom_lines = $2 WHERE osm_id = $3") % m_name).str();
                create_prepared_statement("update_relation_member_geometry", query, 3);
            }
        }

        /**
         * get ID of geometry column, first column is 0
         */
        int get_geometry_column_id();

        /**
         * For the status of the query result and throw an exception if necessary.
         *
         * This method also calls `PQclear(result)`.
         *
         * \param result query result to be checked and freed
         * \param expected_status value to be returned by `PQresultStatus(result)`
         * \param query SQL query or COPY command
         *
         * \throws std::runtime_error if check fails
         */
        void check_and_free_result(PGresult* result, ExecStatusType expected_status, const std::string& query) {
            std::string message;
            if (PQresultStatus(result) != expected_status) {
                message = PQerrorMessage(m_database_connection);
                PQclear(result);
            }
            if (!result || PQresultStatus(result) != expected_status) {
                throw std::runtime_error((boost::format("%1% failed: %2%\n") % query % message).str());
            }
            PQclear(result);
        }

    public:
        Table() = delete;

        /**
         * constructor for production, establishes database connection, read-only access to the database
         */
        /// \todo replace const char* by std::string&
        Table(const char* table_name, Config& config, Columns columns) :
                m_name(table_name),
                m_config(config),
                m_copy_mode(false),
                m_columns(columns) {
            std::string connection_params = "dbname=";
            connection_params.append(m_config.m_database_name);
            m_database_connection = PQconnectdb(connection_params.c_str());
            if (PQstatus(m_database_connection) != CONNECTION_OK) {
                throw std::runtime_error((boost::format("Cannot establish connection to database: %1%\n")
                    %  PQerrorMessage(m_database_connection)).str());
            }
        }

        /**
         * constructor for testing, does not establishes database connection
         */
        Table(Columns& columns, Config& config) :
                m_name(""),
                m_config(config),
                m_copy_mode(false),
                m_columns(columns) { }

        ~Table() {
            if (m_name != "") {
                if (m_copy_mode) {
                    end_copy();
                }
                if (m_begin) {
                    commit();
                }
                PQfinish(m_database_connection);
            }
        }

        /**
         * \brief create a prepared statement
         *
         * \param name name of the prepared statement
         * \param query template query of this statement
         * \param params_count number of argument of this query
         */
        void create_prepared_statement(const char* name, std::string query, int params_count) {
            PGresult *result = PQprepare(m_database_connection, name, query.c_str(), params_count, NULL);
            if (PQresultStatus(result) != PGRES_COMMAND_OK) {
                PQclear(result);
                throw std::runtime_error((boost::format("%1% failed: %2%\n") % query % PQerrorMessage(m_database_connection)).str());
            }
            PQclear(result);
        }

        /**
         * \brief get column definitions
         */
        const Columns& get_columns() const {
            return m_columns;
        }

        /**
         * \brief Send a line to the database (it will get it from STDIN) during copy mode.
         *
         * This method asserts that the database connection is in COPY mode when this method is called.
         *
         * \param line line to send; you may send multiple lines at once as one string, separated by \\n.
         *
         * \throws std::runtime_error
         */
        void send_line(const std::string& line) {
            assert(m_database_connection);
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

        /**
         * \brief get name of the database table
         */
        std::string& get_name() {
            return m_name;
        }

        /**
         * \brief start COPY mode
         *
         * Additionally, this method sets #m_copy_mode to `true`.
         *
         * \throws std::runtime_error
         */
        void start_copy() {
            assert(m_database_connection);
            std::string copy_command = "COPY ";
            copy_command.append(m_name);
            copy_command.append(" (");
            for (ColumnsIterator it = m_columns.begin(); it != m_columns.end(); it++) {
                copy_command.push_back('"');
                copy_command.append(it->name());
                copy_command.append("\",");
            }
            copy_command.pop_back();
            copy_command.append(") FROM STDIN");
            PGresult *result = PQexec(m_database_connection, copy_command.c_str());
            check_and_free_result(result, PGRES_COPY_IN, copy_command);
            m_copy_mode = true;
        }

        /**
         * \brief stop COPY mode
         *
         * \throws std::runtime_error
         *
         * Additionally, t method sets #m_copy_mode to `false`.
         */
        void end_copy() {
            if (!m_copy_mode) {
                // This allows us to call this method even if we are not in copy mode as a measure of safety.
                return;
            }
            assert(m_database_connection);
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

        /**
         * \brief Is the database connection in COPY mode or not?
         */
        bool get_copy() {
            return m_copy_mode;
        }

        /**
         * \brief Send `BEGIN` to table
         *
         * This method can be called both from inside the class and from outside (if you want to encapsulate all
         * your commands in a single transaction).
         */
        void send_begin() {
            send_query("BEGIN");
            m_begin = true;
        }

        /*
         * \brief Send `COMMIT` to table
         *
         * This method is intended to be called from the destructor of this class.
         */
        void commit() {
            send_query("COMMIT");
            m_begin = false;
        }

        /**
         * \brief Send any SQL query.
         *
         * This query will not return anything, i.e. it is useful for `INSERT` and `DELETE` operations.
         *
         * \param query the query
         */
        void send_query(const char* query) {
            if (!m_database_connection) {
                return;
            }
            if (m_copy_mode) {
                throw std::runtime_error((boost::format("%1% failed: You are in COPY mode.\n%2%\n") % query % PQerrorMessage(m_database_connection)).str());
            }
            PGresult *result = PQexec(m_database_connection, query);
            check_and_free_result(result, PGRES_COMMAND_OK, query);
        }

        /*
         * \brief Send `COMMIT` to table and checks if this is currently allowed (i.e. currently not in `COPY` mode)
         *
         * This method is intended to be called from outside of this class and therefore contains some additional checks.
         * It will send an additional BEGIN after sending COMMIT.
         */
        void intermediate_commit() {
            if (m_copy_mode) {
                throw std::runtime_error((boost::format("secure COMMIT failed: You are in COPY mode.\n")).str());
            }
            assert(m_begin);
            commit();
        }
    };

}

#endif /* TABLE_HPP_ */
