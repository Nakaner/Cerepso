/*
 * database_location_handler.hpp
 *
 *  Created on:  2018-10-29
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef DATABASE_LOCATION_HANDLER_HPP_
#define DATABASE_LOCATION_HANDLER_HPP_

#include <memory>
#include <unordered_map>
#include "postgres_table.hpp"
#include "update_location_handler.hpp"

class DatabaseLocationHandler :  public UpdateLocationHandler {
    PostgresTable& m_nodes_table;

    PostgresTable& m_untagged_nodes_table;

    bool m_ignore_errors;

    std::unordered_map<osmium::object_id_type, osmium::Location> m_location_cache;

public:
    DatabaseLocationHandler(PostgresTable& nodes_table, PostgresTable& untagged_nodes_table);

    DatabaseLocationHandler() = delete;

    void ignore_errors();

    /**
     * Store the location of the node in the storage.
     */
    void node(const osmium::Node& node);

    /**
     * Retrieve location of a node by its ID from the database.
     *
     * This does not perform a lookup to the temporary cache.
     *
     * \returns location of the node or an invalid location if it was not found.
     */
    osmium::Location get_node_location_from_persisent(const osmium::object_id_type id) const;

    /**
     * Get location of node with given id.
     */
    osmium::Location get_node_location(const osmium::object_id_type id) const;

    /**
     * Retrieve locations of all nodes in the way from storage and add
     * them to the way object.
     */
    void way(osmium::Way& way);
};



#endif /* DATABASE_LOCATION_HANDLER_HPP_ */
