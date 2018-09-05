/*
 * addr_interpolation_handler.hpp
 *
 *  Created on:  2018-09-03
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef ADDR_INTERPOLATION_HANDLER_HPP_
#define ADDR_INTERPOLATION_HANDLER_HPP_

#include <osmium/handler.hpp>
#include <osmium/memory/buffer.hpp>

#include "postgres_table.hpp"
#include "tags_storage.hpp"

enum class InterpolationType : char {
    UNKNOWN = 0,
    ODD = 1,
    EVEN = 1,
    ALL = 3,
    ALPHABETIC = 4
};

struct HouseNumber {
    int number;
    char suffix;

    HouseNumber() = delete;

    HouseNumber(int base, char alpha) :
        number(base),
        suffix(alpha) {
    }
};

class AddrInterpolationHandler : public osmium::handler::Handler {

    class SecondPassHandler : public osmium::handler::Handler {
        AddrInterpolationHandler& m_interpolation_handler;

    public:
        SecondPassHandler(AddrInterpolationHandler& interpolation_handler) noexcept;

        /**
         * Store tags
         *
         * \osmiumcallback
         */
        void node(const osmium::Node& node);

        /**
         * Interpolate
         *
         * \osmiumcallback
         */
        void way(const osmium::Way& way);
    };


    std::vector<osmium::object_id_type> m_required_nodes;
    SecondPassHandler m_handler_pass2;
    std::vector<osmium::object_id_type>::iterator m_last_node_iter;
    PostgresTable& m_table;
    TagsStorage m_tags_storage;

    using NodeConstIterator = osmium::WayNodeList::const_iterator;

public:
    AddrInterpolationHandler(PostgresTable& interpolated_nodes_table);

    /**
     * Check if value of addr:interpolation=* is valid.
     */
    static bool valid_interpolation(const char* value);

    void way(const osmium::Way& way);

    /**
     * node callback for second pass
     */
    void handle_node(const osmium::Node& node);

    /**
     * way callback for second pass
     */
    void handle_way(const osmium::Way& node);

    void after_pass1();

    bool node_needed(const osmium::object_id_type id);

    void interpolate(const osmium::Way& way, std::string& query);

    void add_points_to_str(std::string& query, NodeConstIterator it1, NodeConstIterator it2,
            const osmium::TagList* tags1, const osmium::TagList* tags2, const osmium::Way& way,
            InterpolationType type);

    osmium::Location get_remote_point(NodeConstIterator it1, NodeConstIterator it2, double fraction);

    int interpolation_difference(InterpolationType type, HouseNumber hn1, HouseNumber hn2) noexcept;

    HouseNumber parse_number(const char* start);

    int increment_numeric(const int start, int increment);

    char increment_alphabetic(const char start_suffix, int increment);

    SecondPassHandler& handler();
};



#endif /* ADDR_INTERPOLATION_HANDLER_HPP_ */
