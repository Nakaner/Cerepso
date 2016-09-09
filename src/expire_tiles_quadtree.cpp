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

void ExpireTilesQuadtree::expire_from_point(double lon, double lat) {
    double  tmp_x;
    double  tmp_y;
//    int     min_tile_x;
//    int     min_tile_y;
    int     norm_x;
    /* Convert Mercator coordinates into tile coordinates */
    coords_to_tile(&tmp_x, &tmp_y, lon, lat, map_width);
    // commented out because we currently only expire the tile not its neighbours if the point is near the edge of the tile
//    min_tile_x = tmp_x - TILE_EXPIRY_LEEWAY;
//    min_tile_y = tmp_y - TILE_EXPIRY_LEEWAY;
//    if (min_tile_x < 0) min_tile_x = 0;
//    if (min_tile_y < 0) min_tile_y = 0;
//    norm_x =  normalise_tile_x_coord(min_tile_x);
    norm_x = normalise_tile_x_coord(static_cast<int>(tmp_x));
    expire_tile(norm_x, static_cast<int>(tmp_y));
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
    for (int z = 0; z < zoom; z++) {
        qt = qt + ((x & (1 << z)) << z);
        qt = qt + ((y & (1 << z)) << (z+1));
    }
    return qt;
}

int ExpireTilesQuadtree::quadtree_upscale(int qt_old, int zoom_steps, int offset /* = 0 */) {
    qt_old = qt_old << (2 * zoom_steps);
    assert(offset < pow(2, 2*zoom_steps));
    assert(offset >= 0);
    return qt_old + offset;
}

void ExpireTilesQuadtree::output_and_destroy() {
    FILE *outfile (fopen(m_config.m_expire_tiles.c_str(), "a"));
    if (outfile == nullptr) {
        fprintf(stderr, "Failed to open expired tiles file (%s).  Tile expiry list will not be written!\n", strerror(errno));
        return;
    }
    // iterate over all expired tiles
    for (std::map<int, bool>::iterator it = m_dirty_tiles.begin(); it != m_dirty_tiles.end(); it++) {
        // loop over all requested zoom levels (from maximum to minimum zoom level)
        for (int dz = 0; dz <= m_config.m_max_zoom - m_config.m_min_zoom; dz++) {
            int64_t qt_new = it->first >> (dz*2);
            xy_coord_t xy = quadtree_to_xy(qt_new, m_config.m_max_zoom-dz);
            fprintf(outfile, "%i/%i/%i\n", m_config.m_max_zoom-dz, xy.x, xy.y);
        }
    }
    if (outfile) {
        fclose(outfile);
    }
}
