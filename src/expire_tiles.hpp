/*
 * expire_tiles.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#include "columns.hpp"
#include <osmium/osm/location.hpp>
#include <geos/geom/Geometry.h>
#include <geos/geom/CoordinateArraySequence.h>

#ifndef EXPIRE_TILES_HPP_
#define EXPIRE_TILES_HPP_

/**
 * \brief x and y coordinate (you could also call it "address") of a tile
 *
 * The origin of this coordinate system is in the upper left corner, y axis points to the south!
 */
class TileCoordinate {
public:
    double x;
    double y;

    TileCoordinate(double x, double y);

    /**
     * \brief swap two TileCoordinates
     *
     * This method copies the x and y coordinate of the other TileCoordinate to this TileCoordinate and vice versa.
     *
     * \param other other TileCoordinate
     */
    void swap(TileCoordinate& other);
};

/**
 * \brief Abstract class for creating tile expiry lists. Implementations are in classed derived from this class.
 */
class ExpireTiles {
public:
    ExpireTiles(Config& config);

    ExpireTiles() = delete;

    virtual ~ExpireTiles() {}

    /**
     * \brief Expire tiles at the requested zoom levels at this position.
     *
     * This method is a wrapper around ExpireTiles::expire_from_point(const double, const double).
     *
     * \param location where to expire
     */
    void expire_from_point(osmium::Location location);

    /**
     * \brief Expire tiles at the requested zoom levels at this position.
     *
     * \param lon longitude
     * \param lat latitude
     */
    virtual void expire_from_point(const double lon, const double lat) = 0;

    /**
     * \brief Expire all tiles which intersected a line.
     *
     * Implementation may vary much between the subclasses. While some implementations may expire all tiles
     * intersected by the line, other implementations may only expire only the tiles where the points of the line are.
     *
     * \param coords pointer to the geos::geom::CoordinateSequence which represents the line
     */
    virtual void expire_from_coord_sequence(const geos::geom::CoordinateSequence* coords) = 0;

    /**
     * Expire all tiles crossed by the line (given as WKB hex string).
     *
     * \param geom_ptr pointer to the linestring, it must be a geos::geom::LineString*
     */
    void expire_from_geos_linestring(geos::geom::Geometry* geom_ptr);

    /**
     * \brief Output the list of expired tiles to a file.
     *
     * This method writes the expiry log formatted like osm2pgsql's log to the file specified by
     * member of \link Config#m_expire_tiles m_config::m_expire_tiles \endlink.
     */
    virtual void output_and_destroy() = 0;

protected:
    Config& m_config;

    /// number of tiles in x direction to cover the world (1 at zoom 0, 2 at zoom 1, …)
    int map_width;

    /// earth circumfence for calculation of tile IDs
    const double EARTH_CIRCUMFERENCE = 40075016.68;

    /// How many tiles worth of space to leave either side of a changed feature
    const double TILE_EXPIRY_LEEWAY = 0.1;

    /// \todo documentation, rename arguments
    void coords_to_tile(double *tilex, double *tiley, double lon, double lat, int map_width);

    /**
     * \brief Transform a pair of longitude, latitude (WGS84) and map width to tile numbers
     *
     * \param lon longitude
     * \param lat latitude
     * \param map_width number of tiles in x direction (square root of the total number of tiles at this zoom level),
     * i.e. map_width = 2^zoom
     *
     * \returns coordinate pair as instance of TileCoordinate. x and y are inside  [0; map_width-1[
     *
     * \todo make arguments const
     */
    TileCoordinate coords_to_tile(double lon, double lat, int map_width);

    /**
     * \brief Convert WGS84 geographic coordinates to Web Mercator coordinates.
     *
     * This method modifies the arguments.
     *
     * \param lat pointer to latitude to be converted
     * \param lon pointer to longitude to be converted
     */
    void latlon2merc(double *lat, double *lon);

    int normalise_tile_x_coord(int x);

    /**
     * \brief Mark the tile as expired.
     *
     * \param x x index
     * \param y y index
     */
    virtual void expire_tile(int x, int y) = 0;
};



#endif /* EXPIRE_TILES_HPP_ */
