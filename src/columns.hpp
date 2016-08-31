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

enum class TableType : char {
    POINT = 1,
    UNTAGGED_POINT = 2,
    WAYS_LINEAR = 3,
    WAYS_POLYGON = 4,
    RELATION_POLYGON = 5,
    RELATION_OTHER = 6
};

/**
 * program configuration
 */
struct Config {
    bool m_debug = false;
    std::string m_osm_file = "";
    std::string m_database_name = "pgimportertest";
    bool tags_hstore = true;
    bool metadata = true;
    bool m_all_geom_indexes = false;
    bool m_geom_indexes = true;
    bool m_order_by_geohash = true;
    bool m_append = false;
    bool m_id_index = true;
    bool m_usernames = true;
    std::string m_location_handler = "sparse_mmap_array";
};

typedef std::pair<const std::string, const std::string> Column;
typedef std::vector<Column> ColumnsVector;

using ColumnsIterator = ColumnsVector::iterator;

class Columns {
private:
    ColumnsVector m_columns;
    TableType m_type;

public:
    Columns() = delete;

    Columns(Config& config, TableType type);

    Columns(Config& config, ColumnsVector& additional_columns, TableType type);

    //TODO add method which reads additional columns config from file and returns an instance of Columns

    ColumnsIterator begin() {
        return m_columns.begin();
    }

    ColumnsIterator end() {
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

    int size() {
        return m_columns.size();
    }

    /**
     * returns name of n-th (0 is first) column
     */
    const std::string& column_name_at(size_t n) {
        return m_columns.at(n).first;
    }

    /**
     * returns type of n-th (0 is first) column
     */
    const std::string& column_type_at(size_t n) {
        return m_columns.at(n).second;
    }

    TableType get_type() {
        return m_type;
    }
};



#endif /* COLUMNS_HPP_ */
