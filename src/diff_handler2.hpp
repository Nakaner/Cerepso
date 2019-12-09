/*
 * diff_handler2.hpp
 *
 *  Created on: 04.10.2016
 *      Author: michael
 */

#ifndef DIFF_HANDLER2_HPP_
#define DIFF_HANDLER2_HPP_

#include <functional>
#include <geos/geom/GeometryFactory.h>
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include "postgres_handler.hpp"
#include "geos_compatibility_definitions.hpp"
#include "expire_tiles.hpp"
#include "definitions.hpp"
#include "update_location_handler.hpp"

enum class TypeProgress : char {
    POINT = 1,
    WAY = 2,
    RELATION = 3
};

/**
 * \brief Handler to import diff files (second pass) into tables imported using MyHandler.
 *
 * This handler imports all objects with version > 1 into the database. It should be called after DiffHandler1
 */

class DiffHandler2 : public PostgresHandler {

    /**
     * additional table for relations which is not inherited from PostgresHandler
     */
    PostgresTable& m_relations_table;

    UpdateLocationHandler& m_location_index;

    /**
     * pointer to the used implementation of ExpireTiles
     */
    ExpireTiles* m_expire_tiles;

    /**
     * list of ways which have to be updated because one of their nodes has been moved
     */
    std::vector<osmium::object_id_type> m_pending_ways;

    /**
     * list of relations which have to be updated because one of their member nodes or ways has been moved
     */
    std::vector<osmium::object_id_type> m_pending_relations;

    /**
     * Iterator pointing to element in list of pending ways which has been worked on.
     */
    std::vector<osmium::object_id_type>::size_type m_pending_ways_idx;

    /**
     * Iterator pointing to element in list of pending relations which has been worked on.
     */
    std::vector<osmium::object_id_type>::size_type m_pending_relations_idx;

    osmium::area::MultipolygonManager<osmium::area::Assembler>* m_mp_manager;

    osmium::memory::Buffer m_relation_buffer;

    osmium::memory::CallbackBuffer m_new_areas_buffer;

    /// Buffer for assembled areas which required an update due to changes to their members
    osmium::memory::CallbackBuffer m_updated_areas_buffer;

#ifdef GEOS_36
    geos_factory_type m_geom_factory;
#else
    geos_factory_type m_geom_factory;
#endif

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

    /**
     * Update geometry of an area.
     *
     * \param id OSM way ID
     */
    void update_area_geometry(const osmium::Area& area);

    /**
     * Update multigeometry of the points and lines referenced by a relation.
     *
     * \param id OSM relation ID
     */
    void update_relation(const osmium::object_id_type id);

    /**
     * Build an Osmium Area object for a relation whose members changed.
     *
     * \param id relation ID
     * \param members ID, type and rank of the members. The method will set the ID of a
     * way member in the vector to 0 if there are no nodes available for it.
     */
    void update_multipolygon_geometry(const osmium::object_id_type id,
            std::vector<postgres_drivers::MemberIdTypePos>& members);

    /**
     * Sort a container of osmium::object_id_type, remove duplicates and then call a function for each non-zero element.
     *
     * \param container container to work on
     * \param func function to call
     */
    void clean_up_container_and_work_on(std::vector<osmium::object_id_type>& container, std::function<void(osmium::object_id_type)> func);

public:
    DiffHandler2(CerepsoConfig& config, PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            PostgresTable& relations_table, PostgresTable& node_ways_table, PostgresTable& node_relations_table,
            PostgresTable& way_relations_table, PostgresTable& relation_relations_table,
            ExpireTiles* expire_tiles, UpdateLocationHandler& location_index,
            PostgresTable* areas_table = nullptr,
            osmium::area::MultipolygonManager<osmium::area::Assembler>* mp_manager = nullptr);

    /**
     * \brief constructor for testing purposes, will not establish database connections
     */
    DiffHandler2(PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            PostgresTable& relations_table, PostgresTable& node_ways_table, PostgresTable& node_relations_table,
            PostgresTable& way_relations_table, PostgresTable& relation_relations_table,
            CerepsoConfig& config, ExpireTiles* expire_tiles,
            UpdateLocationHandler& location_index, PostgresTable* areas_table = nullptr,
            osmium::area::MultipolygonManager<osmium::area::Assembler>* mp_manager = nullptr);

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
    void area(const osmium::Area& area);

    void incomplete_relation(const osmium::relations::RelationHandle& rel_handle);

    void incomplete_relation(const osmium::Relation& relation);

    /**
     * Flush area output buffers.
     *
     * Call this method at the end of processing.
     */
    void flush();

    /**
     * Update all relations whose node or way members have changed without any direct changes to the relation.
     */
    void after_relations();
};

#endif /* DIFF_HANDLER2_HPP_ */
