/*
 * expire_tiles_quadtree.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#include <map>
#include "expire_tiles.hpp"

#ifndef EXPIRE_TILES_QUADTREE_HPP_
#define EXPIRE_TILES_QUADTREE_HPP_

struct xy_coord_t {
    int x = 0;
    int y = 0;
    xy_coord_t(int x_coord, int y_coord) : x(x_coord), y(y_coord) {}
    xy_coord_t() : xy_coord_t(0, 0) {}
};

class ExpireTilesQuadtree : public ExpireTiles {
private:
    std::map<int, bool> m_dirty_tiles;
    std::unique_ptr<std::ostream> m_outstream;

    void expire_tile(int x, int y);

    /**
     * \brief Expire all tiles "used" by a vertical line.
     *
     * All parameters have to be Web Mercator tile numbers, i.e. (15.5, 56.5) is the middle of tile (15, 56).
     *
     * \param x x-coordinate
     * \param y1 y-coordinate of southern point
     * \param y2 y-coordinate of northern point
     *
     * \todo make arguments const
     */
    void expire_vertical_line(double x, double y1, double y2);

    /**
     * \brief Expire all tiles covered by this bounding box.
     *
     * All parameters have to be Web Mercator tile numbers, i.e. (15.5, 56.5) is the middle of tile (15, 56).
     *
     * \param x1 x-coordinate of south-west corner
     * \param y1 y-coordinate of south-west corner
     * \param x2 x-coordinate of north-east corner
     * \param y2 y-coordinate of north-east corner
     *
     * \todo make arguments const
     */
    void expire_bbox(double x1, double y1, double x2, double y2);

    /**
     * \brief Get length on sphere of a segment.
     *
     * Arguments have to be geographic WGS84 coordinates.
     *
     * \param lon1 longitude of starting point
     * \param lat1 latitude of starting point
     * \param lon2 longitude of end point
     * \param lat2 latitude of end point
     *
     * \returns length in metre
     */
    double segment_length(double lon1, double lat1, double lon2, double lat2);

public:
    /**
     * \brief Expire all tiles covered by this line segment.
     *
     * All parameters have to be tile IDs, i.e. (15.5, 56.5) is the middle of tile (15, 56).
     * The line must not cross crosses the international date line (lon=±180°).
     * Begin and end of the line segment must not be swapped because this method does not check
     * the order of the vertices beforehand.
     *
     * \param x1 x coordinate of start node
     * \param y1 y coordinate of start node
     * \param x2 x coordinate of end node
     * \param y2 x coordinate of end node
     */
    void expire_line_segment(double x1, double y1, double x2, double y2);

    /**
     * \brief Convert a quadtree coordinate into a tile ID (x and y) using bitshifts.
     *
     * Quadtree coordinates are interleaved this way: YXYX…
     *
     * \param qt_coord quadtree ID
     * \param zoom zoom level
     */
    xy_coord_t quadtree_to_xy(int qt_coord, int zoom);


    /**
     * \brief Convert a tile ID (x and y) into quadtree coordinate using bitshifts.
     *
     * Quadtree coordinates are interleaved this way: YXYX…
     *
     * \param x x index
     * \param y y index
     * \param zoom zoom level
     * \returns quadtree ID as integer
     */
    int xy_to_quadtree(int x, int y, int zoom);

    /**
     * \brief Get quadtree coordinate of one zoomlevel higher
     *
     * Example: 0b01 is (1,0) at zoom level 1. This method would return 0b0100 for zoom level 2 (3,0).
     * If you want another subtile, use offset. offset=1 for (4,0), offset=2 for (3,1), offset=3 for (4,1).
     *
     * \param qt_old old quadtree ID
     * \param zoom_steps number of zoom levels to scale up
     * \param offset offset from upper left subtile. Must be within following interval: [0, 2^(2*zoom_steps)-1]
     * \return new quadtree ID
     */
    int quadtree_upscale(int qt_old, int zoom_steps, int offset = 0);

    ExpireTilesQuadtree(Config& config) : ExpireTiles(config) {
        if (config.m_min_zoom < 0) {
            return;
        }
        map_width = 1 << config.m_max_zoom;}

    ExpireTilesQuadtree() = delete;

    ~ExpireTilesQuadtree() {}

    void expire_from_point(double lon, double lat);

    /**
     * Expire all tiles crossed by the line (given as geos::geom::CoordinateSequence).
     *
     * \param coords CoordinateSequence with all vertices of the line. Coordinates are expected as
     * longitude and latitude (WGS84/EPSG:4326).
     */
    void expire_from_coord_sequence(const geos::geom::CoordinateSequence* coords);

    /**
     * Expire all tiles crossed by this line segment. This method has special rules if the
     * line segment crosses the 180th meridian.
     *
     * \param lon1 longitude of the beginning
     * \param lat1 latitude of the beginning
     * \param lon2 longitude of the end
     * \param lat2 latitude of the end
     */
    void expire_line_segment_180secure(double lon1, double lat1, double lon2, double lat2);

    void output_and_destroy();
};


#endif /* EXPIRE_TILES_QUADTREE_HPP_ */
