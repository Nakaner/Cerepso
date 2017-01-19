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
 * \brief Factory for an implementation of ExpireTiles
 */

class ExpireTilesFactory {
public:
    /**
     * \brief Return the implementation you request.
     *
     * Following implementations are available:
     * * `classic`, very similar to osm2pgsql
     * * `quadtree`, new implementation which also supports relations and expires the tiles of all way and
     *   node members if something has changed of the relation
     * * `dummy` does not create an expiry lis, use it if don't need such a list
     *
     * \param config program configuration, Config::m_expiry_type is important
     */
    ExpireTiles* create_expire_tiles(CerepsoConfig& config);

private:
    ExpireTilesClassic* create_classic(CerepsoConfig& config);
    ExpireTilesQuadtree* create_qt(CerepsoConfig& config);
    ExpireTilesDummy* create_dummy(CerepsoConfig& config);
};


#endif /* EXPIRE_TILES_FACTORY_HPP_ */
