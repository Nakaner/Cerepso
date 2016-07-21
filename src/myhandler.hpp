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

    /**
     * constructor for normal usage
     */
    MyHandler(Config& config, Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns, Columns& way_polygon_columns,
            Columns& relation_polygon_columns) : PostgresHandler(config, node_columns, untagged_nodes_columns, way_linear_columns,
                    way_polygon_columns, relation_polygon_columns) { }

    /**
     * constructor for testing purposes, will not establish database connections
     */
    MyHandler(Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns, Columns& way_polygon_columns,
                Columns& relation_polygon_columns, Config& config) : PostgresHandler(node_columns, untagged_nodes_columns, way_linear_columns,
                        way_polygon_columns, relation_polygon_columns, config) { }

    ~MyHandler() {
    }

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void area(const osmium::Area& area);
};



#endif /* MYHANDLER_HPP_ */
