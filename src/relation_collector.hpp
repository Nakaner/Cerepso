/*
 * relation_collector.hpp
 *
 *  Created on: 01.07.2016
 *      Author: michael
 */

#ifndef RELATION_COLLECTOR_HPP_
#define RELATION_COLLECTOR_HPP_

#include <osmium/geom/geos.hpp>
#include <geos/io/WKBWriter.h>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/relations/collector.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/relations/detail/member_meta.hpp>
#include "columns.hpp"
#include "table.hpp"
#include "myhandler.hpp"

/**
 * The RelationCollector collects all relations which are not covered by MultipolygonCollector.
 * It writes ge
 */
class RelationCollector: public osmium::relations::Collector<RelationCollector,
    true, true, false> {

    typedef typename osmium::relations::Collector<RelationCollector, true, true, true> collector_type;

private:

    osmium::memory::Buffer m_output_buffer;
    Table m_database_table;
    osmium::geom::GEOSFactory<> m_geos_factory;
    geos::geom::GeometryFactory m_geos_geom_factory;
    geos::io::WKBWriter m_geos_wkb_writer;
    static constexpr size_t initial_output_buffer_size = 1024 * 1024;
    static constexpr size_t max_buffer_size_for_flush = 100 * 1024;

    /** Helper to retrieve relation member */
    osmium::Node& get_member_node(size_t offset) const;

    /** Helper to retrieve relation member */
    osmium::Way& get_member_way(size_t offset) const ;

    /** Helper method. Returns true if the member will likely miss in the data source file.
     *
     *  If the member has an offset of 0 in the buffer and the relation is not contained
     *  completely in the data source file, the member will likely not be contained in the
     *  data source file.
     *
     *  If the member has an offset of 0 in the buffer and the relation is contained
     *  completely in the data source file, the member will be contained in the data
     *  source file.
     */
    bool member_missing(const osmium::RelationMember& member, bool complete);

public:

    RelationCollector() = delete;
    explicit RelationCollector(Config& config, Columns& node_columns);

    /**
     * This method decides which relations we're interested in, and
     * instructs Osmium to collect their members for us.
     *
     * @return opposite of MultipolygonCollector::keep_relation()
     */
    bool keep_relation(const osmium::Relation& relation) const;
    /**
     * Tells Osmium which members to keep for a relation of interest.
     */
    bool keep_member(const osmium::relations::RelationMeta& relation_meta, const osmium::RelationMember& member) const;
    /**
     * Called by Osmium when a relation has been fully read (i.e. all
     * members are present)
     */
    void complete_relation(osmium::relations::RelationMeta& relation_meta);
    /**
     * Called after all relations have been read, to processs those that
     * were not fully read. We cannot assume that all members will be present
     * for all relations.
     */
    void handle_incomplete_relations();

};



#endif /* RELATION_COLLECTOR_HPP_ */
