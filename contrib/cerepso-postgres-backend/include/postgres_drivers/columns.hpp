/*
 * columns.hpp
 *
 *  Created on: 24.06.2016
 *      Author: michael
 */

#ifndef COLUMNS_HPP_
#define COLUMNS_HPP_

#include <string>
#include <vector>

#include <osmium/osm/metadata_options.hpp>
#include <osmium/tags/tags_filter.hpp>

#include "config.hpp"

namespace postgres_drivers {

    namespace detail {
        using table_type_base_type = unsigned char;
        constexpr table_type_base_type table_type_osm_object_mask = 0x10;
        constexpr table_type_base_type table_type_osm_member_mask = 0x20;
    }

    /**
     * \brief Typed enum which defines the geometry type of the table.
     *
     * The types are different from simple features because OSM does not
     * follow OGC Simple Feature Specification. For example, nod
     */
    enum class TableType : detail::table_type_base_type {
        /// other
        OTHER = detail::table_type_osm_object_mask,
        /// nodes with tags
        POINT = detail::table_type_osm_object_mask + 1,
        /// nodes without tags
        UNTAGGED_POINT = detail::table_type_osm_object_mask + 2,
        /// ways
        WAYS_LINEAR = detail::table_type_osm_object_mask + 3,
        /// ways which are polygons
        WAYS_POLYGON = detail::table_type_osm_object_mask + 4,
        /// relations which are multipolygons
        RELATION_POLYGON = detail::table_type_osm_object_mask + 5,
        /// relations
        RELATION_OTHER = detail::table_type_osm_object_mask + 6,
        /// areas (ways and multipolygon/boundary relations)
        AREA = detail::table_type_osm_object_mask + 7,
        /// mapping of nodes to ways using them
        NODE_WAYS = detail::table_type_osm_member_mask,
        /// mapping of nodes to relations using them
        RELATION_MEMBER_NODES = detail::table_type_osm_member_mask + 1,
        /// mapping of ways to relations using them
        RELATION_MEMBER_WAYS = detail::table_type_osm_member_mask + 2
    };

    inline bool is_osm_object_table_type(const TableType type) noexcept {
        return (static_cast<detail::table_type_base_type>(type) & detail::table_type_osm_object_mask) == detail::table_type_osm_object_mask;
    }

    enum class ColumnType : char {
        NONE = 0,
        SMALLINT = 1,
        INT = 2,
        BIGINT = 3,
        BIGINT_ARRAY = 4,
        REAL = 5,
        CHAR = 6,
        CHAR_ARRAY = 7,
        TEXT = 8,
        TEXT_ARRAY = 9,
        HSTORE = 50,
        GEOMETRY = 100,
        POINT = 101,
        MULTIPOINT = 102,
        LINESTRING = 103,
        MULTILINESTRING = 104,
        POLYGON = 105,
        MULTIPOLYGON = 106,
        GEOMETRYCOLLECTION = 107
    };

    enum class ColumnClass : char {
        NONE = 0,
        TAGS_OTHER = 1,
        TAG = 2,
        OSM_ID = 3,
        VERSION = 4,
        UID = 5,
        USERNAME = 6,
        CHANGESET = 7,
        TIMESTAMP = 8,
        GEOMETRY = 9,
        WAY_NODES = 10,
        MEMBER_IDS = 11,
        MEMBER_TYPES = 12,
        MEMBER_ROLES = 13,
        GEOMETRY_MULTIPOINT = 14,
        GEOMETRY_MULTILINESTRING = 15,
        OTHER = 16,
        INTERPOLATION_HOUSENUMBER = 17,
        INTERPOLATION_STREET = 18,
        INTERPOLATION_PLACE = 19,
        INTERPOLATION_SUBURB = 20,
        INTERPOLATION_POSTCODE = 21,
        INTERPOLATION_CITY = 22,
        INTERPOLATION_PROVINCE = 23,
        INTERPOLATION_STATE = 24,
        INTERPOLATION_COUNTRY = 25,
        NODE_ID = 26,
        WAY_ID = 27,
        LONGITUDE = 28,
        LATITUDE = 29,
        ROLE = 30
    };

