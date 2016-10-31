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
#include "config.hpp"

/**
 * \brief Typed enum which defines the geometry type of the table.
 *
 * The types are different from simple features because OSM does not
 * follow OGC Simple Feature Specification. For example, nod
 */
enum class TableType : char {
    /// nodes with tags
    POINT = 1,
    /// nodes without tags
    UNTAGGED_POINT = 2,
    /// ways
    WAYS_LINEAR = 3,
    /// ways which are polygons
    WAYS_POLYGON = 4,
    /// relations which are multipolygons
    RELATION_POLYGON = 5,
    /// relations
    RELATION_OTHER = 6
};

typedef std::pair<const std::string, const std::string> Column;
typedef std::vector<Column> ColumnsVector;
using ColumnsIterator = ColumnsVector::iterator;

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
    TableType m_type;

public:
    Columns() = delete;

    Columns(Config& config, TableType type);

    Columns(Config& config, ColumnsVector& additional_columns, TableType type);


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

    /**
     * \brief Get number of columns of this table.
     */
    int size() {
        return m_columns.size();
    }

    /**
     * \brief Get name of n-th (0 is first) column.
     *
     * \param n column index
     *
     * \returns column name
     */
    const std::string& column_name_at(size_t n) {
        return m_columns.at(n).first;
    }

    /**
     * \brief Get type of n-th (0 is first) column.
     *
     * \param n column index
     *
     * \returns column type
     */
    const std::string& column_type_at(size_t n) {
        return m_columns.at(n).second;
    }

    /**
     * \brief Get geometry type of this table.
     */
    TableType get_type() {
        return m_type;
    }
};



#endif /* COLUMNS_HPP_ */
