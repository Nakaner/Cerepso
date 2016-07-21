/*
 * append_handler.hpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#ifndef APPEND_HANDLER_HPP_
#define APPEND_HANDLER_HPP_
#include <sstream>
#include "postgres_handler.hpp"

/**
 * Diff handler for table imported using MyHandler class.
 *
 * This handler has buffers for both deletions and inserts.
 */

class AppendHandler : public PostgresHandler {
private:
    /**
     * buffers for insert via COPY
     */
    std::string m_nodes_table_copy_buffer;
    std::string m_untagged_nodes_table_copy_buffer;
    std::string m_ways_table_copy_buffer;

    /**
     * list of objects which should be deleted
     */
    std::vector<osmium::object_id_type> m_delete_nodes;

    /**
     * maximum size of copy buffer
     */
    static const int BUFFER_SEND_SIZE = 10000;

public:
    AppendHandler(Config& config, Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns)
        : PostgresHandler(config, node_columns, untagged_nodes_columns, way_linear_columns) { }

    ~AppendHandler();

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void relation(const osmium::Relation& area);

    void area(const osmium::Area& area) {};
};



#endif /* APPEND_HANDLER_HPP_ */
