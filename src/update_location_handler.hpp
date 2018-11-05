/*
 * update_location_handler.hpp
 *
 *  Created on:  2018-10-31
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef UPDATE_LOCATION_HANDLER_HPP_
#define UPDATE_LOCATION_HANDLER_HPP_

#include <memory>
#include <osmium/handler.hpp>


class UpdateLocationHandler : public osmium::handler::Handler {
public:
    virtual ~UpdateLocationHandler() {
    }

    virtual void ignore_errors() = 0;

    /**
     * Store the location of the node in the storage.
     */
    virtual void node(const osmium::Node& node) = 0;

    /**
     * Retrieve location of a node by its ID from the persisent node cache (database or file).
     *
     * This does not perform a lookup to the temporary cache (in memory cache if the implementation uses one).
     * Some implementations might call get_node_location if get_node_location_from_persisent is called.
     */
    virtual osmium::Location get_node_location_from_persisent(const osmium::object_id_type id) const = 0;

    /**
     * Get location of node with given id.
     */
    virtual osmium::Location get_node_location(const osmium::object_id_type id) const = 0;

    /**
     * Retrieve locations of all nodes in the way from storage and add
     * them to the way object.
     */
    virtual void way(osmium::Way& way) = 0;
};

#endif /* UPDATE_LOCATION_HANDLER_HPP_ */
