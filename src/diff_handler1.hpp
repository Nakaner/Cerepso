/*
 * diff_handler1.hpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#ifndef APPEND_HANDLER_HPP_
#define APPEND_HANDLER_HPP_
#include <geos/geom/GeometryFactory.h>
#include "postgres_handler.hpp"
#include "expire_tiles.hpp"
#include "definitions.hpp"

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
    PostgresTable& m_relations_table;

    ExpireTiles* m_expire_tiles;

    index_type& m_location_index;

    geos::geom::GeometryFactory m_geom_factory;

public:
    DiffHandler1(CerepsoConfig& config, PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            PostgresTable& relations_table, PostgresTable& node_ways_table, ExpireTiles* expire_tiles, index_type& location_index) :
            PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table, nullptr, nullptr, &node_ways_table),
            m_relations_table(relations_table),
            m_expire_tiles(expire_tiles),
            m_location_index(location_index) { }

    /**
     * \brief Constructor for testing purposes, will not establish database connections.
     */
    DiffHandler1(PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            PostgresTable& relations_table, CerepsoConfig& config, PostgresTable& node_ways_table, ExpireTiles* expire_tiles,
            index_type& location_index) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config, nullptr, nullptr, &node_ways_table),
        m_relations_table(relations_table),
        m_expire_tiles(expire_tiles),
        m_location_index(location_index) { }

    ~DiffHandler1() {};


    void node(const osmium::Node& node);


    void way(const osmium::Way& way);

    void relation(const osmium::Relation& area);

    /// Handler not used but has to implemented because it is a full virtual method.
    void area(const osmium::Area& area) {};
};



#endif /* APPEND_HANDLER_HPP_ */
