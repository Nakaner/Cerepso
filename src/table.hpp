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
    std::string m_name;

    bool m_copy_mode = false;

    Columns& m_columns;

    Config& m_config;

    /**
     * ID of geometry column, for fast access only
     */
    int m_geom_column_id = -1;

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
     * Send any SQL query.
     *
     * This query will not return anything, i.e. it is useful for INSERT and DELETE operations.
     */
    void send_query(const char* query);

    /**
     * get the longitude and latitude of a node
     *
     * @param id OSM ID
     */
    geos::geom::Coordinate get_point(const osmium::object_id_type id);

};


#endif /* TABLE_HPP_ */
