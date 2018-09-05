/*
 * config.hpp
 *
 *  Created on: 31.10.2016
 *      Author: michael
 */

#ifndef CEREPSOCONFIG_HPP_
#define CEREPSOCONFIG_HPP_

#include <osmium/osm/metadata_options.hpp>
#include <osmium/osm/tag.hpp>
#include <postgres_drivers/config.hpp>

/**
 * \brief program configuration
 *
 * This struct holds to configuration of the program on runtime.
 * The configuration has been provided by the user using the command line
 * arguments.
 */
class CerepsoConfig {
public:
    postgres_drivers::Config m_driver_config;

    /// OSM file to read (Osmium will detect the file format based on the file name extension
    std::string m_osm_file = "";

    /// Osm2pgsql style file path
    std::string m_style_file = "default.style";

    /// path to flatnodes file
    std::string m_flat_nodes = "flat.nodes";

    /**
     * create geometry index on untagged_nodes table
     *
     * \notForAppendMode
     */
    bool m_all_geom_indexes = false;

    /**
     * create geometry index on all tables except untagged_nodes table
     *
     * \notForAppendMode
     */
    bool m_geom_indexes = true;

    /**
     * Order all tables by ST_GeoHash. This is very time consuming and increase the duration of an import by the factor 1.5 to 2.
     *
     * Please note that the tables are copied during this step, i.e. you need at least as much free disk space (after the import)
     * as the size of the largest table (usually untagged_nodes). In 2016 this was about 400 GB (metadata enabled).
     *
     * \notForAppendMode
     */
    bool m_order_by_geohash = true;

    /**
     * Use append mode (DiffHandler1 and DiffHandler2 instead of ImportHandler and RelationCollector).
     */
    bool m_append = false;

    /**
     * Enable area support
     */
    bool m_areas = false;

    /**
     * Support associatedStreet relations
     */
    bool m_associated_streets = false;

    /**
     * Create virtual address points based on address interpolations
     */
    bool m_address_interpolations = false;

    /**
     * Add objects with tags even if they don't have any tag matching a column.
     */
    bool m_hstore_all = false;

    /**
     * Create index on osm_id column.
     *
     * \notForAppendMode
     */
    bool m_id_index = true;

    /**
     *
     */
    osmium::metadata_options m_metadata = osmium::metadata_options{"none"};

    /**
     * Selected location handler (by Osmium). See the [Osmium Concepts Manual](http://docs.osmcode.org/osmium-concepts-manual/#indexes)
     * for the list of available indexes. There is no one-index-fits-all index!
     */
    std::string m_location_handler = "sparse_mmap_array";

    /// file name where to write tile expiry logs
    std::string m_expire_tiles = "";

    /**
     * Selected TileExpiry implementation.
     *
     * Available implementations:
     *
     * - dummy (default – does nothing)
     * - classic (old osm2pgsql – might be buggy, only for backward compatability, deprecated)
     * - quadtree (new implementation, suggested)
     */
    std::string m_expiry_type = "dummy";

    /// minimum zoom level for tile expiry
    int m_min_zoom = 9;

    /// maximum zoom level for tile expiry
    int m_max_zoom = 15;

    /**
     * available options for tile expiry settings regarding relations
     */
    enum class ExpireRelationsOptions : char {
        NO_RELATIONS = 0,
        ALL = 1,
        NO_ROUTE_RELATIONS = 2
    };

    /// setting for the tile expiry of relations
    ExpireRelationsOptions m_expire_options = ExpireRelationsOptions::ALL;

    /**
     * \brief Return true if a relations should trigger a tile expiration or not.
     *
     * \param tags reference of the TagList instance of the relation to be checked
     *
     * \returns true if the relation should trigger a tile expiration
     */
    bool expire_this_relation(const osmium::TagList& tags) {
        if (m_expire_options == ExpireRelationsOptions::NO_RELATIONS) {
            return false;
        } else if (m_expire_options == ExpireRelationsOptions::ALL) {
            return true;
        }
        const char* type = tags.get_value_by_key("type");
        if (type && !strcmp(type, "route")) {
            return false;
        }
        return true;
    }
};

#endif /* CEREPSOCONFIG_HPP_ */
