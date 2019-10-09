/*
 * dummy_expire.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#ifndef DUMMY_EXPIRE_HPP_
#define DUMMY_EXPIRE_HPP_

/**
 * \brief Dummy class for creating tile expiry lists. Used if no expiry list should be created.
 *
 * All public methods just do nothing.
 * This class implements the Null Object Pattern.
 */
class ExpireTilesDummy : public ExpireTiles {
public:
    ExpireTilesDummy(CerepsoConfig& config) : ExpireTiles(config) {}

    ExpireTilesDummy() = delete;

    ~ExpireTilesDummy() {}

    void expire_from_point(osmium::Location) {}

    void expire_from_point(double, double) {}

    void expire_from_coord_sequence(const geos::geom::CoordinateSequence*) {}

    void expire_from_coord_sequence(const osmium::NodeRefList&) {}

    void output_and_destroy() {}

    void expire_tile(int, int) {}
};



#endif /* DUMMY_EXPIRE_HPP_ */
