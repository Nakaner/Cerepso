/*
 * expire_tiles_quadtree.cpp
 *
 *  Created on: 08.09.2016
 *      Author: michael
 *
 *  This file contains code from osm2pgsql/expire_tiles.cpp
 */

#include "expire_tiles_quadtree.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <assert.h>
#include <set>

void ExpireTilesQuadtree::expire_from_point(double lon, double lat) {
    // convert latlon into Mercator coordinates and these into tile coordinates
    TileCoordinate tc = coords_to_tile(lon, lat, map_width);
    // we currently only expire the tile not its neighbours if the point is near the edge of the tile
    int norm_x = normalise_tile_x_coord(static_cast<int>(tc.x));
    expire_tile(norm_x, static_cast<int>(tc.y));
}

double ExpireTilesQuadtree::segment_length(double lon1, double lat1, double lon2, double lat2) {
    double degrad = 3.14159265358979323846/180; // conversion to radiant
    return acos(sin(lat1 * degrad) * sin(lat2 * degrad) + cos(lat1 * degrad) * cos(lat2 * degrad) * cos((lon2  - lon1) * degrad)) * 6371000;
}

void ExpireTilesQuadtree::expire_line_segment_180secure(double lon1, double lat1, double lon2, double lat2) {
    // convert latlon into Mercator coordinates and these into tile coordinates
    TileCoordinate tc1 = coords_to_tile(lon1, lat1, map_width);
    TileCoordinate tc2 = coords_to_tile(lon2, lat2, map_width);
    // swap ends of this segment if necessary because we go from left to right
    if (tc1.x > tc2.x) {
        tc1.swap(tc2);
    }
    if (tc2.x-tc1.x > map_width/2) { // line crosses 180th meridian → split the line at its intersection with this meridian
        // x-coordinate of intersection point: map_width/2
        double y_split; // y-coordinate of intersection point
        if (tc2.x == map_width && tc1.x == 0) {
            // The line is part of the 180th meridian. We have to treat this in a special way, otherwise there will
            // be a division by 0 in the following code.
            expire_line_segment(0, tc1.y, 0, tc2.y);
            return;
        }
        // This line runs from western to eastern hemisphere over the 180th meridian
        // use intercept theorem to get the intersection point of the line and the 180th meridian
        double x_distance = map_width + tc1.x - tc2.x; // x-distance between left point and 180th meridian
        // apply intercept theorem: (y2-y1)/(y_split-y1) = (x2-x1)/(x_split-x1)
        y_split = tc1.y + (tc2.y - tc1.y) * (tc1.x / x_distance);
        expire_line_segment(0, y_split, tc1.x, tc1.y);
        expire_line_segment(tc2.x, tc2.y, map_width, y_split);
    } else {
        expire_line_segment(tc1.x, tc1.y, tc2.x, tc2.y);
    }
}

void ExpireTilesQuadtree::expire_from_coord_sequence(const geos::geom::CoordinateSequence* coords) {
    // check if distance between beginning and end is too large
    // We don't calculate the exact distance by summing up all segments.
    // This might return wrong results for U-shaped lines but it's faster.
    //TODO check if exact calculation based on shere is too slow.
    //HARDCODED 20 km
    if (segment_length(coords->getX(0), coords->getY(0), coords->getX(coords->getSize()-1), coords->getY(coords->getSize()-1)) > 20000) {
        // If the line crosses 180th meridian, following expression will return false.
        if (std::abs(coords->getX(0) - coords->getX(coords->getSize()-1)) > 1) {
            // expire only the tiles where the nodes are, not all tiles intersected by the line
            for (size_t i=0; i <= coords->getSize()-1; i++) {
                expire_from_point(coords->getX(i), coords->getY(i));
            }
        }
    }
    for (size_t i=0; i <= coords->getSize()-2; i++) {
        expire_line_segment_180secure(coords->getX(i), coords->getY(i), coords->getX(i+1), coords->getY(i+1));
    }
}

