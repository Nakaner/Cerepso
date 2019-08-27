/*
 * osmium_wkb_wrapper.hpp
 *
 *  Created on:  2019-08-27
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef CONTRIB_WKBCPP_INCLUDE_WKBHPP_OSMIUM_WKB_WRAPPER_HPP_
#define CONTRIB_WKBCPP_INCLUDE_WKBHPP_OSMIUM_WKB_WRAPPER_HPP_

#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/factory.hpp>
#include <wkbhpp/wkbwriter.hpp>

namespace wkbhpp {

    /**
     * This class provides methods with the signature which are called by
     * osmium::geom::GeometryFactory but are not part of the WKBWriter class because they depend
     * on Osmium types and would introduce an unnecessary dependency on Osmium.
     */
    class WKBImplementation : public WKBWriter {
    public:
        using point_type        = std::string;
        using linestring_type   = std::string;
        using polygon_type      = std::string;
        using multipolygon_type = std::string;
        using ring_type         = std::string;

        explicit WKBImplementation(int srid, wkb_type wtype = wkb_type::wkb, out_type otype = out_type::binary) :
            WKBWriter(srid, wtype, otype) {
        }

        point_type make_point(const osmium::geom::Coordinates& xy) const {
            return WKBWriter::make_point(xy.x, xy.y);
        }

        void linestring_add_location(const osmium::geom::Coordinates& xy) {
            WKBWriter::linestring_add_location(xy.x, xy.y);
        }

        void polygon_add_location(const osmium::geom::Coordinates& xy) {
            WKBWriter::polygon_add_location(xy.x, xy.y);
        }

        polygon_type polygon_finish(const size_t) {
            return WKBWriter::polygon_finish();
        }

        void multipolygon_add_location(const osmium::geom::Coordinates& xy) {
            WKBWriter::multipolygon_add_location(xy.x, xy.y);
        }

    }; // class WKBImplementation

    template <typename TProjection = osmium::geom::IdentityProjection>
    using full_wkb_factory = osmium::geom::GeometryFactory<WKBImplementation, TProjection>;

} // namespace wkbhpp

#endif /* CONTRIB_WKBCPP_INCLUDE_WKBHPP_OSMIUM_WKB_WRAPPER_HPP_ */