    inline std::string column_type_to_str(const ColumnType c, const int epsg = 0) {
        switch (c) {
        case ColumnType::SMALLINT:
            return "smallint";
        case ColumnType::INT:
            return "int";
        case ColumnType::BIGINT:
            return "bigint";
        case ColumnType::BIGINT_ARRAY:
            return "bigint[]";
        case ColumnType::REAL:
            return "real";
        case ColumnType::CHAR:
            return "char(1)";
        case ColumnType::CHAR_ARRAY:
            return "char(1)[]";
        case ColumnType::TEXT:
            return "text";
        case ColumnType::TEXT_ARRAY:
            return "text[]";
        case ColumnType::POINT:
            if (epsg != 0) {
                return "geometry(Point)";
            }
            return "geometry(Point, " + std::to_string(epsg) + ")";
        case ColumnType::MULTIPOINT:
            if (epsg != 0) {
                return "geometry(MultiPoint)";
            }
            return "geometry(PMultioint, " + std::to_string(epsg) + ")";
        case ColumnType::LINESTRING:
            if (epsg != 0) {
                return "geometry(LineString)";
            }
            return "geometry(LineString, " + std::to_string(epsg) + ")";
        case ColumnType::MULTILINESTRING:
            if (epsg != 0) {
                return "geometry(MultiLineString)";
            }
            return "geometry(MultiLineString, " + std::to_string(epsg) + ")";
        case ColumnType::POLYGON:
            if (epsg != 0) {
                return "geometry(Polygon)";
            }
            return "geometry(Polygon, " + std::to_string(epsg) + ")";
        case ColumnType::MULTIPOLYGON:
            if (epsg != 0) {
                return "geometry(MultiPolygon)";
            }
            return "geometry(MultiPolygon, " + std::to_string(epsg) + ")";
        case ColumnType::GEOMETRY:
            if (epsg != 0) {
                return "geometry(Geometry)";
            }
            return "geometry(Geometry, " + std::to_string(epsg) + ")";
        case ColumnType::GEOMETRYCOLLECTION:
            if (epsg != 0) {
                return "geometry(GeometryCollection)";
            }
            return "geometry(GeometryCollection, " + std::to_string(epsg) + ")";
        case ColumnType::HSTORE:
            return "hstore";
        default:
            return "";
        }
    }


    class Column {
        std::string m_name;
        ColumnType m_type;
        int m_epsg;
        ColumnClass m_column_class;

    public:

        Column(std::string name, ColumnType type, ColumnClass column_class) :
            m_name(name),
            m_type(type),
            m_epsg(0),
            m_column_class(column_class) {
        }

        Column(std::string name, ColumnType type, int epsg, ColumnClass column_class) :
            m_name(name),
            m_type(type),
            m_epsg(epsg),
            m_column_class(column_class) {
        }

        Column(std::string name, ColumnType type, int epsg) :
            m_name(name),
            m_type(type),
            m_epsg(epsg),
            m_column_class(ColumnClass::GEOMETRY) {
        }

        const std::string pg_type() const {
            return column_type_to_str(m_type, m_epsg);
        }

        const std::string& name() const {
            return m_name;
        }

        ColumnType type() const {
            return m_type;
        }

        int epsg() const {
            return m_epsg;
        }

        bool tag_column() const {
            return m_column_class == ColumnClass::TAG;
        }

        ColumnClass column_class() const {
            return m_column_class;
        }
    };

    inline int compare_string_charptr(const std::string& lhs, const char* rhs) {
        return lhs.compare(rhs) < 0;
    }

    inline int compare_strings(const std::string& lhs, const std::string& rhs) {
        return lhs.compare(rhs) < 0;
    }

    using ColumnsVector = std::vector<Column>;

    using ColumnsIterator = ColumnsVector::iterator;
    using ColumnsConstIterator = ColumnsVector::const_iterator;

    /**
     * \brief This class holds the names and types of the columns of a database table.
     *
     * This class implements the iterator pattern. The provided iterator works like an STL iterator.
     *
     * \todo Add method which reads additional columns config from file and returns an instance of Columns.
     */
    class Columns {
    private:
        ColumnsVector m_columns;
        osmium::TagsFilter m_tags_filter;
        osmium::TagsFilter m_drop_filter;
        osmium::TagsFilter m_nocolumn_filter;
        TableType m_type;

        void add_hstore_column(Config& config, TableType type) {
            if (config.tags_hstore) {
                m_columns.emplace_back("tags", ColumnType::HSTORE, ColumnClass::TAGS_OTHER);
            }
        }

