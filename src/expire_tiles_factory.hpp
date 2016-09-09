/*
 * expire_tiles_factory.hpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#ifndef EXPIRE_TILES_FACTORY_HPP_
#define EXPIRE_TILES_FACTORY_HPP_

#include "expire_tiles_classic.hpp"
#include "expire_tiles_quadtree.hpp"
#include "expire_tiles_dummy.hpp"

/**
 * Factory creating layers inherited from OutputLayer.
 */

class ExpireTilesFactory {
public:
    ExpireTiles* create_expire_tiles(Config& config);

private:
    ExpireTilesClassic* create_classic(Config& config);
    ExpireTilesQuadtree* create_qt(Config& config);
    ExpireTilesDummy* create_dummy(Config& config);
};


#endif /* EXPIRE_TILES_FACTORY_HPP_ */
