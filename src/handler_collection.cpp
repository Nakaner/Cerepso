/*
 * handler_collection.cpp
 *
 *  Created on:  2017-07-21
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "handler_collection.hpp"

void HandlerCollection::node(const osmium::Node& node) {
    for (auto cb : m_node_callbacks) {
        cb(node);
    }
}

void HandlerCollection::way(const osmium::Way& way) {
    for (auto cb : m_way_callbacks) {
        cb(way);
    }
}

void HandlerCollection::relation(const osmium::Relation& relation) {
    for (auto cb : m_relation_callbacks) {
        cb(relation);
    }
}