    public:
        Columns() = delete;

        Columns(Config& config, TableType type):
                m_columns(),
                m_tags_filter(false),
                m_drop_filter(false),
                m_nocolumn_filter(false),
                m_type(type)/*,
                m_tags()*/ {
            init(config, type);
        }

        void init(Config& config, TableType type) {
            if (is_osm_object_table_type(type)) {
                m_columns.emplace_back("osm_id", ColumnType::BIGINT, ColumnClass::OSM_ID);
                if (config.metadata.user()) {
                    m_columns.emplace_back("osm_user", ColumnType::TEXT, ColumnClass::USERNAME);
                }
                if (config.metadata.uid()) {
                    m_columns.emplace_back("osm_uid", ColumnType::BIGINT, ColumnClass::UID);
                }
                if (config.metadata.version()) {
                    m_columns.emplace_back("osm_version", ColumnType::INT, ColumnClass::VERSION);
                }
                if (config.metadata.timestamp()) {
                    m_columns.emplace_back("osm_lastmodified", ColumnType::TEXT, ColumnClass::TIMESTAMP);
                }
                if (config.metadata.changeset()) {
                    m_columns.emplace_back("osm_changeset", ColumnType::BIGINT, ColumnClass::CHANGESET);
                }
            }
            switch (type) {
            case TableType::POINT :
                m_columns.emplace_back("geom", ColumnType::POINT, 4326);
                add_hstore_column(config, type);
                break;
            case TableType::UNTAGGED_POINT :
                m_columns.emplace_back("x", ColumnType::INT, ColumnClass::LONGITUDE);
                m_columns.emplace_back("y", ColumnType::INT, ColumnClass::LATITUDE);
                break;
            case TableType::WAYS_LINEAR :
                m_columns.emplace_back("geom", ColumnType::LINESTRING, 4326);
                add_hstore_column(config, type);
                break;
            case TableType::WAYS_POLYGON :
                m_columns.emplace_back("geom", ColumnType::MULTIPOLYGON, 4326);
                add_hstore_column(config, type);
                break;
            case TableType::RELATION_POLYGON :
                m_columns.emplace_back("geom", ColumnType::MULTIPOLYGON, 4326);
                add_hstore_column(config, type);
//                if (config.updateable) {
//                    m_columns.emplace_back("member_ids", ColumnType::BIGINT_ARRAY);
//                    m_columns.emplace_back("member_types", ColumnType::CHAR_ARRAY);
//                }
                break;
            case TableType::RELATION_OTHER :
                m_columns.emplace_back("geom_points", ColumnType::MULTIPOINT, 4326, ColumnClass::GEOMETRY_MULTIPOINT);
                m_columns.emplace_back("geom_lines", ColumnType::MULTILINESTRING, 4326, ColumnClass::GEOMETRY_MULTILINESTRING);
                m_columns.emplace_back("member_ids", ColumnType::BIGINT_ARRAY, ColumnClass::MEMBER_IDS);
                m_columns.emplace_back("member_types", ColumnType::CHAR_ARRAY, ColumnClass::MEMBER_TYPES);
                m_columns.emplace_back("member_roles", ColumnType::TEXT_ARRAY, ColumnClass::MEMBER_ROLES);
                add_hstore_column(config, type);
                break;
            case TableType::AREA :
                m_columns.emplace_back("geom", ColumnType::MULTIPOLYGON, 4326);
                add_hstore_column(config, type);
                break;
            case TableType::NODE_WAYS :
                m_columns.emplace_back("way_id", ColumnType::BIGINT, ColumnClass::OSM_ID);
                m_columns.emplace_back("node_id", ColumnType::BIGINT, ColumnClass::NODE_ID);
                m_columns.emplace_back("position", ColumnType::SMALLINT, ColumnClass::OTHER);
                break;
            case TableType::RELATION_MEMBER_NODES :
                m_columns.emplace_back("node_id", ColumnType::BIGINT, ColumnClass::NODE_ID);
                m_columns.emplace_back("relation_id", ColumnType::BIGINT, ColumnClass::OSM_ID);
                m_columns.emplace_back("position", ColumnType::SMALLINT, ColumnClass::OTHER);
                m_columns.emplace_back("role", ColumnType::TEXT, ColumnClass::ROLE);
                break;
            case TableType::RELATION_MEMBER_WAYS :
                m_columns.emplace_back("way_id", ColumnType::BIGINT, ColumnClass::WAY_ID);
                m_columns.emplace_back("relation_id", ColumnType::BIGINT, ColumnClass::OSM_ID);
                m_columns.emplace_back("position", ColumnType::SMALLINT, ColumnClass::OTHER);
                m_columns.emplace_back("role", ColumnType::TEXT, ColumnClass::ROLE);
                break;
            case TableType::OTHER :
                break;
            }
        }

