/*
 * import_handler.hpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#ifndef IMPORTHANDLER_HPP_
#define IMPORTHANDLER_HPP_

#include "postgres_handler.hpp"

/**
 * \brief Handler for nodes and ways to be imported into the database.
 *
 * To import relations into the database, use RelationCollector class.
 */

class ImportHandler : public PostgresHandler {
public:
    ImportHandler() = delete;

    /**
     * \brief constructor for normal usage
     *
     * This constructor takes references to the program configuration and the instances of Table class.
     */
    ImportHandler(CerepsoConfig& config,  FeaturesTable& nodes_table, NodeLocationsTable* untagged_nodes_table, FeaturesTable& ways_table,
            AssociatedStreetRelationManager* assoc_manager = nullptr, FeaturesTable* areas_table = nullptr,
            WayNodesTable* node_ways_table = nullptr, RelationMembersTable* node_relations_table = nullptr,
            RelationMembersTable* way_relations_table = nullptr, RelationMembersTable* relation_relations_table = nullptr) :
        PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table, assoc_manager, areas_table, node_ways_table,
                node_relations_table, way_relations_table, relation_relations_table) {
    }

    /**
     * \brief Constructor for testing purposes, will not establish database connections
     *
     * This constructor takes references to the program configuration and the instances of Table class.
     */
    ImportHandler(FeaturesTable& nodes_table, NodeLocationsTable* untagged_nodes_table, FeaturesTable& ways_table, CerepsoConfig& config,
            AssociatedStreetRelationManager* assoc_manager = nullptr, FeaturesTable* areas_table = nullptr,
            WayNodesTable* node_ways_table = nullptr, RelationMembersTable* node_relations_table = nullptr,
            RelationMembersTable* way_relations_table = nullptr, RelationMembersTable* relation_relations_table = nullptr) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config, assoc_manager, areas_table, node_ways_table,
                node_relations_table, way_relations_table, relation_relations_table) {
    }

    ~ImportHandler() {
    }

    /**
     * \osmiumcallback
     */
    void node(const osmium::Node& node);

    /**
     * \osmiumcallback
     */
    void way(const osmium::Way& way);

    /**
     * \osmiumcallback
     */
    void area(const osmium::Area& area);

    /**
     * \osmiumcallback
     */
    void relation(const osmium::Relation& relation);
};



#endif /* IMPORTHANDLER_HPP_ */
