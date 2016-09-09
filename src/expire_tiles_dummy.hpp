/*
 * dummy_expire.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#ifndef DUMMY_EXPIRE_HPP_
#define DUMMY_EXPIRE_HPP_

/**
 * @brief Dummy class for creating tile expiry lists. Used if no expiry list should be created.
 * All public methods just do nothing.
 * This class implements the Null Object Pattern.
 */
class ExpireTilesDummy : public ExpireTiles {
public:
    ExpireTilesDummy(Config& config) : ExpireTiles(config) {}

    ExpireTilesDummy() = delete;

    ~ExpireTilesDummy() {}

    void expire_from_point(osmium::Location location) {}

    void expire_from_point(double lon, double lat) {}

    void expire_from_coord_sequence(geos::geom::CoordinateSequence* coords) {}

    // output the list of expired tiles to a file.s
    void output_and_destroy() {}

    void expire_tile(int x, int y) {}
};



#endif /* DUMMY_EXPIRE_HPP_ */