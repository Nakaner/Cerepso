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
    EVEN = 2,
    ALL = 3,
    ALPHABETIC = 4
};

/**
 * Represent the numeric number and the optional alphabetic suffix of a standard :-) Western house number
 * as supported by address interpolations.
 */
struct HouseNumber {
    /// numeric part
    int number;
    /// suffix ('\0' if not set)
    char suffix;

    /// Create an invalid house number.
    HouseNumber();

    /**
     * Create a house number.
     *
     * \param base numeric part
     * \param alpha optional suffix
     */
    HouseNumber(int base, char alpha);

    /// Check if house number is valid.
    bool valid() const;

    /**
     * Get difference between this house number and another house number using the given interpolation scheme.
     *
     * All, even and odd interpolations are treated the same way. For alphabetic interpolations, suffixes are
     * always converted using ::lower(char).
     *
     * \param type type of interpolatin (all, even, odd, alphabetic).
     * \param other other house number
     *
     * \returns difference as signed integer
     */
    int interpolation_difference(InterpolationType type, HouseNumber& other) const noexcept;

    /**
     * Factory method to create a instance of HouseNumber form a string.
     *
     * \returns The house number. This might be invalid if
     *
     * \throws std::runtime_error if (a) the suffix is not alphabetic, (b) the suffix is longer
     * than one character or (c) if the number is equal or smaller than zero
     */
    static HouseNumber parse_number(const char* start);

    /**
     * Increment the numeric part by a given increment.
     */
    void increment_numeric(const int increment);

    /**
     * Increment the alphabetic part by a given increment.
     *
     * \throws std::runtime_error if the resulting suffix is not in range [A-Za-z]
     */
    void increment_alphabetic(const int increment);
};

/**
 * Class to track which nodes are referenced by address interpolation ways.
 *
 * IDs are stored in a vector which has to be sorted before lookups are possible.
 */
class RequiredNodesTracking {
    std::vector<osmium::object_id_type> m_vector;

    /**
     * Iterator to element in the vector which has been returned by the last successful call of
     * node_needed.
     */
    std::vector<osmium::object_id_type>::iterator m_last_node_iter;

public:
    RequiredNodesTracking();

    /**
     * Register the ID of a node.
     */
    void track(const osmium::object_id_type id);

    /**
     * Order the list of IDs and remove duplicates.
     */
    void prepare_for_lookup();

    /**
     * Check if a given ID is registered.
     *
     * This method should be called with ascending ID because the search starts
     * at the last ID found in the index by this method. If you call this method with an
     * arbitrary order, search times are depended from the last found ID.
     */
    bool node_needed(const osmium::object_id_type id);
};

/**
 * A handler with two passes inspired by osmium::relations::RelationManager
 * to create the virtual address points between the beginning and end of an
 * address interpolation. This handler writes the interpolated points directly into its
 * own database table.
 */
class AddrInterpolationHandler : public osmium::handler::Handler {

    /**
     * Handler for the second pass, just a wrapper to around AddrInterpolationHandler in order to
     * be able to use osmium::visitor.
     */
    class SecondPassHandler : public osmium::handler::Handler {
        AddrInterpolationHandler& m_interpolation_handler;

    public:
        SecondPassHandler(AddrInterpolationHandler& interpolation_handler) noexcept;

        /**
         * Wrapper around AddrInterpolationHandler::handle_node
         *
         * \osmiumcallback
         */
        void node(const osmium::Node& node);

        /**
         * Wrapper around AddrInterpolationHandler::handle_way
         *
         * \osmiumcallback
         */
        void way(const osmium::Way& way);
    };


    RequiredNodesTracking m_required_nodes;
    SecondPassHandler m_handler_pass2;
    PostgresTable& m_table;
    TagsStorage m_tags_storage;

    using NodeConstIterator = osmium::WayNodeList::const_iterator;

    /**
     * Add all interpolated address points of one or multiple segments of the interpolated way
     * to a string.
     *
     * \param query string to add the data to be written to the database to
     * \param it1 iterator pointing to the first node of the segment
     * \param it2 iterator pointing to the last node of the segtment
     * \param tags1 pointer to tags of the first node
     * \param tags2 pointer to tags of the last node
     * \param way OSM way
     * \param type type of interpolation
     */
    void add_points_to_str(std::string& query, NodeConstIterator it1, NodeConstIterator it2,
            const osmium::TagList* tags1, const osmium::TagList* tags2, const osmium::Way& way,
            InterpolationType type);

    /**
     * Get location of an interpolated address point.
     *
     * \param it1 iterator pointing to the first address node with a housenumber of this segment
     * \param it2 iterator pointing to the last address node with a housenumber of this segment
     * \param fraction fraction between 0.0 and 1.0 of the total length of this segment where the
     * interpolated address point should be located.
     * \returns location of the interpolated point in WGS84
     */
    osmium::Location get_remote_point(NodeConstIterator it1, NodeConstIterator it2, double fraction);

public:
    AddrInterpolationHandler(PostgresTable& interpolated_nodes_table);

    /**
     * Check if value of addr:interpolation=* is valid.
     */
    static bool valid_interpolation(const char* value);

    /**
     * way callback for first pass
     *
     * \osmiumcallback
     */
    void way(const osmium::Way& way);

    /**
     * Check if the given node is referenced by any address interpolation
     * and store its tags.
     */
    void handle_node(const osmium::Node& node);

    /**
     * Check if the given way is an addresse interpolation. Create virtual points for the
     * interpolated addresses in the database.
     */
    void handle_way(const osmium::Way& way);

    /**
     * Has to be called after the first pass.
     */
    void after_pass1();

    /**
     * Create points for the interpolated addresses and write a string to be written into the
     * database via PostgreSQL COPY.
     *
     * \param way way to interpolate
     * \param query string to append the query to
     */
    void interpolate(const osmium::Way& way, std::string& query);

    SecondPassHandler& handler();
};



#endif /* ADDR_INTERPOLATION_HANDLER_HPP_ */
