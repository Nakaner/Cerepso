/*
 * way_nodes_table.hpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef TABLE_WAY_NODES_TABLE_HPP_
#define TABLE_WAY_NODES_TABLE_HPP_

#include <osmium/osm/node_ref.hpp>
#include "postgres_table.hpp"

/**
 * ID of a member node of a way and its position in the WayNodeList
 */
struct MemberNode {
    osmium::NodeRef node_ref;
    int position;

    MemberNode(osmium::object_id_type nr, int pos) :
        node_ref(nr),
        position(pos) {
    }

    MemberNode() = delete;

    bool operator<(const MemberNode other) {
        return position < other.position;
    }
};

class WayNodesTable : public PostgresTable {
protected:
    /**
     * Register all the prepared statements we need.
     */
    void request_prepared_statements();

public:
    WayNodesTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns);

    WayNodesTable() = delete;

    ~WayNodesTable();

    /**
     * \brief delete node list of a way
     *
     * This method executes the prepared statement `delete_way_node_list`.
     *
     * \param id way ID
     *
     * \throws std::runtime_error
     */
    void delete_way_node_list(const osmium::object_id_type id);

    /**
     * \brief Get IDs of ways referencing a node.
     *
     * \param node_id OSM node ID
     * \throws std::runtime_error if query execution fails
     * \returns vector of way/relation IDs or empty vector if none was found
     */
    std::vector<osmium::object_id_type> get_way_ids(const osmium::object_id_type node_id);

    /**
     * \brief Get member nodes of a way.
     *
     * \param way_id OSM way ID
     * \throws std::runtime_error if query execution fails
     * \returns vector of node IDs sorted by position or empty vector if none was found
     */
    //TODO check if MemberNode is really necessary or a vector of osmium::object_id_type is sufficient
    std::vector<MemberNode> get_way_nodes(const osmium::object_id_type way_id);
};



#endif /* TABLE_WAY_NODES_TABLE_HPP_ */
