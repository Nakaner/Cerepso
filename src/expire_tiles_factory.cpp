/*
 * expire_tiles_factory.cpp
 *
 *  Created on: 06.09.2016
 *      Author: michael
 */

#include "expire_tiles_factory.hpp"

ExpireTiles* ExpireTilesFactory::create_expire_tiles(CerepsoConfig& config) {
    if (config.m_expiry_type == "classic") {
        return this->create_classic(config);
    }
    else if (config.m_expiry_type == "quadtree") {
        return this->create_qt(config);
    }
    return this->create_dummy(config);
}

ExpireTilesClassic* ExpireTilesFactory::create_classic(CerepsoConfig& config) {
    return new ExpireTilesClassic(config);
}

ExpireTilesQuadtree* ExpireTilesFactory::create_qt(CerepsoConfig& config) {
    return new ExpireTilesQuadtree(config);
}

ExpireTilesDummy* ExpireTilesFactory::create_dummy(CerepsoConfig& config) {
    return new ExpireTilesDummy(config);
}


