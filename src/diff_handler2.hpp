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
 * \brief Handler to import diff files (second pass) into tables imported using MyHandler.
 *
 * This handler imports all objects with version > 1 into the database. It should be called after DiffHandler1
 */

class DiffHandler2 : public PostgresHandler {
private:
    /**
     * additional table for relations which is not inherited from PostgresHandler
     */
    Table& m_relations_table;

    /**
     * pointer to the used implementation of ExpireTiles
     */
    ExpireTiles* m_expire_tiles;

    geos::geom::GeometryFactory m_geom_factory;

    /**
     * \brief Track progress of import.
     *
     * Nodes have to be imported before ways, otherwise we will not find some v1 nodes which are needed by "their" ways.
     */
    TypeProgress m_progress = TypeProgress::POINT;

    /**
     * \brief Search in both table holding the nodes for the node in question and return it as instance of geos::geom::Coordinate.
     *
     * This method looks both into nodes and untagged_nodes table.
     *
     * \param id OSM ID of the node
     * \throws std::runtime_error if it is not found in any table
     * \returns unique_ptr to the Coordinate instance
     */
    std::unique_ptr<const geos::geom::Coordinate> get_point_from_tables(osmium::object_id_type id);

    /**
     * \brief Write all nodes which have to be written to the database.
     *
     * This method terminates COPY mode of the nodes' and untagged nodes' tables. It starts COPY mode for the ways' table.
     */
    void write_new_nodes();

    /**
     * \brief Write all ways which have to be written to the database.
     *
     * This method terminates COPY mode of the ways' table. It starts COPY mode for the relations' table.
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
     * \brief constructor for testing purposes, will not establish database connections
     */
    DiffHandler2(Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Table& relations_table, Config& config,
            ExpireTiles* expire_tiles) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config),
        m_relations_table(relations_table),
        m_expire_tiles(expire_tiles) { }

    ~DiffHandler2();

    friend void end_copy_nodes_tables(DiffHandler2&);

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    /**
     * \brief Append a line to a given string for insertion of way into its table via COPY.
     *
     * This method has been extracted out of way(const osmium::Way& way) to be easier to test.
     */
    void insert_way(const osmium::Way& way, std::string& copy_buffer);

    /**
     * \brief Append a line to a given string for insertion of relation into its table via COPY.
     *
     * This method has been extracted out of relation(const osmium::Relation& relation) to be easier to test.
     */
    void insert_relation(const osmium::Relation& relation);

    void relation(const osmium::Relation& area);

    /// Handler not used but has to implemented because it is a full virtual method.
    void area(const osmium::Area& area) {};
};

#endif /* DIFF_HANDLER2_HPP_ */
