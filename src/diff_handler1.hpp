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

enum class TypeProgress {POINT, WAY, RELATION};

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
    Table& m_relations_table;

    /**
     * buffers for insert via COPY
     */
    std::string m_nodes_table_copy_buffer;
    std::string m_untagged_nodes_table_copy_buffer;
    std::string m_ways_table_copy_buffer;
    std::string m_relations_table_copy_buffer;


//    /**
//     * list of objects which should be deleted
//     */
//    std::vector<osmium::object_id_type> m_delete_nodes;

    /**
     * maximum size of copy buffer
     */
    static const int BUFFER_SEND_SIZE = 10000;

    /**
     * Track progress of import.
     *
     * Nodes have to be imported before ways, otherwise we will not find some v1 nodes which are needed by "their" ways.
     */
    TypeProgress m_progress = TypeProgress::POINT;

    /**
     * write all nodes which have to be written to the database
     */
    void write_new_nodes();

    /**
     * write all ways which have to be written to the database
     */
    void write_new_ways();

public:
    DiffHandler1(Config& config, Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Table& relations_table) :
            PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table),
            m_relations_table(relations_table) { }

    /**
     * constructor for testing purposes, will not establish database connections
     */
    DiffHandler1(Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Table& relations_table, Config& config) :
        PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config),
        m_relations_table(relations_table)
        { }

    ~DiffHandler1();

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void insert_way(const osmium::Way& way, std::string& ways_table_copy_buffer);

    void insert_relation(const osmium::Relation& relation);

    void relation(const osmium::Relation& area);

    void area(const osmium::Area& area) {};
};



#endif /* APPEND_HANDLER_HPP_ */
