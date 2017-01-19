/*
 * expire_tiles.cpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 *
 *  This file contains code from osm2pgsql/expire_tiles.cpp
 */

#include <string.h>
#include <osmium/geom/util.hpp>
#include "expire_tiles_classic.hpp"

ExpireTilesClassic::ExpireTilesClassic(CerepsoConfig& config) : ExpireTiles(config),
        m_dirty(nullptr) {
    if (config.m_min_zoom < 0) {
        return;
    }
    map_width = 1 << config.m_min_zoom;
    tile_width = EARTH_CIRCUMFERENCE / map_width;
}

namespace {
/*
 * We store the dirty tiles in an in-memory tree during runtime
 * and dump them out to a file at the end.  This allows us to easilly drop
 * duplicate tiles from the output.
 *
 * This data structure consists of a node, representing a tile at zoom level 0,
 * which contains 4 pointers to nodes representing each of the child tiles at
 * zoom level 1, and so on down the the zoom level specified in
 * Options->expire_tiles_zoom.
 *
 * The memory allowed to this structure is not capped, but daily deltas
 * generally produce a few hundred thousand expired tiles at zoom level 17,
 * which are easilly accommodated.
 */
    int calc_complete(struct ExpireTilesClassic::Tile * tile) {
        int c;

        c = tile->complete[0][0];
        c += tile->complete[0][1];
        c += tile->complete[1][0];
        c += tile->complete[1][1];
        return c;
    }

    void destroy_tree(struct ExpireTilesClassic::Tile * tree) {
        if (! tree) return;
        if (tree->subtiles[0][0]) destroy_tree(tree->subtiles[0][0]);
        if (tree->subtiles[0][1]) destroy_tree(tree->subtiles[0][1]);
        if (tree->subtiles[1][0]) destroy_tree(tree->subtiles[1][0]);
        if (tree->subtiles[1][1]) destroy_tree(tree->subtiles[1][1]);
        free(tree);
    }

    /*
     * Mark a tile as dirty.
     * Returns the number of subtiles which have all their children marked as dirty.
     */
    int _mark_tile(struct ExpireTilesClassic::Tile ** tree, int x, int y, int zoom, int this_zoom) {
        int zoom_diff = zoom - this_zoom - 1;
        int rel_x;
        int rel_y;
        int complete;

        if (! *tree) *tree = (struct ExpireTilesClassic::Tile *)calloc(1, sizeof(**tree));
        rel_x = (x >> zoom_diff) & 1;
        rel_y = (y >> zoom_diff) & 1;
        if (! (*tree)->complete[rel_x][rel_y]) {
            if (zoom_diff <= 0) {
                (*tree)->complete[rel_x][rel_y] = 1;
            } else {
                complete = _mark_tile(&((*tree)->subtiles[rel_x][rel_y]), x, y, zoom, this_zoom + 1);
                if (complete >= 4) {
                    (*tree)->complete[rel_x][rel_y] = 1;
                    /* We can destroy the subtree to save memory now all the children are dirty */
                    destroy_tree((*tree)->subtiles[rel_x][rel_y]);
                    (*tree)->subtiles[rel_x][rel_y] = nullptr;
                }
            }
        }
        return calc_complete(*tree);
    }

    /*
     * Mark a tile as dirty.
     * Returns the number of subtiles which have all their children marked as dirty.
     */
    int mark_tile(struct ExpireTilesClassic::Tile ** tree_head, int x, int y, int zoom) {
        return _mark_tile(tree_head, x, y, zoom, 0);
    }

    void output_dirty_tile_impl(FILE * outfile, int x, int y, int zoom, int min_zoom, int &outcount) {
        int y_min;
        int x_iter;
        int y_iter;
        int x_max;
        int y_max;
        int out_zoom;
        int zoom_diff;

        if (zoom > min_zoom) out_zoom = zoom;
        else out_zoom = min_zoom;
        zoom_diff = out_zoom - zoom;
        y_min = y << zoom_diff;
        x_max = (x + 1) << zoom_diff;
        y_max = (y + 1) << zoom_diff;
        for (x_iter = x << zoom_diff; x_iter < x_max; x_iter++) {
            for (y_iter = y_min; y_iter < y_max; y_iter++) {
                outcount++;
                if ((outcount <= 1) || ((outcount % 1000) == 0)) {
                    fprintf(stderr, "\rWriting dirty tile list (%iK)\n", outcount / 1000);
                    fflush(stderr);
                }
                fprintf(outfile, "%i/%i/%i\n", out_zoom, x_iter, y_iter);
            }
        }
    }

