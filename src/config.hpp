/*
 * config.hpp
 *
 *  Created on: 31.10.2016
 *      Author: michael
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

/**
 * \brief program configuration
 *
 * This struct holds to configuration of the program on runtime.
 * The configuration has been provided by the user using the command line
 * arguments.
 */
struct Config {
    /// debug modus enabled
    bool m_debug = false;

    /// OSM file to read (Osmium will detect the file format based on the file name extension
    std::string m_osm_file = "";

    /// name of the database to write to
    std::string m_database_name = "pgimportertest";

    /// store tags as hstore \unsupported
    bool tags_hstore = true;

    /**
     * Import metadata of OSM objects into the database.
     * This increase the size of the database very much.     *
     */
    bool metadata = true;

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
     * Create index on osm_id column.
     *
     * \notForAppendMode
     */
    bool m_id_index = true;

    /**
     * Import user names in addition to user IDs. Please note that this increases the size of your database by about 100 GB!
     */
    bool m_usernames = true;

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
};

#endif /* CONFIG_HPP_ */
