#ifndef OSMIUM_PATCHED_RELATIONS_MEMBERS_DATABASE_HPP
#define OSMIUM_PATCHED_RELATIONS_MEMBERS_DATABASE_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2019 Jochen Topf <jochen@topf.org>.
Modification of method visibility by Michael Reichert, 2019-12-04

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

#include <osmium/osm/object.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/relations/relations_database.hpp>
#include <osmium/storage/item_stash.hpp>
#include <osmium/util/iterator.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>
#include <vector>

namespace osmium_patched {

    namespace relations {

        /**
         * This is the parent class for the MembersDatabase class. All the
         * functionality which doesn't depend on the template parameter used
         * in derived databases is contained in this class.
         *
         * Usually you want to use the MembersDatabase class only.
         */
        class MembersDatabaseCommon {

            struct element {

                /**
                 * Special value used for member_num to mark the element as
                 * removed.
                 */
                enum {
                    removed_value = std::numeric_limits<std::size_t>::max()
                };

                /**
                 * Object ID of this relation member. Can be a node, way,
                 * or relation ID. It depends on the database in which this
                 * object is stored which kind of object is referenced here.
                 */
                osmium::object_id_type member_id;

                /**
                 * Position of this member in the parent relation.
                 */
                std::size_t member_num;

                /**
                 * Position of the parent relation in the relations database.
                 */
                std::size_t relation_pos;

                /**
                 * Handle to the stash where the object is stored.
                 *
                 * The default value is the invalid one signifying that the
                 * object hasn't been found yet.
                 */
                osmium::ItemStash::handle_type object_handle;

                explicit element(std::size_t rel_pos, osmium::object_id_type memb_id, std::size_t memb_num) noexcept :
                    member_id(memb_id),
                    member_num(memb_num),
                    relation_pos(rel_pos) {
                }

                /**
                 * This constructor is used to create dummy elements that
                 * can be compared to the elements in a vector using the
                 * equal_range algorithm.
                 */
                explicit element(osmium::object_id_type m_id) noexcept :
                    member_id(m_id),
                    member_num(0),
                    relation_pos(0) {
                }

                bool is_removed() const noexcept {
                    return member_num == removed_value;
                }

                void remove() noexcept {
                    member_num = removed_value;
                }

                bool operator<(const element& other) const noexcept {
                    return std::tie(member_id, member_num, relation_pos) <
                           std::tie(other.member_id, other.member_num, other.relation_pos);
                }

            }; // struct element

            // comparison function only comparing member_id.
            struct compare_member_id {
                bool operator()(const element& a, const element& b) const noexcept {
                    return a.member_id < b.member_id;
                }
            };

            std::vector<element> m_elements{};

        protected:

            osmium::ItemStash& m_stash;
            osmium::relations::RelationsDatabase& m_relations_db;

#ifndef NDEBUG
            // This is used only in debug builds to make sure the
            // prepare_for_lookup() function is called at the right place.
            bool m_init_phase = true;
#endif

            using iterator = std::vector<element>::iterator;
            using const_iterator = std::vector<element>::const_iterator;

        public:
            osmium::iterator_range<iterator> find(osmium::object_id_type id) {
                return osmium::make_range(std::equal_range(m_elements.begin(), m_elements.end(), element{id}, compare_member_id{}));
            }

            osmium::iterator_range<const_iterator> find(osmium::object_id_type id) const {
                return osmium::make_range(std::equal_range(m_elements.cbegin(), m_elements.cend(), element{id}, compare_member_id{}));
            }

        protected:
            static typename osmium::iterator_range<iterator>::iterator::difference_type count_not_removed(const osmium::iterator_range<iterator>& range) noexcept {
                return std::count_if(range.begin(), range.end(), [](const element& elem) {
                    return !elem.is_removed();
                });
            }

            void add_object(const osmium::OSMObject& object, osmium::iterator_range<iterator>& range) {
                const auto handle = m_stash.add_item(object);
                for (auto& elem : range) {
                    elem.object_handle = handle;
                }
            }

            MembersDatabaseCommon(osmium::ItemStash& stash, osmium::relations::RelationsDatabase& relations_db) :
                m_stash(stash),
                m_relations_db(relations_db) {
            }

        public:

            /**
             * Return an estimate of the number of bytes currently needed
             * for the MembersDatabase. This does NOT include the memory used
             * in the stash. Used for debugging.
             */
            std::size_t used_memory() const noexcept {
                return sizeof(element) * m_elements.capacity() +
                       sizeof(MembersDatabaseCommon);
            }

            /**
             * The number of members tracked in the database. Includes
             * members tracked, but not found yet, members found and members
             * marked as removed.
             *
             * Complexity: Constant.
             */
            std::size_t size() const noexcept {
                return m_elements.size();
            }

            /**
             * Result from the count() function.
             */
            struct counts {
                /// The number of members tracked and not found yet.
                std::size_t tracked   = 0;
                /// The number of members tracked and found already.
                std::size_t available = 0;
                /// The number of members that were tracked, found and then removed because of a completed relation.
                std::size_t removed   = 0;
            };

            /**
             * Counts the number of members in different states. Usually only
             * used for testing and debugging.
             *
             * Complexity: Linear in the number of members tracked.
             */
            counts count() const noexcept {
                counts c;

                for (const auto& elem : m_elements) {
                    if (elem.is_removed()) {
                        ++c.removed;
                    } else if (elem.object_handle.valid()) {
                        ++c.available;
                    } else {
                        ++c.tracked;
                    }
                }

                return c;
            }

