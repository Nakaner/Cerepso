/*
 * file_based_location_handler.hpp
 *
 *  Created on:  2018-10-31
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef FILE_BASED_LOCATION_HANDLER_HPP_
#define FILE_BASED_LOCATION_HANDLER_HPP_

#include <memory>
#include <osmium/handler/node_locations_for_ways.hpp>

template <class TLocationStorage>
class FileBasedLocationHandler : public UpdateLocationHandler {
    osmium::handler::NodeLocationsForWays<TLocationStorage> m_location_handler;
    // We have to have the unique_ptr as a member to prevent that it goes out of scope.
    std::unique_ptr<TLocationStorage> m_storage_pos;
    bool m_second_pass;

public:
    FileBasedLocationHandler(std::unique_ptr<TLocationStorage> storage_pos) :
        // m_location_handler must be initialised before ownership of storage_pos is moved to m_storage_pos
        m_location_handler(*storage_pos),
        m_storage_pos(std::move(storage_pos)),
        m_second_pass(false) {
    }

    void ignore_errors() {
        m_location_handler.ignore_errors();
    }

    void node(const osmium::Node& node) {
        m_location_handler.node(node);
    }

    osmium::Location get_node_location_from_persisent(const osmium::object_id_type id) const {
        return get_node_location(id);
    }

    osmium::Location get_node_location(const osmium::object_id_type id) const {
        return m_location_handler.get_node_location(id);
    }

    void way(osmium::Way& way) {
        m_location_handler.way(way);
    }
};



#endif /* FILE_BASED_LOCATION_HANDLER_HPP_ */
