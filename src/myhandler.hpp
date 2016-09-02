/*
 * myhandler.hpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#ifndef MYHANDLER_HPP_
#define MYHANDLER_HPP_

#include "postgres_handler.hpp"


class MyHandler : public PostgresHandler {
public:
    MyHandler() = delete;

//    /**
//     * constructor for normal usage
//     */
//    MyHandler(Config& config, Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns)
//        : PostgresHandler(config, node_columns, untagged_nodes_columns, way_linear_columns) { }
    MyHandler(Config& config,  Table& nodes_table, Table& untagged_nodes_table, Table& ways_table) :
        PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table) {}

//    /**
//     * constructor for testing purposes, will not establish database connections
//     */
    MyHandler(Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Config& config) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config) { }

    ~MyHandler() {
    }

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void area(const osmium::Area& area) {};
};



#endif /* MYHANDLER_HPP_ */
