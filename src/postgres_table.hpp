/*
 * table.hpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#ifndef POSTGRES_TABLE_HPP_
#define POSTGRES_TABLE_HPP_

#include <boost/format.hpp>
#include <sstream>
#include <libpq-fe.h>
#include <osmium/osm/types.hpp>
#include <geos/geom/Point.h>
#include <postgres_drivers/table.hpp>

#include "cerepsoconfig.hpp"

/**
 * \brief This class manages connection to a database table. We have one connection per table,
 * therefore this class is called PostgresTable, not DBConnection.
 */
class PostgresTable : public postgres_drivers::Table {
private:
    /// \brief reference to program configuration
    CerepsoConfig& m_program_config;

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
    PostgresTable() = delete;

    /**
     * \brief constructor for production, establishes database connection
     */
    PostgresTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns& columns);

    /**
     * \brief constructor for testing, does not establishes database connection
     */
    PostgresTable(postgres_drivers::Columns& columns, CerepsoConfig& config);

    ~PostgresTable();

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
     * \brief delete (try it) all objects with the given OSM object IDs
     *
     * This method will execute a `DELETE` command for each entry in this list.
     *
     * \param list list of OSM object IDs to be deleted
     *
     * \throws std::runtime_error
     */
    void delete_from_list(std::vector<osmium::object_id_type>& list);

    /**
     * \brief delete object with given ID
     *
     * This method executes the prepared statement `delete_statement`.
     *
     * \param id ID of the object
     *
     * \throws std::runtime_error
     */
    void delete_object(const osmium::object_id_type id);

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


#endif /* POSTGRES_TABLE_HPP_ */
