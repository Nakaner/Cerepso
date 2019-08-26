/*
 * config.hpp
 *
 *  Created on:  2018-08-20
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef INCLUDE_POSTGRES_DRIVERS_CONFIG_HPP_
#define INCLUDE_POSTGRES_DRIVERS_CONFIG_HPP_

#include <string>

#include <osmium/osm/metadata_options.hpp>

namespace postgres_drivers {
    /**
     * program configuration
     *
     * \todo remove members which are only neccesary for write access
     */
    struct Config {
        /// debug mode enabled
        bool m_debug = false;
        /// name of the database
        std::string m_database_name = "pgimportertest";
        /// store tags as hstore \unsupported
        bool tags_hstore = true;

        /**
         * Import metadata of OSM objects into the database.
         * This increase the size of the database a lot.
         */
        osmium::metadata_options metadata = osmium::metadata_options{"none"};

        /**
         * Create tables and columns necessary for updates.
         */
        bool updateable = true;

        /**
         * Create table of nodes without tags.
         */
        bool untagged_nodes = false;
    };
}

#endif /* INCLUDE_POSTGRES_DRIVERS_CONFIG_HPP_ */
