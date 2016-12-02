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
    ImportHandler(Config& config,  Table& nodes_table, Table& untagged_nodes_table, Table& ways_table) :
        PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table) {}

    /**
     * \brief Constructor for testing purposes, will not establish database connections
     *
     * This constructor takes references to the program configuration and the instances of Table class.
     */
    ImportHandler(Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Config& config) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config) { }

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
    void area(const osmium::Area& area) {};
};



#endif /* IMPORTHANDLER_HPP_ */