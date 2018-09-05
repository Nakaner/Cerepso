/*
 * tags_storage.hpp
 *
 *  Created on:  2018-09-03
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef TAGS_STORAGE_HPP_
#define TAGS_STORAGE_HPP_

#include <osmium/storage/item_stash.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/types.hpp>


class TagsStorage {

    struct element {
        /// node ID
        osmium::object_id_type m_id;

        /// pointer to TagList in buffer
        osmium::ItemStash::handle_type m_handle;

        element(const osmium::object_id_type id, osmium::ItemStash::handle_type handle) noexcept :
            m_id(id),
            m_handle(handle) {
        }

        element(const osmium::object_id_type id) noexcept :
            m_id(id),
            m_handle() {
        }

        element() = delete;

        /**
         * Get ID of the object.
         */
        osmium::object_id_type id() const {
            return m_id;
        }

        /**
         * Get handle of this object.
         */
        osmium::ItemStash::handle_type& handle() {
            return m_handle;
        }

        bool operator<(const element& other) const {
            return m_id < other.id();
        }
    };
    /**
     * Buffer for the TagLists of all the nodes
     */
    osmium::ItemStash m_stash;

    std::vector<element> m_index;

public:
    TagsStorage();

    void add(const osmium::object_id_type id, const osmium::TagList& tags);

    /**
     * Get object.
     */
    const osmium::TagList* get(const osmium::object_id_type id);
};

#endif /* TAGS_STORAGE_HPP_ */
