/*
 * myhandler.hpp
 *
 *  Created on: 20.06.2016
 *      Author: michael
 */

#ifndef MYHANDLER_HPP_
#define MYHANDLER_HPP_

#include <libpq-fe.h>
#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/geom/wkt.hpp>
#include <osmium/geom/wkb.hpp>
#include "table.hpp"


class MyHandler : public osmium::handler::Handler {
private:
    osmium::geom::WKBFactory<> wkb_factory;
    Table m_nodes_table;
    Table m_untagged_nodes_table;
    Table m_ways_linear_table;
    Table m_ways_polygon_table;
    Table m_relations_polygon_table;

    /**
     * helper function
     */
    void add_separator_to_stringstream(std::stringstream& ss);

    /**
     * Add metadata (user, uid, changeset, timestamp) to the stringstream.
     * A separator (via add_separator_to_stringstream) will be added before the first and after the last item.
     *
     * @param ss stringstream
     * @param object OSM object
     */
    void add_metadata_to_stringstream(std::stringstream& ss, const osmium::OSMObject& object);


public:
    MyHandler() = delete;

    MyHandler(Config& config, Columns& node_columns, Columns& untagged_nodes_columns, Columns& way_linear_columns, Columns& way_polygon_columns,
            Columns& relation_polygon_columns) :
        wkb_factory(osmium::geom::wkb_type::wkb, osmium::geom::out_type::hex),
        m_nodes_table("nodes", config, node_columns),
        m_untagged_nodes_table("untagged_nodes", config, untagged_nodes_columns),
        m_ways_linear_table("ways_linear", config, way_linear_columns),
        m_ways_polygon_table("ways_polygon", config, way_polygon_columns),
        m_relations_polygon_table("relation_polygon", config, relation_polygon_columns) { }

    ~MyHandler() {
    }

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void area(const osmium::Area& area);
};



#endif /* MYHANDLER_HPP_ */
