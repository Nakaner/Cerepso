/*
 * expire_tiles.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 *  Copied from osm2pgsql/expire_tiles.*
 */

#include <iostream>
#include <fstream>
#include "expire_tiles.hpp"

#ifndef EXPIRE_TILES_CLASSIC_HPP_
#define EXPIRE_TILES_CLASSIC_HPP_

/**
 * \brief This class implements the tile expiry algorithm of osm2pgsql.
 *
 * The code has only been slightly modified after being copied from osm2pgsql.
 * Therefore it has no unit tests and serves just backward compatability (with
 * all bugs if there are any ;-)).
 */
class ExpireTilesClassic : public ExpireTiles {
public:

    /**
     * \brief Node of the tree structure of tiles managed by ExpireTilesClassic
     */
    struct Tile {
        /**
         * This members stores the information wether there are subtiles. If an entry of the
         * array is 1, the subtile exists and can be accessed via
         * \link Tile#complete ExpireTilesClassic::Tile::complete \endlink.
         * If an entry is 0, there are no subtiles in the tree.
         */
        int complete[2][2];

        /**
         * pointers to the four subtiles of this tile
         */
        struct Tile* subtiles[2][2];
    };

    /* customisable tile output. this can be passed into the
     * `output_and_destroy` function to override output to a file.
     * this is primarily useful for testing.
     */
    struct tile_output {
        virtual ~tile_output() {}
        // dirty a tile at x, y & zoom, and all descendants of that
        // tile at the given zoom if zoom < min_zoom.
        virtual void output_dirty_tile(int x, int y, int zoom, int min_zoom) = 0;
    };

    ExpireTilesClassic(CerepsoConfig& config);

    ExpireTilesClassic() = delete;

    ~ExpireTilesClassic();

    void expire_from_point(double lon, double lat);

    void expire_from_coord_sequence(const geos::geom::CoordinateSequence* coords);

    /**
     * \brief Output the list of expired tiles to a file. Note that this consumes the list of expired tiles destructively.
     */
    void output_and_destroy();

private:
    /// dirty tile tree
    Tile* m_dirty;
    double tile_width;

    void from_line_segment(double lon_a, double lat_a, double lon_b, double lat_b);

    void expire_tile(int x, int y);

    /**
     * \brief Output the list of expired tiles using a `tile_output` functor. this consumes the list of expired tiles destructively.
     */
    void output_and_destroy(tile_output *output);
};



#endif /* EXPIRE_TILES_CLASSIC_HPP_ */
