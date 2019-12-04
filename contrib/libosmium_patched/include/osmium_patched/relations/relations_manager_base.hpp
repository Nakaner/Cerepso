/*
This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2019 Jochen Topf <jochen@topf.org>.

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#ifndef OSMIUM_PATCHED_RELATION_MANAGER_BASE_HPP_
#define OSMIUM_PATCHED_RELATION_MANAGER_BASE_HPP_

#include <osmium_patched/relations/members_database.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/check_order.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/memory/callback_buffer.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/relations/manager_util.hpp>
#include <osmium/relations/relations_database.hpp>
#include <osmium/storage/item_stash.hpp>
#include <osmium/tags/taglist.hpp>
#include <osmium/tags/tags_filter.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace osmium_patched {

    namespace relations {

        /**
         * This is a base class of the RelationsManager class template. It
         * contains databases for the relations and the members that we need
         * to keep track of and handles the ouput buffer. Unlike the
         * RelationsManager class template this is a plain class.
         *
         * Usually it is better to use the RelationsManager class template
         * as a basis for your code, but you can also use this class if you
         * have special needs.
         */
        class RelationsManagerBase : public osmium::handler::Handler {

            // All relations and members we are interested in will be kept
            // in here.
            osmium::ItemStash m_stash{};

            /// Database of all relations we are interested in.
            osmium::relations::RelationsDatabase m_relations_db;

            /// Databases of all members we are interested in.
            relations::MembersDatabase<osmium::Node>     m_member_nodes_db;
            relations::MembersDatabase<osmium::Way>      m_member_ways_db;
            relations::MembersDatabase<osmium::Relation> m_member_relations_db;

            /// Output buffer.
            osmium::memory::CallbackBuffer m_output{};

        public:

            RelationsManagerBase() :
                m_relations_db(m_stash),
                m_member_nodes_db(m_stash, m_relations_db),
                m_member_ways_db(m_stash, m_relations_db),
                m_member_relations_db(m_stash, m_relations_db) {
            }

            /// Access the internal RelationsDatabase.
            osmium::relations::RelationsDatabase& relations_database() noexcept {
                return m_relations_db;
            }

            /// Access the internal database containing member nodes.
            relations::MembersDatabase<osmium::Node>& member_nodes_database() noexcept {
                return m_member_nodes_db;
            }

            /// Access the internal database containing member nodes.
            const relations::MembersDatabase<osmium::Node>& member_nodes_database() const noexcept {
                return m_member_nodes_db;
            }

            /// Access the internal database containing member ways.
            relations::MembersDatabase<osmium::Way>& member_ways_database() noexcept {
                return m_member_ways_db;
            }

            /// Access the internal database containing member ways.
            const relations::MembersDatabase<osmium::Way>& member_ways_database() const noexcept {
                return m_member_ways_db;
            }

            /// Access the internal database containing member relations.
            relations::MembersDatabase<osmium::Relation>& member_relations_database() noexcept {
                return m_member_relations_db;
            }

            /// Access the internal database containing member relations.
            const relations::MembersDatabase<osmium::Relation>& member_relations_database() const noexcept {
                return m_member_relations_db;
            }

            /**
             * Access the internal database containing members of the
             * specified type (non-const version of this function).
             *
             * @param type osmium::item_type::node, way, or relation.
             */
            relations::MembersDatabaseCommon& member_database(osmium::item_type type) {
                switch (type) {
                    case osmium::item_type::node:
                        return m_member_nodes_db;
                    case osmium::item_type::way:
                        return m_member_ways_db;
                    case osmium::item_type::relation:
                        return m_member_relations_db;
                    default:
                        break;
                }
                throw std::logic_error{"Should not be here."};
            }

            /**
             * Access the internal database containing members of the
             * specified type (const version of this function).
             *
             * @param type osmium::item_type::node, way, or relation.
             */
            const relations::MembersDatabaseCommon& member_database(osmium::item_type type) const {
                switch (type) {
                    case osmium::item_type::node:
                        return m_member_nodes_db;
                    case osmium::item_type::way:
                        return m_member_ways_db;
                    case osmium::item_type::relation:
                        return m_member_relations_db;
                    default:
                        break;
                }
                throw std::logic_error{"Should not be here."};
            }

            /**
             * Get member object from relation member.
             *
             * @returns A pointer to the member object if it is available.
             *          Returns nullptr otherwise.
             */
            const osmium::OSMObject* get_member_object(const osmium::RelationMember& member) const noexcept {
                if (member.ref() == 0) {
                    return nullptr;
                }
                return member_database(member.type()).get_object(member.ref());
            }

            /**
             * Get node with specified ID from members database.
             *
             * @param id The node ID we are looking for.
             * @returns A pointer to the member node if it is available.
             *          Returns nullptr otherwise.
             */
            const osmium::Node* get_member_node(osmium::object_id_type id) const noexcept {
                if (id == 0) {
                    return nullptr;
                }
                return member_nodes_database().get(id);
            }

            /**
             * Get way with specified ID from members database.
             *
             * @param id The way ID we are looking for.
             * @returns A pointer to the member way if it is available.
             *          Returns nullptr otherwise.
             */
            const osmium::Way* get_member_way(osmium::object_id_type id) const noexcept {
                if (id == 0) {
                    return nullptr;
                }
                return member_ways_database().get(id);
            }

            /**
             * Get relation with specified ID from members database.
             *
             * @param id The relation ID we are looking for.
             * @returns A pointer to the member relation if it is available.
             *          Returns nullptr otherwise.
             */
            const osmium::Relation* get_member_relation(osmium::object_id_type id) const noexcept {
                if (id == 0) {
                    return nullptr;
                }
                return member_relations_database().get(id);
            }

            /**
             * Sort the members databases to prepare them for reading. Usually
             * this is called between the first and second pass reading through
             * an OSM data file.
             */
            void prepare_for_lookup() {
                m_member_nodes_db.prepare_for_lookup();
                m_member_ways_db.prepare_for_lookup();
                m_member_relations_db.prepare_for_lookup();
            }

            /**
             * Return the memory used by different components of the manager.
             */
            osmium::relations::relations_manager_memory_usage used_memory() const noexcept {
                return {
                    m_relations_db.used_memory(),
                      m_member_nodes_db.used_memory()
                    + m_member_ways_db.used_memory()
                    + m_member_relations_db.used_memory(),
                    m_stash.used_memory()
                };
            }

            /// Access the output buffer.
            osmium::memory::Buffer& buffer() noexcept {
                return m_output.buffer();
            }

            /// Set the callback called when the output buffer is full.
            void set_callback(const std::function<void(osmium::memory::Buffer&&)>& callback) {
                m_output.set_callback(callback);
            }

            /// Flush the output buffer.
            void flush_output() {
                m_output.flush();
            }

            /// Flush the output buffer if it is full.
            void possibly_flush() {
                m_output.possibly_flush();
            }

            /// Return the contents of the output buffer.
            osmium::memory::Buffer read() {
                return m_output.read();
            }

        }; // class RelationsManagerBase

    } // namespace relations

} // namespace osmium_patched


#endif /* OSMIUM_PATCHED_RELATION_MANAGER_BASE_HPP_ */
