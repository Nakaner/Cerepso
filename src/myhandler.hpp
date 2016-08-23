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
private:
    std::string untagged_nodes_copy_buffer = "";
    std::string nodes_copy_buffer = "";
    std::string ways_copy_buffer = "";

public:
    MyHandler() = delete;

    /**
     * constructor for normal usage
     */
    MyHandler(Config& config, Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns)
        : PostgresHandler(config, node_columns, untagged_nodes_columns, way_linear_columns) { }

    /**
     * constructor for testing purposes, will not establish database connections
     */
    MyHandler(Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns,
                Config& config) : PostgresHandler(node_columns, untagged_nodes_columns, way_linear_columns, config) { }

    ~MyHandler() {
    }

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void area(const osmium::Area& area) {};
};



#endif /* MYHANDLER_HPP_ */
