/*
 * expire_tiles.cpp
 *
 *  Created on: 08.09.2016
 *      Author: michael
 *
 *  This file contains code from osm2pgsql/expire_tiles.cpp
 */

#include <osmium/geom/util.hpp>
#include "expire_tiles.hpp"


ExpireTiles::ExpireTiles(Config& config) :
        m_config(config) {}

void ExpireTiles::coords_to_tile(double *tilex, double *tiley,
                                  double lon, double lat, int map_width) {
    latlon2merc(&lat, &lon);

    *tilex = map_width * (0.5 + lon / EARTH_CIRCUMFERENCE);
    *tiley = map_width * (0.5 - lat / EARTH_CIRCUMFERENCE);
}

void ExpireTiles::latlon2merc(double *lat, double *lon) {
    if (*lat > 85.07)
        *lat = 85.07;
    else if (*lat < -85.07)
        *lat = -85.07;

    *lon = (*lon) * EARTH_CIRCUMFERENCE / 360.0;
    *lat = log(tan(osmium::geom::PI/4.0 + osmium::geom::deg_to_rad(*lat) / 2.0)) * EARTH_CIRCUMFERENCE/(osmium::geom::PI*2);
}

int ExpireTiles::normalise_tile_x_coord(int x) {
    x %= map_width;
    if (x < 0) x = (map_width - x) + 1;
    return x;
}

void ExpireTiles::expire_from_point(osmium::Location location) {
    expire_from_point(location.lon(), location.lat());
}
