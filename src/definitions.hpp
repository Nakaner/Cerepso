/*
 * definitions.hpp
 *
 *  Created on:  2018-08-23
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef DEFINITIONS_HPP_
#define DEFINITIONS_HPP_

#include <osmium/index/map.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

#endif /* DEFINITIONS_HPP_ */