void ExpireTilesQuadtree::expire_line_segment(double x1, double y1, double x2, double y2) {
    assert(x1 <= x2);
    assert(x2-x1 <= map_width/2);
    if (x1 == x2 && y1 == y2) {
        // The line is degenerated and only a point.
        return;
    }
    // The following if block ensures that x2-x1 does not cause an underfolow which could cause a division by zero.
    if (x2-x1 < 1) { // the extend of the bounding box of the line in x-direction is small
        if ((static_cast<int>(x2) == static_cast<int>(x1)) || (x2-x1 < 0.00000001)) {
            /**
             * Case 1: The linestring is parallel to a meridian or does not cross a tile border.
             * Therefore we can treat it as a vertical linestring.
             *
             * Case 2: This linestring is almost parallel (very small error). We just treat it as a parallel of a meridian.
             * The resulting error is negligible. If we expired the wrong tile, it would not matter because all tiles
             * overlap a little bit, i.e. the changed line will still be processed.
             */
            if (y2 < y1) {
                // swap coordinates
                double temp = y2;
                y2 = y1;
                y1 = temp;
            }
            expire_vertical_line(x1, y1, y2);
            return;
        }
    }
    // y(x) = m * x + c with incline as m and y_intercept as c
    double incline = (y2-y1)/(x2-x1);
    double y_intercept = y2 - incline * x2;

    // mark start tile as expired
    expire_tile(static_cast<int>(x1), static_cast<int>(y1));
    // expire all tiles the line enters by crossing their western edge
    for (int x = static_cast<int>(x1 + 1); x <= static_cast<int>(x2); x++) {
        expire_tile(x, static_cast<int>(incline * x + y_intercept));
    }
    // the same for all tiles which are entered by crossing their southern edge
    expire_tile(static_cast<int>(x1), static_cast<int>(y1));
    for (int y = static_cast<int>(y1 + 1); y <= static_cast<int>(y2); y++) {
        expire_tile(static_cast<int>((y - y_intercept)/incline), y);
    }
}

void ExpireTilesQuadtree::expire_vertical_line(double x, double y1, double y2) {
    assert(y1 <= y2); // line in correct order and not collapsed
    // mark the tile of the southern end as expired
    expire_tile(static_cast<int>(x), static_cast<int>(y1));
    // mark all tiles above it as expired until we reach the northern end of the line
    for (int y = static_cast<int>(y1 + 1); y <= static_cast<int>(y2); y++) {
        expire_tile(static_cast<int>(x), y);
    }
}

void ExpireTilesQuadtree::expire_bbox(double x1, double y1, double x2, double y2) {
    // checks order and ensures that the box is not collapsed
    assert(x1 < x2);
    assert(y1 < y2);
    for (int x = static_cast<int>(x1); x <= static_cast<int>(x2); x++) {
        for (int y = static_cast<int>(y1); y <= static_cast<int>(y2); y++) {
            expire_tile(x, y);
        }
    }
}

void ExpireTilesQuadtree::expire_tile(int x, int y) {
    if (m_dirty_tiles.find(xy_to_quadtree(x, y, m_config.m_max_zoom)) == m_dirty_tiles.end()) {
        m_dirty_tiles.insert(std::make_pair<int, bool>(xy_to_quadtree(x, y, m_config.m_max_zoom), true));
    }
}

xy_coord_t ExpireTilesQuadtree::quadtree_to_xy(int qt_coord, int zoom) {
    xy_coord_t result;
    for (int z = zoom; z > 0; z -= 1) {
        int next_zoom = z - 1;
        result.y = result.y + ((qt_coord  & (1 << (z+next_zoom))) >> z);
        result.x = result.x + ((qt_coord  & (1 << (2*next_zoom))) >> next_zoom);
    }
    return result;
}

int ExpireTilesQuadtree::xy_to_quadtree(int x, int y, int zoom) {
    int qt = 0;
    // the two highest bits are the bits of zoom level 1, the third and fourth bit are level 2, …
    for (int z = 0; z < zoom; z++) {
        qt = qt + ((x & (1 << z)) << z);
        qt = qt + ((y & (1 << z)) << (z+1));
    }
    return qt;
}

int ExpireTilesQuadtree::quadtree_upscale(int qt_old, int zoom_steps, int offset /* = 0 */) {
    qt_old = qt_old << (2 * zoom_steps);
    // check validity of arguments
    assert(offset < pow(2, 2*zoom_steps)); // offset <= 3 if zoom_steps = 1, offset <= 15 if zoom_steps=2, …
    assert(offset >= 0);
    return qt_old + offset;
}

void ExpireTilesQuadtree::output_and_destroy() {
    FILE *outfile (fopen(m_config.m_expire_tiles.c_str(), "a"));
    if (outfile == nullptr) {
        fprintf(stderr, "Failed to open expired tiles file (%s).  Tile expiry list will not be written!\n", strerror(errno));
        return;
    }
    std::set<int64_t> expired_tiles;
    // iterate over all expired tiles
    for (std::map<int, bool>::iterator it = m_dirty_tiles.begin(); it != m_dirty_tiles.end(); it++) {
        // loop over all requested zoom levels (from maximum to minimum zoom level)
        for (int dz = 0; dz <= m_config.m_max_zoom - m_config.m_min_zoom; dz++) {
            int64_t qt_new = it->first >> (dz*2);
            if (expired_tiles.insert(qt_new).second) {
                // expired_tiles.insert(qt_new).second is true if the tile has not been written to the list yet
                xy_coord_t xy = quadtree_to_xy(qt_new, m_config.m_max_zoom-dz);
                fprintf(outfile, "%i/%i/%i\n", m_config.m_max_zoom-dz, xy.x, xy.y);
            }
        }
    }
    if (outfile) {
        fclose(outfile);
    }
}