        Columns(Config& config, ColumnsVector& additional_columns, osmium::TagsFilter drop_filter,
                std::vector<std::string>& nocolumn_keys, TableType type) :
            m_columns(),
            m_tags_filter(false),
            m_drop_filter(drop_filter),
            m_nocolumn_filter(),
            m_type(type)/*,
            m_tags()*/ {
            init(config, type);
            for (Column& col : additional_columns) {
                m_columns.push_back(col);
                if (col.tag_column()) {
                    m_tags_filter.add_rule(true, col.name());
                }
            }
            for (auto& k : nocolumn_keys) {
                m_tags_filter.add_rule(true, k);
            }
        }

        Columns(Config& config, ColumnsVector&& additional_columns) :
            m_columns(),
            m_tags_filter(false),
            m_drop_filter(false),
            m_nocolumn_filter(),
            m_type(TableType::OTHER) {
            for (Column& col : additional_columns) {
                m_columns.push_back(col);
                if (col.tag_column()) {
                    m_tags_filter.add_rule(true, col.name());
                }
            }
        }

        static ColumnsVector addr_interpolation_columns() {
            return ColumnsVector{
                // We call it osm_id to make the application of diff updates work without modifications.
                Column("osm_id", ColumnType::INT, ColumnClass::OSM_ID),
                Column("housenumber", ColumnType::TEXT, ColumnClass::INTERPOLATION_HOUSENUMBER),
                Column("street", ColumnType::TEXT, ColumnClass::INTERPOLATION_STREET),
                Column("place", ColumnType::TEXT, ColumnClass::INTERPOLATION_PLACE),
                Column("suburb", ColumnType::TEXT, ColumnClass::INTERPOLATION_SUBURB),
                Column("postcode", ColumnType::TEXT, ColumnClass::INTERPOLATION_POSTCODE),
                Column("city", ColumnType::TEXT, ColumnClass::INTERPOLATION_CITY),
                Column("province", ColumnType::TEXT, ColumnClass::INTERPOLATION_PROVINCE),
                Column("state", ColumnType::TEXT, ColumnClass::INTERPOLATION_STATE),
                Column("country", ColumnType::TEXT, ColumnClass::INTERPOLATION_COUNTRY),
                Column("geom", ColumnType::POINT, ColumnClass::GEOMETRY)
            };
        }

        ColumnsConstIterator cbegin() const {
            return m_columns.begin();
        }

        ColumnsConstIterator cend() const {
            return m_columns.end();
        }

        const Column& front() const {
            return m_columns.front();
        }

        const Column& back() const {
            return m_columns.back();
        }

        const Column& at(size_t n) const {
            return m_columns.at(n);
        }

        ColumnsIterator begin() {
            return m_columns.begin();
        }

        ColumnsIterator end() {
            return m_columns.end();
        }

        ColumnsConstIterator begin() const {
            return m_columns.begin();
        }

        ColumnsConstIterator end() const {
            return m_columns.end();
        }

        Column& front() {
            return m_columns.front();
        }

        Column& back() {
            return m_columns.back();
        }

        Column& at(size_t n) {
            return m_columns.at(n);
        }

        /**
         * \brief Get number of columns of this table.
         */
        size_t size() const {
            return m_columns.size();
        }

        /**
         * \brief Get number of columns of this table.
         */
        size_t size() {
            return m_columns.size();
        }

        /**
         * \brief Get geometry type of this table.
         */
        TableType get_type() {
            return m_type;
        }

        const osmium::TagsFilter& filter() const {
            return m_tags_filter;
        }

        const osmium::TagsFilter& drop_filter() const {
            return m_drop_filter;
        }
    };
}


#endif /* COLUMNS_HPP_ */
