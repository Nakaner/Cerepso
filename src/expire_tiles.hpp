/*
 * expire_tiles.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#include "columns.hpp"
#include <osmium/osm/location.hpp>
#include <geos/geom/CoordinateArraySequence.h>

#ifndef EXPIRE_TILES_HPP_
#define EXPIRE_TILES_HPP_

/**
 * @brief Abstract class for creating tile expiry lists. Implementations are in classed derived from this class.
 */
class ExpireTiles {
public:
    ExpireTiles(Config& config);

    ExpireTiles() = delete;

    virtual ~ExpireTiles() {}

    void expire_from_point(osmium::Location location);

    virtual void expire_from_point(const double lon, const double lat) = 0;

    virtual void expire_from_coord_sequence(geos::geom::CoordinateSequence* coords) = 0;

    // output the list of expired tiles to a file.s
    virtual void output_and_destroy() = 0;

protected:
    Config& m_config;

    int map_width; // number of tiles in x direction to cover the world (1 at zoom 0, 2 at zoom 1, …)

    const double EARTH_CIRCUMFERENCE = 40075016.68;
    const double TILE_EXPIRY_LEEWAY = 0.1; // How many tiles worth of space to leave either side of a changed feature

    void coords_to_tile(double *tilex, double *tiley, double lon, double lat, int map_width);

    void latlon2merc(double *lat, double *lon);

    int normalise_tile_x_coord(int x);

    virtual void expire_tile(int x, int y) = 0;
};



#endif /* EXPIRE_TILES_HPP_ */