            /**
             * Tell the database that you are interested in an object with
             * the specified id and that it is a member of the given relation
             * (as specified through the relation handle).
             *
             * @param rel_handle Relation this object is a member of.
             * @param member_id Id of an object of type TObject.
             * @param member_num This is the nth member in the relation.
             */
            void track(osmium::relations::RelationHandle& rel_handle, osmium::object_id_type member_id, std::size_t member_num) {
                assert(m_init_phase && "Can not call MembersDatabase::track() after MembersDatabase::prepare_for_lookup().");
                assert(rel_handle.relation_database() == &m_relations_db);
                m_elements.emplace_back(rel_handle.pos(), member_id, member_num);
                rel_handle.increment_members();
            }

            /**
             * Prepare the database for lookup. Call this function after
             * calling track() for all objects needed and before adding
             * the first object with add() or querying the first object
             * with get(). You can only call this function once.
             */
            void prepare_for_lookup() {
                assert(m_init_phase && "Can not call MembersDatabase::prepare_for_lookup() twice.");
                std::sort(m_elements.begin(), m_elements.end());
#ifndef NDEBUG
                m_init_phase = false;
#endif
            }

            /**
             * Remove the entry with the specified member_id and relation_id
             * from the database. If the entry doesn't exist, nothing happens.
             */
            void remove(osmium::object_id_type member_id, osmium::object_id_type relation_id) {
                const auto range = find(member_id);

                if (range.empty()) {
                    return;
                }

                // If this is the last time this object was needed, remove it
                // from the stash.
                if (count_not_removed(range) == 1) {
                    m_stash.remove_item(range.begin()->object_handle);
                }

                for (auto& elem : range) {
                    if (!elem.is_removed() && relation_id == m_relations_db[elem.relation_pos]->id()) {
                        elem.remove();
                        break;
                    }
                }
            }

            /**
             * Find the object with the specified id in the database and
             * return a pointer to it. Returns nullptr if there is no object
             * with that id in the database.
             *
             * Complexity: Logarithmic in the number of members tracked (as
             *             returned by size()).
             */
            const osmium::OSMObject* get_object(osmium::object_id_type id) const {
                assert(!m_init_phase && "Call MembersDatabase::prepare_for_lookup() before calling get_object().");
                const auto range = find(id);
                if (range.empty()) {
                    return nullptr;
                }
                const auto handle = range.begin()->object_handle;
                if (handle.valid()) {
                    return &m_stash.get<osmium::OSMObject>(handle);
                }
                return nullptr;
            }

        }; // class MembersDatabaseCommon

        /**
         * A MembersDatabase is used together with a RelationsDatabase to
         * bring a relation and their members together. It tracks all members
         * of a specific type needed to complete a relation.
         *
         * More documentation is in the MembersDatabaseCommon parent class
         * which contains all the pieces that aren't dependent on the
         * template parameter.
         *
         * @tparam TObject The object type stores in the members database.
         *                 Can be osmium::Node, Way, or Relation.
         */
        template <typename TObject>
        class MembersDatabase : public MembersDatabaseCommon {

            static_assert(std::is_base_of<osmium::OSMObject, TObject>::value, "TObject must be osmium::Node, Way, or Relation.");

        public:

            /**
             * Construct a MembersDatabase.
             *
             * @param stash Reference to an ItemStash object. All member objects
             *              will be stored in this stash. It must be available
             *              until the MembersDatabase is destroyed.
             * @param relation_db The RelationsDatabase where relations are
             *                    stored. Usually it will use the same ItemStash
             *                    as the MembersDatabase.
             */
            MembersDatabase(osmium::ItemStash& stash, osmium::relations::RelationsDatabase& relation_db) :
                MembersDatabaseCommon(stash, relation_db) {
            }

            /**
             * Add the specified object to the database.
             *
             * @param object Object to add.
             * @param func If the object is the last member to complete a
             *             relation, this function is called with the relation
             *             as a parameter.
             * @returns true if the object was actually added, false if no
             *          relation needed this object.
             */
            template <typename TFunc>
            bool add(const TObject& object, TFunc&& func) {
                assert(!m_init_phase && "Call MembersDatabase::prepare_for_lookup() before calling add().");
                auto range = find(object.id());

                if (range.empty()) {
                    // No relation needs this object.
                    return false;
                }

                // At least one relation needs this object. Store it and
                // "tell" all relations.
                add_object(object, range);

                for (auto& elem : range) {
                    assert(!elem.is_removed());
                    assert(elem.member_id == object.id());

                    auto rel_handle = m_relations_db[elem.relation_pos];
                    assert(elem.member_num < rel_handle->members().size());
                    rel_handle.decrement_members();

                    if (rel_handle.has_all_members()) {
                        func(rel_handle);
                    }
                }

                return true;
            }

            /**
             * Find the object with the specified id in the database and
             * return a pointer to it. Returns nullptr if there is no object
             * with that id in the database.
             *
             * Complexity: Logarithmic in the number of members tracked (as
             *             returned by size()).
             */
            const TObject* get(osmium::object_id_type id) const {
                assert(!m_init_phase && "Call MembersDatabase::prepare_for_lookup() before calling get().");
                return static_cast<const TObject*>(get_object(id));
            }

        }; // class MembersDatabase

    } // namespace relations

} // namespace osmium_patched

#endif // OSMIUM_PATCHED_RELATIONS_MEMBERS_DATABASE_HPP
