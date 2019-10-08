/*
 * \file postgres_handler.hpp Header file for PostgresHandler class
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

//#include <libpq-fe.h>
#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/geom/factory.hpp>
#include <wkbhpp/wkbwriter.hpp>
#include <memory>
#include "postgres_table.hpp"
#include "associated_street_relation_manager.hpp"

#ifndef POSTGRES_HANDLER_HPP_
#define POSTGRES_HANDLER_HPP_

class PostgresHandler : public osmium::handler::Handler {

    /**
     * Prepare lines for SQL COPY to be written into the nodes_relations and ways_relations tables.
     *
     * \param relation relation to be processed
     * \param type type of interest (node, way or relation)
     *
     * \returns string (multiple lines) to be written to the database or an empty string if there was no
     * member of the type of interest.
     */
    static std::string prepare_relation_member_list_query(const osmium::Relation& relation, const osmium::item_type type);

public:
    /**
     * Get tags of a associatedStreet relation the given object belongs to.
     *
     * \returns pointer to TagList or nullptr if no relation refers this object
     */
    const osmium::TagList* get_relation_tags_to_apply(const osmium::object_id_type id,
            const osmium::item_type type);

    static void add_osm_id(std::string& ss, const osmium::object_id_type id);

    static void add_int32(std::string& ss, const int32_t number);

    static void add_int16(std::string& ss, const int16_t number);

    static void add_username(std::string& ss, const char* username);

    static void add_uid(std::string& ss, const osmium::user_id_type uid);

    static void add_version(std::string& ss, const osmium::object_version_type version);

    static void add_timestamp(std::string& ss, const osmium::Timestamp& timestamp);

    static void add_changeset(std::string& ss, const osmium::changeset_id_type changeset);

    static void add_way_nodes(const osmium::WayNodeList& nodes, std::string& query);

    static void add_member_ids(const osmium::RelationMemberList& members, std::string& query);

    static void add_member_roles(const osmium::RelationMemberList& members, std::string& query);

    static void add_member_types(const osmium::RelationMemberList& members, std::string& query);

    static void add_geometry(const osmium::OSMObject& object, std::string& query, PostgresTable& table);

    static std::string prepare_query(const osmium::OSMObject& object, PostgresTable& table,
            CerepsoConfig& config, const osmium::TagList* rel_tags_to_apply);

    static std::string prepare_node_way_query(const osmium::Way& way);

    static std::string prepare_node_relation_query(const osmium::Relation& relation);

    static std::string prepare_way_relation_query(const osmium::Relation& relation);

    /**
     * \brief Build the line which will be inserted via `COPY` into the database.
     *
     * \param relation Reference to the relation object
     * \param query Reference to the string where the line should be appended.
     *              It is possible that one string contains multiple lines (e.g. for buffered writing).
     * \param multipoint_wkb string containing the value of the geom_points column as WKB HEX string
     * \param multilinestring_wkb string containing the value of the geom_lines column as WKB HEX string
     * \param config reference to program configuration
     * \param table table to write to
     */
    static void prepare_relation_query(const osmium::Relation& relation, std::string& query, std::stringstream& multipoint_wkb,
            std::stringstream& multilinestring_wkb, CerepsoConfig& config, PostgresTable& table);

    /**
     * \brief Node handler has derived from osmium::handler::Handler.
     *
     * This method processes a node from the input buffer.
     * \virtual
     *
     * \param node Node to be processed.
     */
    virtual void node(const osmium::Node& node) = 0;

    /**
     * \brief Way handler has derived from osmium::handler::Handler.
     *
     * This method processes a way from the input buffer.
     * \virtual
     *
     * \param way Way to be processed.
     */
    virtual void way(const osmium::Way& way) = 0;

    virtual void area(const osmium::Area& area) = 0;

protected:
    CerepsoConfig& m_config;
    /// reference to nodes table
    PostgresTable& m_nodes_table;
    /// reference to table of untagged nodes
    PostgresTable* m_untagged_nodes_table;
    /// reference to table of all ways
    PostgresTable& m_ways_linear_table;
    /// reference to table of polkygons
    PostgresTable* m_areas_table;
    /// pointer to table containing mapping of nodes to the ways using them
    PostgresTable* m_node_ways_table;
    /// pointer to table containing mapping of nodes to the relations using them
    PostgresTable* m_node_relations_table;
    /// pointer to table containing mapping of ways to the relations using them
    PostgresTable* m_way_relations_table;
    /// reference to relation manager for associatedStreet relations
    AssociatedStreetRelationManager* m_assoc_manager;


    PostgresHandler(CerepsoConfig& config, PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            AssociatedStreetRelationManager* assoc_manager = nullptr, PostgresTable* areas_table = nullptr,
            PostgresTable* node_ways_table = nullptr, PostgresTable* node_relations_table = nullptr,
            PostgresTable* way_relations_table = nullptr) :
            m_config(config),
            m_nodes_table(nodes_table),
            m_untagged_nodes_table(untagged_nodes_table),
            m_ways_linear_table(ways_table),
            m_areas_table(areas_table),
            m_node_ways_table(node_ways_table),
            m_node_relations_table(node_relations_table),
            m_way_relations_table(way_relations_table),
            m_assoc_manager(assoc_manager) {}


    /**
     * \brief constructor for testing purposes, will not establish database connections
     */
    PostgresHandler(PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table, CerepsoConfig& config,
            AssociatedStreetRelationManager* assoc_manager = nullptr, PostgresTable* areas_table = nullptr,
            PostgresTable* node_ways_table = nullptr, PostgresTable* node_relations_table = nullptr,
            PostgresTable* way_relations_table = nullptr)  :
            m_config(config),
            m_nodes_table(nodes_table),
            m_untagged_nodes_table(untagged_nodes_table),
            m_ways_linear_table(ways_table),
            m_areas_table(areas_table),
            m_node_ways_table(node_ways_table),
            m_node_relations_table(node_relations_table),
            m_way_relations_table(way_relations_table),
            m_assoc_manager(assoc_manager)  {}

    virtual ~PostgresHandler()  {}


    /**
     * \osmiumcallback
     */
    void handle_node(const osmium::Node& node);

    void handle_area(const osmium::Area& area);

    static bool fill_field(const osmium::OSMObject& object, postgres_drivers::ColumnsConstIterator it,
            std::string& query, bool column_added, std::vector<const char*>& written_keys, PostgresTable& table,
            const osmium::TagList* rel_tags_to_apply);
};



#endif /* POSTGRES_HANDLER_HPP_ */
