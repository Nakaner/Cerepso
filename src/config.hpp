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
    bool m_debug = false;
    std::string m_osm_file = "";
    std::string m_database_name = "pgimportertest";
    bool tags_hstore = true;
    bool metadata = true;
    bool m_all_geom_indexes = false;
    bool m_geom_indexes = true;
    bool m_order_by_geohash = true;
    bool m_append = false;
    bool m_id_index = true;
    bool m_usernames = true;
    std::string m_location_handler = "sparse_mmap_array";
    std::string m_expire_tiles = "";
    std::string m_expiry_type = "dummy";
    int m_min_zoom = 9;
    int m_max_zoom = 15;
};

#endif /* CONFIG_HPP_ */
