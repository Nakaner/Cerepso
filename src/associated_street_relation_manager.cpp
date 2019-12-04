/*
 * associate_street_relation_manager.cpp
 *
 *  Created on:  2018-08-16
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "associated_street_relation_manager.hpp"

AssociatedStreetRelationManager::AssociatedStreetRelationManager() :
    RelationsManagerBase(),
    m_handler_pass2(*this) {
}

bool AssociatedStreetRelationManager::new_relation(const osmium::Relation& relation) const {
    return relation.tags().has_tag("type", "associatedStreet");
}

void AssociatedStreetRelationManager::relation(const osmium::Relation& relation) {
    if (new_relation(relation)) {
        auto rel_handle = relations_database().add(relation);

        std::size_t n = 0;
        for (auto& member : rel_handle->members()) {
            member_database(member.type()).track(rel_handle, member.ref(), n);
            ++n;
        }
    }
}

const osmium::TagList* AssociatedStreetRelationManager::get_relation_tags(const osmium::object_id_type id,
        const osmium::item_type type) {
    const osmium_patched::relations::MembersDatabaseCommon& md = member_database(type);
    auto range = md.find(id);
    if (range.begin() == range.end()) {
        return nullptr;
    }
    return &(relations_database()[range.begin()->relation_pos]->tags());
}