    struct tile_output_file : public ExpireTilesClassic::tile_output {
      tile_output_file(const std::string &expire_tiles_filename)
        : outcount(0)
        , outfile(fopen(expire_tiles_filename.c_str(), "a")) {
        if (outfile == nullptr) {
          fprintf(stderr, "Failed to open expired tiles file (%s).  Tile expiry list will not be written!\n", strerror(errno));
        }
      }

      virtual ~tile_output_file() {
        if (outfile) {
          fclose(outfile);
        }
      }

      virtual void output_dirty_tile(int x, int y, int zoom, int min_zoom) {
        output_dirty_tile_impl(outfile, x, y, zoom, min_zoom, outcount);
      }

    private:
      int outcount;
      FILE *outfile;
    };

    void _output_and_destroy_tree(ExpireTilesClassic::tile_output *output, struct ExpireTilesClassic::Tile * tree, int x, int y, int this_zoom, int min_zoom) {
        int sub_x = x << 1;
        int sub_y = y << 1;
        ExpireTilesClassic::tile_output *out;

        if (! tree) return;

        out = output;
        if ((tree->complete[0][0]) && output) {
            output->output_dirty_tile(sub_x + 0, sub_y + 0, this_zoom + 1, min_zoom);
            out = nullptr;
        }
        if (tree->subtiles[0][0]) _output_and_destroy_tree(out, tree->subtiles[0][0], sub_x + 0, sub_y + 0, this_zoom + 1, min_zoom);

        out = output;
        if ((tree->complete[0][1]) && output) {
            output->output_dirty_tile(sub_x + 0, sub_y + 1, this_zoom + 1, min_zoom);
            out = nullptr;
        }
        if (tree->subtiles[0][1]) _output_and_destroy_tree(out, tree->subtiles[0][1], sub_x + 0, sub_y + 1, this_zoom + 1, min_zoom);

        out = output;
        if ((tree->complete[1][0]) && output) {
            output->output_dirty_tile(sub_x + 1, sub_y + 0, this_zoom + 1, min_zoom);
            out = nullptr;
        }
        if (tree->subtiles[1][0]) _output_and_destroy_tree(out, tree->subtiles[1][0], sub_x + 1, sub_y + 0, this_zoom + 1, min_zoom);

        out = output;
        if ((tree->complete[1][1]) && output) {
            output->output_dirty_tile(sub_x + 1, sub_y + 1, this_zoom + 1, min_zoom);
            out = nullptr;
        }
        if (tree->subtiles[1][1]) _output_and_destroy_tree(out, tree->subtiles[1][1], sub_x + 1, sub_y + 1, this_zoom + 1, min_zoom);

        free(tree);
    }
} // namespace anonymous

void ExpireTilesClassic::output_and_destroy(tile_output *output) {
    _output_and_destroy_tree(output, m_dirty, 0, 0, 0, m_config.m_min_zoom);
    m_dirty = nullptr;
}

ExpireTilesClassic::~ExpireTilesClassic() {
  if (m_dirty != nullptr) {
    destroy_tree(m_dirty);
    m_dirty = nullptr;
  }
}

void ExpireTilesClassic::expire_tile(int x, int y) {
    mark_tile(&m_dirty, x, y, m_config.m_min_zoom);
}

