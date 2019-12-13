/*
 * feature_table.hpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef TABLE_FEATURES_TABLE_HPP_
#define TABLE_FEATURES_TABLE_HPP_

#include <osmium/osm/entity_bits.hpp>
#include <wkbhpp/osmium_wkb_wrapper.hpp>
#include "postgres_table.hpp"

class FeaturesTable : public PostgresTable {

    wkbhpp::full_wkb_factory<> m_wkb_factory;

    osmium::osm_entity_bits::type m_bitmask;
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

//    /**
//     * \brief Get index of geometry column, first column is 0.
//     */
//    int get_geometry_column_id();

    /**
     * \brief Create index on geometry column.
     */
    void create_geom_index();

protected:
    /**
     * Register all the prepared statements we need.
     */
    void request_prepared_statements();

public:
    FeaturesTable() = delete;

    /**
     * \brief constructor for production, establishes database connection
     */
    FeaturesTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns, osmium::osm_entity_bits::type types);

    /**
     * \brief constructor for testing, does not establishes database connection
     */
    FeaturesTable(postgres_drivers::Columns& columns, CerepsoConfig& config, osmium::osm_entity_bits::type types);

    ~FeaturesTable();

    wkbhpp::full_wkb_factory<>& wkb_factory();

    /**
     * Should the object be written to this table.
     */
    bool has_interesting_tags(const osmium::TagList& tags);

    /**
     * Return whether the table accepts objects of the provided entity type.
     */
    bool is_accepted_type(osmium::item_type type);

    /**
     * \brief Count how often a given OSM ID is present in the table.
     *
     * Use this method to check presence or absence of an OSM ID.
     *
     * \param id OSM ID (should be negative for areas derived from relations if searching the areas table)
     * \throws std::runtime_error If SQL query execution fails.
     * \returns number of hits
     */
    int count_osm_id(const int64_t id);

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
     * Delete objects with given IDs. Relations need to have negative IDs in the list.
     */
    void delete_from_list(std::vector<osmium::object_id_type>& list);

//    /**
//     * \brief get a way as geos::geom::LineString
//     *
//     * \param id OSM ID
//     * \param geometry_factory pointer to a GeometryFactory to build GEOS geometries
//     * \throws std::runtime_error if query execution fails
//     * \returns unique_ptr to Geometry or empty unique_ptr otherwise
//     */
//    std::unique_ptr<geos::geom::Geometry> get_linestring(const osmium::object_id_type id, geos_factory_type::pointer geometry_factory);

    /**
     * \brief Update geometry of an entry.
     *
     * \param id OSM object ID (column osm_id)
     * \param geometry WKB string
     * \throws std::runtime_error if query execution fails
     */
    void update_geometry(const osmium::object_id_type id, const char* geometry);
};



#endif /* TABLE_FEATURES_TABLE_HPP_ */
