/*
 * relation_collector.hpp
 *
 *  Created on: 01.07.2016
 *      Author: michael
 */

#ifndef RELATION_COLLECTOR_HPP_
#define RELATION_COLLECTOR_HPP_

#include <geos/io/WKBWriter.h>
#include <osmium_geos_factory/geos_factory.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/relations/relations_manager.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/relations/detail/member_meta.hpp>
#include "geos_compatibility_definitions.hpp"
#include "import_handler.hpp"
#include "postgres_table.hpp"

/**
 * \brief The RelationCollector collects all relations.
 *
 * It is used during the first pass of Cerepso
 * to read all relations, their tags and reference from the input file. At the second pass it collects
 * all ways and nodes which are referenced by the collected relations.
 */
class RelationCollector: public osmium::relations::RelationsManager<RelationCollector,
    true, true, false> {

    typedef typename osmium::relations::RelationsManager<RelationCollector, true, true, true> collector_type;

private:
    /// reference to program configuration
    CerepsoConfig& m_config;
    osmium::memory::Buffer m_output_buffer;

    /// \brief database connection for the relations table
    PostgresTable m_database_table;
    osmium_geos_factory::GEOSFactory<> m_geos_factory;
    geos_factory_type m_geos_geom_factory;
    geos::io::WKBWriter m_geos_wkb_writer;
    static constexpr size_t initial_output_buffer_size = 1024 * 1024;
    static constexpr size_t max_buffer_size_for_flush = 100 * 1024;

    /** \brief Helper method. Returns true if the member will likely miss in the data source file.
     *
     *  If the member has an offset of 0 in the buffer and the relation is not contained
     *  completely in the data source file, the member will likely not be contained in the
     *  data source file.
     *
     *  If the member has an offset of 0 in the buffer and the relation is contained
     *  completely in the data source file, the member will be contained in the data
     *  source file.
     *
     *  \todo implement; necessary to support partial relations of extracts
     */
    bool member_missing(const osmium::RelationMember& member, bool complete);

    /**
     * \brief process a relation
     *
     * This method will insert the relation into the database
     *
     * \param relation reference to Relation object
     */
    void process_relation(const osmium::Relation& relation);

    /**
     * \brief Build the query string which will be inserted into the database using SQL COPY.
     *
     * \param relation reference to Relation object
     * \param query reference to a string which will be inserted into the database
     *
     * \throws osmium::geometry_error
     */
    void build_relation_query(const osmium::Relation& relation, std::string& query);

public:

    RelationCollector() = delete;
    explicit RelationCollector(CerepsoConfig& config, postgres_drivers::Columns& node_columns);

    /**
     * \brief This method decides which relations we're interested in, and
     * instructs Osmium to collect their members for us.
     *
     * This method is derived from osmium::handler::Handler.
     *
     * \return always `true` because we do not filter out any relations
     */
    bool new_relation(const osmium::Relation& relation) const;

    /**
     * \brief Tells Osmium which members to keep for a relation of interest.
     *
     * This method is derived from osmium::handler::Handler.
     *
     * \return always `true` because we do not filter out any members
     */
    bool new_member(const osmium::Relation& /*relation*/, const osmium::RelationMember& /*member*/, std::size_t /*n*/) noexcept;

    /**
     * \brief Called by Osmium when a relation has been fully read (i.e. all
     * members are present)
     *
     * This method is required by Osmium.
     */
    void complete_relation(const osmium::Relation& relation);

    /**
     * \brief Process incomplete relations (some members missing).
     *
     * Called after all relations have been read, to processs those that
     * were not fully read. We cannot assume that all members will be present
     * for all relations (happens if we read an extract instead of the planet dump).
     *
     * This method is required by Osmium.
     *
     * \todo implement; necessary to support partial relations of extracts
     * \todo change to new RelationManager
     */
    void handle_incomplete_relations();
};



#endif /* RELATION_COLLECTOR_HPP_ */
