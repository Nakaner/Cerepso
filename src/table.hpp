/*
 * table.hpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <boost/format.hpp>
#include <sstream>
#include <libpq-fe.h>
#include <osmium/osm/types.hpp>
#include <geos/geom/Point.h>
#include "columns.hpp"
#include "config.hpp"

/**
 * \brief This class manages connection to a database table. We have one connection per table,
 * therefore this class is called Table, not DBConnection.
 */

class Table {
private:
    /**
     * \brief name of the table
     */
    std::string m_name = "";

    /**
     * \brief track if COPY mode has been entered
     */
    bool m_copy_mode = false;

    /**
     * \brief track if a BEGIN COMMIT block has been opened
     */
    bool m_begin = false;

    /// \brief reference to columns definition
    Columns& m_columns;

    /// \brief reference to program configuration
    Config& m_config;

    /**
     * \brief connection to the database
     *
     * This pointer is a nullpointer if this table is used in demo mode (for testing purposes).
     */
    PGconn *m_database_connection;

    /**
     * maximum size of copy buffer
     */
    static const int BUFFER_SEND_SIZE = 10000;

    /**
     * \brief Create all necessary prepared statements for this table.
     *
     * This method chooses the suitable prepared statements which are dependend from the table type (point vs. way vs. …).
     */
    void create_prepared_statements();

    /**
     * \brief Create a prepared statement.
     *
     * \param name name of the statement
     * \param query the statement itself
     * \param params_count number of parameters
     */
    void create_prepared_statement(const char* name, std::string query, int params_count);

    /**
     * \brief Create index on geometry column.
     */
    void create_geom_index();

    /**
     * \brief Create index on `osm_id` column.
     */
    void create_id_index();

    /**
     * \brief Order content by `ST_GeoHash(ST_ENVELOPE(geometry_column), 10) COLLATE`
     *
     * This method reimplements the same feature of osm2pgsql. It consumes much time (approx.
     * the same amount as the import itself) and needs additional disk space because the
     * table is copied.
     *
     * See \link doc/usage.txt usage manual \endlink for details.
     */
    void order_by_geohash();

    /**
     * \brief Get index of geometry column, first column is 0.
     */
    int get_geometry_column_id();

public:
    Table() = delete;

    /**
     * \brief constructor for production, establishes database connection
     */
    Table(const char* table_name, Config& config, Columns& columns);

    /**
     * \brief constructor for testing, does not establishes database connection
     */
    Table(Columns& columns, Config& config);

    ~Table();

    /**
     * \brief get column definitions
     */
    Columns& get_columns();

    /**
     * \brief Add a key or value of an OSM tag to a hstore column and escape forbidden characters before appending.
     *
     * This method is taken from osm2pgsql/table.cpp, void table_t::escape4hstore(const char *src, string& dst).
     * Use it not only to escape forbidden characters but also to prevent SQL injections.
     *
     * \param source C string which to be escaped and appended
     * \param destination string where the escaped string has to be appended (later usually used for INSERT query or COPY)
     */
    static void escape4hstore(const char* source, std::string& destination);

    /**
     * \brief Escape a string from an insecure source and append it to another string.
     *
     * Use this method if you want to insert a string into a PostgreSQL array but have to escape certain characters to
     * prevent SQL injections.
     *
     * This method is based on osm2pgsql/middle-pgsql.cpp, char *escape_tag(char *ptr, const std::string &in, bool escape).
     *
     * \param source C string which should be escaped
     * \param destination string where the escaped string will be appended
     */
    static void escape4array(const char* source, std::string& destination);

    /**
     * \brief Escape a string from an insecure source and append it to another string.
     *
     * Use this method if you want to insert a string into the database but have to escape certain characters to
     * prevent SQL injections.
     *
     * \param source C string which should be escaped
     * \param destination string where the escaped string will be appended
     */
    static void escape(const char* source, std::string& destination);

    /**
     * check if the object mentioned by this query exists and, if yes, delete it.
     *
     * \param database_query SQL query
     *
     * \todo remove
     */
    void delete_if_existing(const char* database_query);

    /**
     * delete (try it) all objects with the given OSM object IDs
     */
    void delete_from_list(std::vector<osmium::object_id_type>& list);

    /**
     * \brief delete object with given ID
     *
     * This method executes the prepared statement `delete_statement`.
     *
     * \param id ID of the object
     */
    void delete_object(const osmium::object_id_type id);

    /**
     * \brief Send a line to the database (it will get it from STDIN) during copy mode.
     *
     * This method asserts that the database connection is in COPY mode when this method is called.
     *
     * \param line line to send; you may send multiple lines at once as one string, separated by \\n.
     */
    void send_line(const std::string& line);

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
     */
    void start_copy();

    /**
     * \brief stop COPY mode
     *
     * Additionally, t method sets #m_copy_mode to `false`.
     */
    void end_copy();

    /**
     * \brief Is the database connection in COPY mode or not?
     */
    bool get_copy();

    /**
     * \brief Send `BEGIN` to table
     *
     * This method can be called both from inside the class and from outside (if you want to get your changes persistend after
     * you sent them all to the database.
     */
    void send_begin();

    /*
     * \brief Send `COMMIT` to table
     *
     * This method is intended to be called from the destructor of this class.
     */
    void commit();

    /**
     * \brief Send any SQL query.
     *
     * This query will not return anything, i.e. it is useful for `INSERT` and `DELETE` operations.
     *
     * \param query the query
     */
    void send_query(const char* query);

    /*
     * \brief Send `COMMIT` to table and checks if this is currently allowed (i.e. currently not in `COPY` mode)
     *
     * This method is intended to be called from outside of this class and therefore contains some additional checks.
     * It will send an additional BEGIN after sending COMMIT.
     */
    void intermediate_commit();

    /**
     * \brief get the longitude and latitude of a node as geos::geom::Coordinate
     *
     * \param id OSM ID
     * \throws std::runtime_error If SQL query execution fails.
     * \returns unique_ptr to coordinate or empty unique_ptr otherwise
     */
    std::unique_ptr<geos::geom::Coordinate> get_point(const osmium::object_id_type id);

    /**
     * \brief get a way as geos::geom::LineString
     *
     * \param id OSM ID
     * \param geometry_factory reference to a GeometryFactory to build GEOS geometries
     * \throws std::runtime_error if query execution fails
     * \returns unique_ptr to Geometry or empty unique_ptr otherwise
     */
    std::unique_ptr<geos::geom::Geometry> get_linestring(const osmium::object_id_type id, geos::geom::GeometryFactory& geometry_factory);
};


#endif /* TABLE_HPP_ */
