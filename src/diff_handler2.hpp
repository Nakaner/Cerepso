/*
 * diff_handler2.hpp
 *
 *  Created on: 04.10.2016
 *      Author: michael
 */

#ifndef DIFF_HANDLER2_HPP_
#define DIFF_HANDLER2_HPP_

#include <geos/geom/GeometryFactory.h>
#include "postgres_handler.hpp"
#include "expire_tiles.hpp"

enum class TypeProgress {POINT, WAY, RELATION};

/**
 * Diff handler for table imported using MyHandler class.
 *
 * This handler deletes all objects with version > 1 from the tables. Another handler will import them again.
 */

class DiffHandler2 : public PostgresHandler {
private:
    /**
     * additional table for relations which is not inherited from PostgresHandler
     */
    Table& m_relations_table;

    ExpireTiles* m_expire_tiles;

    geos::geom::GeometryFactory m_geom_factory;

    /**
     * Track progress of import.
     *
     * Nodes have to be imported before ways, otherwise we will not find some v1 nodes which are needed by "their" ways.
     */
    TypeProgress m_progress = TypeProgress::POINT;

    /**
     * Search in both table holding the nodes for the node in question and return it as instance of geos::geom::Coordinate.
     *
     * This method looks both into nodes and untagged_nodes table.
     *
     * @param id OSM ID of the node
     *
     * @throws std::runtime_error if it is not found in any table
     *
     * @returns unique_ptr to the Coordinate instance
     */
    std::unique_ptr<const geos::geom::Coordinate> get_point_from_tables(osmium::object_id_type id);

    /**
     * write all nodes which have to be written to the database
     */
    void write_new_nodes();

    /**
     * write all ways which have to be written to the database
     */
    void write_new_ways();

public:
    DiffHandler2(Config& config, Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Table& relations_table,
            ExpireTiles* expire_tiles) :
            PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table),
            m_relations_table(relations_table),
            m_expire_tiles(expire_tiles) {
        m_untagged_nodes_table.start_copy();
        m_nodes_table.start_copy();
    }

    /**
     * constructor for testing purposes, will not establish database connections
     */
    DiffHandler2(Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Table& relations_table, Config& config,
            ExpireTiles* expire_tiles) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config),
        m_relations_table(relations_table),
        m_expire_tiles(expire_tiles) { }

    ~DiffHandler2();

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void insert_way(const osmium::Way& way, std::string& copy_buffer);

    void insert_relation(const osmium::Relation& relation);

    void relation(const osmium::Relation& area);

    void area(const osmium::Area& area) {};
};

#endif /* DIFF_HANDLER2_HPP_ */
