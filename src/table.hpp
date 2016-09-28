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
#include <geos/geom/Point.h>

/**
 * This class manages connection to a database table. We have one connection per table,
 * therefore this class is called Table, not DBConnection.
 */

class Table {
private:
    /**
     * name of the table
     */
    std::string m_name = "";

    /**
     * track if COPY mode has been entered
     */
    bool m_copy_mode = false;

    /**
     * track if a BEGIN COMMIT block has been opened
     */
    bool m_begin = false;

    Columns& m_columns;

    Config& m_config;

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

    /*
     * send COMMIT to table
     *
     * This method is intended to be called from the destructor of this class.
     */
    void commit();

    /**
     * create index on geometry column
     */
    void create_geom_index();

    /**
     * create index on osm_id column
     */
    void create_id_index();

    /**
     * Order content by "ST_GeoHash(ST_ENVELOPE(geometry_column), 10) COLLATE"
     *
     * See docs/usage.md for details.
     */
    void order_by_geohash();

    void send_begin();

    /**
     * get ID of geometry column, first column is 0
     */
    int get_geometry_column_id();

public:
    Table() = delete;

    /**
     * constructor for production, establishes database connection
     */
    Table(const char* table_name, Config& config, Columns& columns);

    /**
     * constructor for testing, does not establishes database connection
     */
    Table(Columns& columns, Config& config);

    ~Table();


    Columns& get_columns();

    /**
     * escape a string which should be inserted into a hstore column
     *
     * @param source string which should be escaped
     * @param destination string where the escaped string has to be appended (later usually used for INSERT query)
     */
    static void escape4hstore(const char* source, std::string& destination);

    /**
     * escape a string which should be inserted into a non-hstore column which is inserted into the database using SQL COPY
     *
     * @param source string which should be escaped
     * @param destination string where the escaped string has to be appended (later usually used for INSERT query)
     */
    static void escape(const char* source, std::string& destination);

    /**
     * check if the object mentioned by this query exists and, if yes, delete it.
     *
     * @param query SQL query
     */
    void delete_if_existing(const char* database_query);

    /**
     * delete (try it) all objects with the given OSM object IDs
     */
    void delete_from_list(std::vector<osmium::object_id_type>& list);

    /**
     * delete an object with given ID
     */
    void delete_object(const osmium::object_id_type id);

    /**
     * send a line to the database (it will get it from STDIN) during copy mode
     */
    void send_line(const std::string& line);


    std::string& get_name() {
        return m_name;
    }

    /**
     * start COPY mode
     */
    void start_copy();

    /**
     * stop COPY mode
     */
    void end_copy();

    /**
     * get current state of connection
     * Is it in copy mode or not?
     */
    bool get_copy();

    /**
     * Send any SQL query.
     *
     * This query will not return anything, i.e. it is useful for INSERT and DELETE operations.
     */
    void send_query(const char* query);

    /*
     * send COMMIT to table and checks if this is currently allowed (i.e. currently not in COPY mode)
     *
     * This method is intended to be called from outside of this class and therefore contains some additional checks.
     * It will send an additional BEGIN after sending COMMIT.
     */
    void intermediate_commit();

    /**
     * get the longitude and latitude of a node
     *
     * @param id OSM ID
     *
     * @returns unique_ptr to coordinate or empty unique_ptr (equivalent to nullptr if it were a raw pointer) otherwise
     */
    std::unique_ptr<geos::geom::Coordinate> get_point(const osmium::object_id_type id);

    /**
     * get a way as GEOS LineString
     *
     * @param id OSM ID
     *
     * @returns unique_ptr to Geometry or empty unique_ptr otherwise
     */
    std::unique_ptr<geos::geom::Geometry> get_linestring(const osmium::object_id_type id, geos::geom::GeometryFactory& geometry_factory);
};


#endif /* TABLE_HPP_ */
