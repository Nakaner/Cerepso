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
#include "definitions.hpp"

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
    PostgresTable& m_relations_table;

    index_type& m_location_index;

    /**
     * pointer to the used implementation of ExpireTiles
     */
    ExpireTiles* m_expire_tiles;

    /**
     * list of ways which have to be updated because one of their nodes has been moved
     */
    std::vector<osmium::object_id_type> m_pending_ways;

    /**
     * Iterator pointing to element in list of pending ways which has been worked on.
     */
    std::vector<osmium::object_id_type>::iterator m_pending_ways_it;

    geos::geom::GeometryFactory m_geom_factory;

    /**
     * \brief Track progress of import.
     *
     * Nodes have to be imported before ways, otherwise we will not find some v1 nodes which are needed by "their" ways.
     */
    TypeProgress m_progress = TypeProgress::POINT;

    /**
     * \brief Search the location index for the location of a node.
     *
     * This method does not database queries.
     *
     * \param id OSM ID of the node
     * \returns osmium::Location (invalid if none was found in the index)
     */
    osmium::Location get_point_from_tables(osmium::object_id_type id);

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

    /**
     * Update geometry of a way.
     *
     * \param id OSM way ID
     */
    void update_way(const osmium::object_id_type id);

public:
    DiffHandler2(CerepsoConfig& config, PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            PostgresTable& relations_table, PostgresTable& node_ways_table, ExpireTiles* expire_tiles, index_type& location_index) :
            PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table, nullptr, nullptr, &node_ways_table),
            m_relations_table(relations_table),
            m_location_index(location_index),
            m_expire_tiles(expire_tiles),
            m_pending_ways() {
        m_pending_ways_it = m_pending_ways.begin();
        m_untagged_nodes_table->start_copy();
        m_nodes_table.start_copy();
    }

    /**
     * \brief constructor for testing purposes, will not establish database connections
     */
    DiffHandler2(PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            PostgresTable& relations_table, PostgresTable& node_ways_table, CerepsoConfig& config, ExpireTiles* expire_tiles,
            index_type& location_index) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config, nullptr, nullptr, &node_ways_table),
        m_relations_table(relations_table),
        m_location_index(location_index),
        m_expire_tiles(expire_tiles) { }

    ~DiffHandler2();

    friend void end_copy_nodes_tables(DiffHandler2&);

    friend void end_copy_ways_tables(DiffHandler2&);

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    /**
     * \brief Append a line to a given string for insertion of relation into its table via COPY.
     *
     * This method has been extracted out of relation(const osmium::Relation& relation) to be easier to test.
     *
     * \param relation reference to the relation object
     * \param copy_buffer string whose content will be inserted into the database
     */
    void insert_relation(const osmium::Relation& relation, std::string& copy_buffer);

    /**
     * Update all ways whose node locations have changed without any direct changes to the way.
     */
    void work_on_pending_ways();

    void relation(const osmium::Relation& area);

    /// Handler not used but has to implemented because it is a full virtual method.
    void area(const osmium::Area& area) {};
};

#endif /* DIFF_HANDLER2_HPP_ */
