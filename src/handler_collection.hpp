/*
 * handler_collection.hpp
 *
 *  Created on:  2017-07-20
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_HANDLER_COLLECTION_HPP_
#define SRC_HANDLER_COLLECTION_HPP_

#include <functional>
#include <vector>

#include <osmium/handler.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/way.hpp>

/**
 * Available views
 */
enum class ViewType : char {
    none= 0,
    places= 1,
    geometry= 2,
    highways= 3,
    tagging= 4
};

/**
 * The handler collection manages all handlers and calls their node, way, relation and area callbacks one
 * after another. This allows us to only instanciate those handlers which are necessary.
 */
class HandlerCollection : public osmium::handler::Handler {
    std::vector< std::function< void(const osmium::Node&)> > m_node_callbacks;
    std::vector< std::function< void(const osmium::Way&)> > m_way_callbacks;
    std::vector< std::function< void(const osmium::Relation&)> > m_relation_callbacks;

public:
    template <class THandler>
    void add(THandler& handler) {
        // register node callback
        using namespace std::placeholders;
        auto node_fn = std::bind(&THandler::node, &handler, _1);
        m_node_callbacks.push_back(std::move(node_fn));
        // register way callback
        auto way_fn = std::bind(&THandler::way, &handler, _1);
        m_way_callbacks.push_back(std::move(way_fn));
        // register relation callback
        auto rel_fn = std::bind(&THandler::relation, &handler, _1);
        m_relation_callbacks.push_back(std::move(rel_fn));
    }

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void relation(const osmium::Relation& relation);
};



#endif /* SRC_HANDLER_COLLECTION_HPP_ */
