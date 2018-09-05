/*
 * tags_storage.cpp
 *
 *  Created on:  2018-09-03
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "tags_storage.hpp"

TagsStorage::TagsStorage() :
    m_stash(),
    m_index() {
}

void TagsStorage::add(const osmium::object_id_type id, const osmium::TagList& tags) {
    m_index.emplace_back(id, m_stash.add_item(tags));
}

/**
 * Get object.
 */
const osmium::TagList* TagsStorage::get(const osmium::object_id_type id) {
    const element searched {id};
    auto it = std::lower_bound(m_index.begin(), m_index.end(), searched);
    if (it == m_index.end() || it->id() != id) {
        // not found
        return nullptr;
    }
    return &(m_stash.get<osmium::TagList>(it->handle()));
}
