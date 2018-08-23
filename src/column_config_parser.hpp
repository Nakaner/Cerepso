/*
 * column_config_parser.hpp
 *
 *  Created on:  2018-08-17
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef COLUMN_CONFIG_PARSER_HPP_
#define COLUMN_CONFIG_PARSER_HPP_

#include <vector>

#include <postgres_drivers/columns.hpp>

#include "cerepsoconfig.hpp"
#include "postgres_table.hpp"

enum class ColumnConfigFlag : char {
    LINEAR = 1,
    POLYGON = 2,
    NOCOLUMN = 3,
    DELETE = 4
};

/**
 * class to parse Osm2pgsql style file
 */
class ColumnConfigParser {

    std::string m_filename;

    CerepsoConfig& m_config;

    postgres_drivers::ColumnsVector m_point_columns;
    postgres_drivers::ColumnsVector m_line_columns;
    postgres_drivers::ColumnsVector m_polygon_columns;
    std::vector<std::string> m_nocolumn_keys;
    osmium::TagsFilter m_drop_filter;

    template <typename... TArgs>
    void add_column(postgres_drivers::ColumnsVector& vec, TArgs&&... args) {
        vec.emplace_back(std::forward<TArgs>(args)...);
    }

    std::vector<ColumnConfigFlag> parse_flags(const std::string& flags);

public:
    ColumnConfigParser() = delete;

    ColumnConfigParser(CerepsoConfig& config);

    static postgres_drivers::ColumnType str_to_column_type(const std::string& type);

    void parse();

    PostgresTable make_point_table(const char* prefix);

    PostgresTable make_line_table(const char* prefix);

    PostgresTable make_polygon_table(const char* prefix);

    std::vector<std::string>& nocolumn_keys();

    osmium::TagsFilter drop_filter();
};



#endif /* COLUMN_CONFIG_PARSER_HPP_ */
