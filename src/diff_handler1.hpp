/*
 * diff_handler1.hpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#ifndef APPEND_HANDLER_HPP_
#define APPEND_HANDLER_HPP_
#include "postgres_handler.hpp"
#include "tables/relations_table.hpp"
#include "expire_tiles.hpp"
#include "geos_compatibility_definitions.hpp"
#include "definitions.hpp"
#include "update_location_handler.hpp"

/**
 * \brief Diff handler for table imported using ImportHandler class, pass 1.
 *
 * This handler deletes all objects with version > 1 from the tables. Another handler will import them again in a second pass.
 */

class DiffHandler1 : public PostgresHandler {
private:

    /**
     * \brief additional table for relations which is not inherited from PostgresHandler
     */
    RelationsTable& m_relations_table;

    ExpireTiles* m_expire_tiles;

    UpdateLocationHandler& m_location_index;

    geos_factory_type m_geom_factory;

public:
    DiffHandler1(CerepsoConfig& config, FeaturesTable& nodes_table, NodeLocationsTable* untagged_nodes_table, FeaturesTable& ways_table,
            RelationsTable& relations_table, WayNodesTable& node_ways_table, RelationMembersTable& node_relations_table,
            RelationMembersTable& way_relations_table, RelationMembersTable& relation_relations_table,
            ExpireTiles* expire_tiles, UpdateLocationHandler& location_index,
            FeaturesTable* areas_table = nullptr) :
        PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table, nullptr, areas_table, &node_ways_table,
            &node_relations_table, &way_relations_table, &relation_relations_table),
        m_relations_table(relations_table),
        m_expire_tiles(expire_tiles),
        m_location_index(location_index),
#ifdef GEOS_36
        m_geom_factory(geos::geom::GeometryFactory::create().release(), GEOSGeometryFactoryDeleter())
#else
        m_geom_factory(new geos::geom::GeometryFactory{})
#endif
        { }

    /**
     * \brief Constructor for testing purposes, will not establish database connections.
     */
    DiffHandler1(FeaturesTable& nodes_table, NodeLocationsTable* untagged_nodes_table, FeaturesTable& ways_table,
            RelationsTable& relations_table, CerepsoConfig& config, WayNodesTable& node_ways_table, RelationMembersTable& node_relations_table,
            RelationMembersTable& way_relations_table, RelationMembersTable& relation_relations_table,
            ExpireTiles* expire_tiles, UpdateLocationHandler& location_index,
            FeaturesTable* areas_table = nullptr) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config, nullptr, areas_table, &node_ways_table,
            &node_relations_table, &way_relations_table, &relation_relations_table),
        m_relations_table(relations_table),
        m_expire_tiles(expire_tiles),
        m_location_index(location_index),
#ifdef GEOS_36
        m_geom_factory(geos::geom::GeometryFactory::create().release(), GEOSGeometryFactoryDeleter())
#else
        m_geom_factory(new geos::geom::GeometryFactory{})
#endif
    { }

    ~DiffHandler1() {};


    /**
     * Expire tiles at the old location of the node if there is any. Delete node from database.
     *
     * This method does not use the location of the node. Instead the location is looked up
     * in the persisent location cache.
     */
    void node(const osmium::Node& node);

    /**
     * Expire tiles along the old geometry of the way and delete it from database.
     *
     * This method does not need valid locations on the node references. Instead the locations are
     * looked up in the persisent location cache.
     */
    void way(const osmium::Way& way);

    void relation(const osmium::Relation& area);

    /// Handler not used but has to implemented because it is a full virtual method.
    void area(const osmium::Area& area);
};



#endif /* APPEND_HANDLER_HPP_ */
