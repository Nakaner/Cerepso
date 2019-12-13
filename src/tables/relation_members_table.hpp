/*
 * member_table.hpp
 *
 *  Created on:  2019-12-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef TABLE_RELATION_MEMBERS_TABLE_HPP_
#define TABLE_RELATION_MEMBERS_TABLE_HPP_

#include <osmium/osm/types.hpp>
#include "postgres_table.hpp"

/**
 * Table storing node members of ways or node/way/relation members of relations.
 */
class RelationMembersTable : public PostgresTable {
protected:
    /**
     * Register all the prepared statements we need.
     */
    void request_prepared_statements();

public:
    RelationMembersTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns);

    RelationMembersTable() = delete;

    ~RelationMembersTable();

    /**
     * \brief delete member list of a relation
     *
     * This method executes the prepared statement `delete_members_by_object_id`.
     *
     * \param id way ID
     *
     * \throws std::runtime_error
     */
    void delete_members_by_object_id(const osmium::object_id_type id);

    /**
     * \brief Get IDs of relations referencing an object.
     *
     * \param member_id OSM node ID
     * \throws std::runtime_error if query execution fails
     * \returns vector of way/relation IDs or empty vector if none was found
     */
    std::vector<osmium::object_id_type> get_relation_ids(const osmium::object_id_type member_id);

    /**
     * \brief Get members by ID of the parent object
     *
     * \param members vector to insert results into
     * \param id relation ID
     * \tparam TOSMType OSM object type
     */
    template <osmium::item_type TOSMType>
    void get_members_by_id_and_type(std::vector<postgres_drivers::MemberIdTypePos>& members,
            const osmium::object_id_type id) {
        assert(m_database_connection);
        assert(!m_copy_mode);
        char const *paramValues[1];
        static char buffer[64];
        sprintf(buffer, "%ld", id);
        paramValues[0] = buffer;
        PGresult* result = PQexecPrepared(m_database_connection, "get_members_by_relation_id", 1, paramValues, nullptr, nullptr, 0);
        if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
            throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
            PQclear(result);
            return;
        }
        if (PQntuples(result) == 0) {
            PQclear(result);
            return;
        }
        int tuple_count = PQntuples(result);
        members.reserve(members.size() + PQntuples(result));
        for (int i = 0; i < tuple_count; ++i){
            members.emplace_back(strtoll(PQgetvalue(result, i, 0), nullptr, 10), TOSMType, strtoll(PQgetvalue(result, i, 1), nullptr, 10));
        }
        PQclear(result);
    }
};


#endif /* TABLE_RELATION_MEMBERS_TABLE_HPP_ */