void ExpireTilesClassic::expire_from_point(double lon, double lat) {
    double  tmp_x;
    double  tmp_y;
    int     min_tile_x;
    int     min_tile_y;
    int     norm_x;
    /* Convert the box's Mercator coordinates into tile coordinates */
    coords_to_tile(&tmp_x, &tmp_y, lon, lat, map_width);
    min_tile_x = tmp_x - TILE_EXPIRY_LEEWAY;
    min_tile_y = tmp_y - TILE_EXPIRY_LEEWAY;
    if (min_tile_x < 0) min_tile_x = 0;
    if (min_tile_y < 0) min_tile_y = 0;
    norm_x =  normalise_tile_x_coord(min_tile_x);
    expire_tile(norm_x, min_tile_y);
}

void ExpireTilesClassic::expire_from_coord_sequence(const geos::geom::CoordinateSequence* coords) {
    if (m_config.m_min_zoom < 0 || !coords || coords->getSize() == 0)
        return;

    if (coords->getSize() == 1) {
        expire_from_point(coords->getAt(0).x, coords->getAt(0).y);
    } else {
        for (size_t i = 1; i < coords->getSize(); ++i)
            from_line_segment(coords->getAt(i-1).x, coords->getAt(i-1).y, coords->getAt(i).x, coords->getAt(i).y);
    }
}

void ExpireTilesClassic::from_line_segment(double lon_a, double lat_a, double lon_b, double lat_b) {
    double  tile_x_a;
    double  tile_y_a;
    double  tile_x_b;
    double  tile_y_b;
    double  temp;
    double  x1;
    double  y1;
    double  x2;
    double  y2;
    double  hyp_len;
    double  x_len;
    double  y_len;
    double  x_step;
    double  y_step;
    double  step;
    double  next_step;
    int x;
    int y;
    int norm_x;

    coords_to_tile(&tile_x_a, &tile_y_a, lon_a, lat_a, map_width);
    coords_to_tile(&tile_x_b, &tile_y_b, lon_b, lat_b, map_width);

    if (tile_x_a > tile_x_b) {
        /* We always want the line to go from left to right - swap the ends if it doesn't */
        temp = tile_x_b;
        tile_x_b = tile_x_a;
        tile_x_a = temp;
        temp = tile_y_b;
        tile_y_b = tile_y_a;
        tile_y_a = temp;
    }

    x_len = tile_x_b - tile_x_a;
    if (x_len > map_width / 2) {
        /* If the line is wider than half the map, assume it
           crosses the international date line.
           These coordinates get normalised again later */
        tile_x_a += map_width;
        temp = tile_x_b;
        tile_x_b = tile_x_a;
        tile_x_a = temp;
        temp = tile_y_b;
        tile_y_b = tile_y_a;
        tile_y_a = temp;
    }
    y_len = tile_y_b - tile_y_a;
    hyp_len = sqrt(pow(x_len, 2) + pow(y_len, 2));  /* Pythagoras */
    x_step = x_len / hyp_len;
    y_step = y_len / hyp_len;

    for (step = 0; step <= hyp_len; step+= 0.4) {
        /* Interpolate points 1 tile width apart */
        next_step = step + 0.4;
        if (next_step > hyp_len) next_step = hyp_len;
        x1 = tile_x_a + ((double)step * x_step);
        y1 = tile_y_a + ((double)step * y_step);
        x2 = tile_x_a + ((double)next_step * x_step);
        y2 = tile_y_a + ((double)next_step * y_step);

        /* The line (x1,y1),(x2,y2) is up to 1 tile width long
           x1 will always be <= x2
           We could be smart and figure out the exact tiles intersected,
           but for simplicity, treat the coordinates as a bounding box
           and expire everything within that box. */
        if (y1 > y2) {
            temp = y2;
            y2 = y1;
            y1 = temp;
        }
        for (x = x1 - TILE_EXPIRY_LEEWAY; x <= x2 + TILE_EXPIRY_LEEWAY; x ++) {
            norm_x =  normalise_tile_x_coord(x);
            for (y = y1 - TILE_EXPIRY_LEEWAY; y <= y2 + TILE_EXPIRY_LEEWAY; y ++) {
                expire_tile(norm_x, y);
            }
        }
    }
}

void ExpireTilesClassic::output_and_destroy() {
  if (m_config.m_min_zoom >= 0) {
    tile_output_file output(m_config.m_expire_tiles);

    output_and_destroy(&output);
  }
}

