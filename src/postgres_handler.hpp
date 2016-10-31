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
#include <osmium/osm/area.hpp>
#include <osmium/geom/wkb.hpp>
#include <memory>
#include "table.hpp"

#ifndef POSTGRES_HANDLER_HPP_
#define POSTGRES_HANDLER_HPP_

class PostgresHandler : public osmium::handler::Handler {
public:
    /**
     * add tag hstore column and metadate to query
     */
    static void add_tags(std::string& query, const osmium::OSMObject& object);

    /**
     * \brief Add metadata (user, uid, changeset, timestamp) to the stringstream.
     * A separator (via add_separator_to_stringstream) will be added before the first and after the last item.
     *
     * \param ss stringstream
     * \param object OSM object
     * \param config reference to current program config
     *
     * \todo Move the static methods into either a separate namespace or create a class for the table scheme which is the
     * base class for all the handlers.
     * */
    static void add_metadata_to_stringstream(std::string& ss, const osmium::OSMObject& object, Config& config);

    /**
     * \brief Add a TAB at the end of the string.
     *
     * \param ss String where the TAB should be appended.
     */
    static void add_separator_to_stringstream(std::string& ss);

    /**
     * \brief Build the line which will be inserted via `COPY` into the database.
     *
     * \param node Reference to the node to be inserted
     *
     * \param query Reference to the string where the line should be appended.
     *              It is possible that one string contains multiple lines (e.g. for buffered writing).
     */
    void prepare_node_query(const osmium::Node& node, std::string& query);

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
    osmium::geom::WKBFactory<> wkb_factory;
    Config& m_config;
    /// reference to nodes table
    Table& m_nodes_table;
    /// reference to table of untagged nodes
    Table& m_untagged_nodes_table;
    /// reference to table of all ways
    Table& m_ways_linear_table;
    PostgresHandler(Config& config, Table& nodes_table, Table& untagged_nodes_table, Table& ways_table) :
            wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex),
            m_config(config),
            m_nodes_table(nodes_table),
            m_untagged_nodes_table(untagged_nodes_table),
            m_ways_linear_table(ways_table) {}


    /**
     * \brief constructor for testing purposes, will not establish database connections
     */
    PostgresHandler(Table& nodes_table, Table& untagged_nodes_table, Table& ways_table, Config& config)  :
            wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex),
            m_config(config),
            m_nodes_table(nodes_table),
            m_untagged_nodes_table(untagged_nodes_table),
            m_ways_linear_table(ways_table) {}


    virtual ~PostgresHandler()  {}
};



#endif /* POSTGRES_HANDLER_HPP_ */
