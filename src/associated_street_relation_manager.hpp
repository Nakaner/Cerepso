/*
 * associated_street_relation_manager.hpp
 *
 *  Created on:  2018-08-16
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef ASSOCIATED_STREET_RELATION_MANAGER_HPP_
#define ASSOCIATED_STREET_RELATION_MANAGER_HPP_

#include <osmium/memory/buffer.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/way.hpp>
#include <osmium_patched/relations/relations_manager_base.hpp>
#include <osmium/relations/detail/member_meta.hpp>
#include <osmium/util/iterator.hpp>

class AssociatedStreetRelationManager: public osmium_patched::relations::RelationsManagerBase {

    osmium::relations::SecondPassHandler<AssociatedStreetRelationManager> m_handler_pass2;

    /**
     * \brief This method decides which relations we're interested in, and
     * instructs Osmium to collect their members for us.
     *
     * This method is derived from osmium::handler::Handler.
     *
     * \return always `true` because we do not filter out any relations
     */
    bool new_relation(const osmium::Relation& relation) const;

public:
    AssociatedStreetRelationManager();

    /**
     * Return reference to second pass handler.
     */
    osmium::relations::SecondPassHandler<AssociatedStreetRelationManager>& handler(const std::function<void(osmium::memory::Buffer&&)>& callback = nullptr) {
        set_callback(callback);
        return m_handler_pass2;
    }

    void relation(const osmium::Relation& relation);

    const osmium::TagList* get_relation_tags(const osmium::object_id_type id,
            const osmium::item_type type);

    void handle_node(const osmium::Node& node);
};


#endif /* ASSOCIATED_STREET_RELATION_MANAGER_HPP_ */
