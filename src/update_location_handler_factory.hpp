/*
 * update_location_handler_factory.hpp
 *
 *  Created on:  2018-10-31
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef UPDATE_LOCATION_HANDLER_FACTORY_HPP_
#define UPDATE_LOCATION_HANDLER_FACTORY_HPP_

#include <memory>
#include "database_location_handler.hpp"
#include "file_based_location_handler.hpp"

/**
 * Create a location handler.
 *
 * \param nodes_table database table "nodes"
 * \param untagged_nodes_table database table "untagged_nodes"
 * \param storage_pos pointer to map to store locations. If this parameter is a null pointer, a DatabaseLocationHandler
 * will be returned. If this parameter is not a null pointer, an instance of osmium::handler::NodeLocationsForWays will
 * be returned and the other arguments will be ignored.
 */
template <class TLocationStorage>
std::unique_ptr<UpdateLocationHandler> make_handler(PostgresTable& nodes_table, PostgresTable& untagged_nodes_table,
        std::unique_ptr<TLocationStorage> storage_pos) {
    if (!storage_pos) {
        return std::unique_ptr<UpdateLocationHandler>{static_cast<UpdateLocationHandler*>(new DatabaseLocationHandler(nodes_table, untagged_nodes_table))};
    }
    return std::unique_ptr<UpdateLocationHandler>(static_cast<UpdateLocationHandler*>(new FileBasedLocationHandler<TLocationStorage>(std::move(storage_pos))));
}



#endif /* UPDATE_LOCATION_HANDLER_FACTORY_HPP_ */
