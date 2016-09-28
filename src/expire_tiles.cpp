/*
 * expire_tiles.cpp
 *
 *  Created on: 08.09.2016
 *      Author: michael
 *
 *  This file contains code from osm2pgsql/expire_tiles.cpp
 */

#include <osmium/geom/util.hpp>
#include <geos/geom/Geometry.h>
#include "expire_tiles.hpp"

TileCoordinate::TileCoordinate(double x, double y) : x(x), y(y) {}

void TileCoordinate::swap(TileCoordinate& other) {
    double temp = other.x;
    other.x = x;
    x = temp;
    temp = other.y;
    other.y = y;
    y = temp;
}


ExpireTiles::ExpireTiles(Config& config) :
        m_config(config) {}

void ExpireTiles::coords_to_tile(double *tilex, double *tiley,
                                  double lon, double lat, int map_width) {
    latlon2merc(&lat, &lon);

    *tilex = map_width * (0.5 + lon / EARTH_CIRCUMFERENCE);
    *tiley = map_width * (0.5 - lat / EARTH_CIRCUMFERENCE);
}

TileCoordinate ExpireTiles::coords_to_tile(double lon, double lat, int map_width) {
    if (lat > 85.07) {
        lat = 85.07;
    } else if (lat < -85.07) {
        lat = -85.07;
    }
    double merc_x = lon * EARTH_CIRCUMFERENCE / 360.0;
    double merc_y = log(tan(osmium::geom::PI/4.0 + osmium::geom::deg_to_rad(lat) / 2.0)) * EARTH_CIRCUMFERENCE/(osmium::geom::PI*2);
    TileCoordinate tile (map_width * (0.5 + merc_x / EARTH_CIRCUMFERENCE),  map_width * (0.5 - merc_y / EARTH_CIRCUMFERENCE));
    return tile;
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

void ExpireTiles::expire_from_geos_linestring(std::unique_ptr<geos::geom::Geometry> geom_ptr) {
    if (geom_ptr->getGeometryTypeId() == geos::geom::GeometryTypeId::GEOS_LINESTRING) {
        std::unique_ptr<geos::geom::CoordinateSequence> coords (geom_ptr->getCoordinates());
        expire_from_coord_sequence(coords.get());
    }
    //TODO else case
}
