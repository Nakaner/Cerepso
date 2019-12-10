/*
 * expire_tiles_factory.cpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#include "expire_tiles_factory.hpp"

ExpireTiles* ExpireTilesFactory::create_expire_tiles(CerepsoConfig& config) {
    if (!config.m_expire_tiles.empty()) {
        return this->create_qt(config);
    }
    return this->create_dummy(config);
}

ExpireTilesQuadtree* ExpireTilesFactory::create_qt(CerepsoConfig& config) {
    return new ExpireTilesQuadtree(config);
}

ExpireTilesDummy* ExpireTilesFactory::create_dummy(CerepsoConfig& config) {
    return new ExpireTilesDummy(config);
}


