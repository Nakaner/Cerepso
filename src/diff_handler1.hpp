/*
 * append_handler.hpp
 *
 *  Created on: 11.07.2016
 *      Author: michael
 */

#ifndef APPEND_HANDLER_HPP_
#define APPEND_HANDLER_HPP_
//#include <sstream>
#include "postgres_handler.hpp"

/**
 * Diff handler for table imported using MyHandler class.
 *
 * This handler deletes all objects with version > 1 from the tables. Another handler will import them again.
 */

class DiffHandler1 : public PostgresHandler {
private:
    /**
     * additional table for relations which is not inherited from PostgresHandler
     */
    Table m_relations_table;

    /**
     * buffers for insert via COPY
     */
    std::string m_nodes_table_copy_buffer;
    std::string m_untagged_nodes_table_copy_buffer;
    std::string m_ways_table_copy_buffer;

//    /**
//     * list of objects which should be deleted
//     */
//    std::vector<osmium::object_id_type> m_delete_nodes;

    /**
     * maximum size of copy buffer
     */
    static const int BUFFER_SEND_SIZE = 10000;

public:
    DiffHandler1(Config& config, Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns,
            Columns relation_other_columns) :
            PostgresHandler(config, node_columns, untagged_nodes_columns, way_linear_columns),
            m_relations_table("relations_other", config, relation_other_columns) { }

    /**
     * constructor for testing purposes, will not establish database connections
     */
    DiffHandler1(Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns, Columns relation_other_columns,
                Config& config) : PostgresHandler(node_columns, untagged_nodes_columns, way_linear_columns, config),
            m_relations_table(relation_other_columns, config) { }

    ~DiffHandler1();

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    /**
     * add tags, metadata and geometry of a way to the copy buffer
     *
     * This method is public because we want to test it.
     *
     * @param way the way
     * @param ways_table_copy_buffer copy buffer
     */
    void insert_way(const osmium::Way& way, std::string& ways_table_copy_buffer);

    void relation(const osmium::Relation& area);

    void area(const osmium::Area& area) {};
};



#endif /* APPEND_HANDLER_HPP_ */
