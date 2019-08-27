#ifndef WKBHPP_WKBWRITER_HPP
#define WKBHPP_WKBWRITER_HPP

/*

This file is part of WKBHPP.

Copyright 2019 Michael Reichert <code@michreichert.de> and others
(see README).

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

// macros for version
#define WKBHPP_VERSION_MAJOR 0
#define WKBHPP_VERSION_MINOR 1
#define WKBHPP_VERSION_PATCH 0

#define WKBHPP_VERSION_STRING "0.1.0"

// macros for endianess (taken from Libosmium)
// Windows is only available for little endian architectures
// http://stackoverflow.com/questions/6449468/can-i-safely-assume-that-windows-installations-will-always-be-little-endian
#if defined(__FreeBSD__)
# include <sys/endian.h>
#elif !defined(_WIN32) && !defined(__APPLE__)
# include <endian.h>
#else
# define __LITTLE_ENDIAN 1234
# define __BYTE_ORDER __LITTLE_ENDIAN
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace wkbhpp {

    class wkb_error : public std::runtime_error {

    public:

        explicit wkb_error(const std::string& message) :
            std::runtime_error(message) {
        }

    }; // class geometry_error

    enum class wkb_type : bool {
        wkb  = false,
        ewkb = true
    }; // enum class wkb_type

    enum class out_type : bool {
        binary = false,
        hex    = true
    }; // enum class out_type

    template <typename T>
    inline void str_push(std::string& str, T data) {
        str.append(reinterpret_cast<const char*>(&data), sizeof(T));
    }

    inline std::string convert_to_hex(const std::string& str) {
        static const char* lookup_hex = "0123456789ABCDEF";
        std::string out;
        out.reserve(str.size() * 2);

        for (char c : str) {
            out += lookup_hex[(static_cast<unsigned int>(c) >> 4u) & 0xfu];
            out += lookup_hex[ static_cast<unsigned int>(c)        & 0xfu];
        }

        return out;
    }

    class WKBWriter {
        /**
         * Type of WKB geometry.
         * These definitions are from
         * 99-049_OpenGIS_Simple_Features_Specification_For_SQL_Rev_1.1.pdf (for WKB)
         * and https://trac.osgeo.org/postgis/browser/trunk/doc/ZMSgeoms.txt (for EWKB).
         * They are used to encode geometries into the WKB format.
         */
         enum wkbGeometryType : uint32_t {
             wkbPoint               = 1,
             wkbLineString          = 2,
             wkbPolygon             = 3,
             wkbMultiPoint          = 4,
             wkbMultiLineString     = 5,
             wkbMultiPolygon        = 6,
             wkbGeometryCollection  = 7,

             // SRID-presence flag (EWKB)
             wkbSRID                = 0x20000000
         }; // enum wkbGeometryType

         /**
         * Byte order marker in WKB geometry.
         */
         enum class wkb_byte_order_type : uint8_t {
             XDR = 0,         // Big Endian
             NDR = 1          // Little Endian
         }; // enum class wkb_byte_order_type

         std::string m_data;
         uint32_t m_points = 0;
         int m_srid;
         wkb_type m_wkb_type;
         out_type m_out_type;

         std::size_t m_linestring_size_offset = 0;
         std::size_t m_polygons = 0;
         std::size_t m_rings = 0;
         std::size_t m_multipolygon_size_offset = 0;
         std::size_t m_polygon_size_offset = 0;
         std::size_t m_ring_size_offset = 0;

         std::size_t header(std::string& str, wkbGeometryType type, bool add_length) const {
#if __BYTE_ORDER == __LITTLE_ENDIAN
             str_push(str, wkb_byte_order_type::NDR);
#else
             str_push(str, wkb_byte_order_type::XDR);
#endif
             if (m_wkb_type == wkb_type::ewkb) {
                 str_push(str, type | wkbSRID);
                 str_push(str, m_srid);
             } else {
                 str_push(str, type);
             }
             const std::size_t offset = str.size();
             if (add_length) {
                 str_push(str, static_cast<uint32_t>(0));
             }
             return offset;
         }

         void set_size(const std::size_t offset, const std::size_t size) {
             if (size > std::numeric_limits<uint32_t>::max()) {
                 throw wkb_error{"Too many points in geometry"};
             }
             const auto s = static_cast<uint32_t>(size);
             std::copy_n(reinterpret_cast<const char*>(&s), sizeof(uint32_t), &m_data[offset]);
         }

    public:

         using point_type        = std::string;
         using linestring_type   = std::string;
         using polygon_type      = std::string;
         using multipolygon_type = std::string;
         using ring_type         = std::string;

         explicit WKBWriter(int srid, wkb_type wtype = wkb_type::wkb, out_type otype = out_type::binary) :
             m_srid(srid),
             m_wkb_type(wtype),
             m_out_type(otype) {
         }

         /* Point */
         std::string make_point(const double x, const double y) const {
             std::string data;
             header(data, wkbPoint, false);
             str_push(data, x);
             str_push(data, y);

             if (m_out_type == out_type::hex) {
                 return convert_to_hex(data);
             }

             return data;
         }

         /* LineString */

         void linestring_start() {
             m_data.clear();
             m_linestring_size_offset = header(m_data, wkbLineString, true);
         }

         void linestring_add_location(const double x, const double y) {
             str_push(m_data, x);
             str_push(m_data, y);
         }

         std::string linestring_finish(std::size_t num_points) {
             set_size(m_linestring_size_offset, num_points);
             std::string data;

             using std::swap;
             swap(data, m_data);

             if (m_out_type == out_type::hex) {
                 return convert_to_hex(data);
             }

             return data;
         }

         /* Polygon */

         void polygon_start() {
             m_data.clear();
             m_rings = 0;
             m_polygon_size_offset = header(m_data, wkbPolygon, true);
         }

         void polygon_outer_ring_start() {
             ++m_rings;
             m_points = 0;
             m_ring_size_offset = m_data.size();
             str_push(m_data, static_cast<uint32_t>(0));
         }

         void polygon_outer_ring_finish() {
             set_size(m_ring_size_offset, m_points);
         }

         void polygon_inner_ring_start() {
             polygon_outer_ring_start();
         }

         void polygon_inner_ring_finish() {
             polygon_outer_ring_finish();
         }

         void polygon_add_location(const double x, const double y) {
             multipolygon_add_location(x, y);
         }

         std::string polygon_finish() {
             set_size(m_polygon_size_offset, m_rings);
             std::string data;

             using std::swap;
             swap(data, m_data);

             if (m_out_type == out_type::hex) {
                 return convert_to_hex(data);
             }

             return data;
         }

         /* MultiPolygon */

         void multipolygon_start() {
             m_data.clear();
             m_polygons = 0;
             m_multipolygon_size_offset = header(m_data, wkbMultiPolygon, true);
         }

         void multipolygon_polygon_start() {
             ++m_polygons;
             m_rings = 0;
             m_polygon_size_offset = header(m_data, wkbPolygon, true);
         }

         void multipolygon_polygon_finish() {
             set_size(m_polygon_size_offset, m_rings);
         }

         void multipolygon_outer_ring_start() {
             ++m_rings;
             m_points = 0;
             m_ring_size_offset = m_data.size();
             str_push(m_data, static_cast<uint32_t>(0));
         }

         void multipolygon_outer_ring_finish() {
             set_size(m_ring_size_offset, m_points);
         }

         void multipolygon_inner_ring_start() {
             ++m_rings;
             m_points = 0;
             m_ring_size_offset = m_data.size();
             str_push(m_data, static_cast<uint32_t>(0));
         }

         void multipolygon_inner_ring_finish() {
             set_size(m_ring_size_offset, m_points);
         }

         void multipolygon_add_location(const double x, const double y) {
             str_push(m_data, x);
             str_push(m_data, y);
             ++m_points;
         }

         std::string multipolygon_finish() {
             set_size(m_multipolygon_size_offset, m_polygons);
             std::string data;

             using std::swap;
             swap(data, m_data);

             if (m_out_type == out_type::hex) {
                 return convert_to_hex(data);
             }

             return data;
         }

    }; // class WKBWriter

} // namespace wkbhpp



#endif /* WKBHPP_WKBWRITER_HPP */
