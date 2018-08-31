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
#include <osmium/geom/wkb.hpp>
#include <memory>
#include "postgres_table.hpp"
#include "associated_street_relation_manager.hpp"

#ifndef POSTGRES_HANDLER_HPP_
#define POSTGRES_HANDLER_HPP_

class PostgresHandler : public osmium::handler::Handler {
public:
    /**
     * Get tags of a associatedStreet relation the given object belongs to.
     *
     * \returns pointer to TagList or nullptr if no relation refers this object
     */
    const osmium::TagList* get_relation_tags_to_apply(const osmium::object_id_type id,
            const osmium::item_type type);

    static void add_username(std::string& ss, const char* username);

    static void add_uid(std::string& ss, const osmium::user_id_type uid);

    static void add_version(std::string& ss, const osmium::object_version_type version);

    static void add_timestamp(std::string& ss, const osmium::Timestamp& timestamp);

    static void add_changeset(std::string& ss, const osmium::changeset_id_type changeset);

    /**
     * \brief Add a TAB at the end of the string.
     *
     * \param ss String where the TAB should be appended.
     */
    static void add_separator_to_stringstream(std::string& ss);

    static void add_way_nodes(const osmium::WayNodeList& nodes, std::string& query);

    static void add_member_ids(const osmium::RelationMemberList& members, std::string& query);

    static void add_member_roles(const osmium::RelationMemberList& members, std::string& query);

    static void add_member_types(const osmium::RelationMemberList& members, std::string& query);

    static std::string prepare_query(const osmium::OSMObject& object, PostgresTable& table,
            CerepsoConfig& config, const osmium::TagList* rel_tags_to_apply);

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
    /// reference to relation manager for associatedStreet relations
    AssociatedStreetRelationManager* m_assoc_manager;

    PostgresHandler(CerepsoConfig& config, PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
            AssociatedStreetRelationManager* assoc_manager = nullptr, PostgresTable* areas_table = nullptr) :
            m_config(config),
            m_nodes_table(nodes_table),
            m_untagged_nodes_table(untagged_nodes_table),
            m_ways_linear_table(ways_table),
            m_areas_table(areas_table),
            m_assoc_manager(assoc_manager)  {}


    /**
     * \brief constructor for testing purposes, will not establish database connections
     */
    PostgresHandler(PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table, CerepsoConfig& config,
            AssociatedStreetRelationManager* assoc_manager = nullptr, PostgresTable* areas_table = nullptr)  :
            m_config(config),
            m_nodes_table(nodes_table),
            m_untagged_nodes_table(untagged_nodes_table),
            m_ways_linear_table(ways_table),
            m_areas_table(areas_table),
            m_assoc_manager(assoc_manager)  {}

    virtual ~PostgresHandler()  {}


    /**
     * \osmiumcallback
     */
    void handle_node(const osmium::Node& node);

    static bool fill_field(const osmium::OSMObject& object, postgres_drivers::ColumnsConstIterator it,
            std::string& query, bool column_added, std::vector<const char*>& written_keys, PostgresTable& table,
            const osmium::TagList* rel_tags_to_apply);
};



#endif /* POSTGRES_HANDLER_HPP_ */
