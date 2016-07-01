/*
 * table.hpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <libpq-fe.h>
#include <boost/format.hpp>
#include "columns.hpp"

/**
 * This class manages connection to a database table. We have one connection per table,
 * therefore this class is called Table, not DBConnection.
 */

class Table {
private:
    /**
     * name of the table
     */
    std::string m_name;

    bool m_copy_mode = false;

    Config& m_config;

    Columns& m_columns;

    /**
     * connection
     */
    PGconn *m_database_connection;

    void start_copy();

    void end_copy();

    void send_begin();

    void send_query(const char* query);

public:
    Table() = delete;

    Table(const char* table_name, Config& config, Columns& columns);

    ~Table();

    /**
     * send a line to the database (it will get it from STDIN) during copy mode
     */
    void send_line(const std::string& line);

    Columns& get_columns();

    /**
     * escape a string which should be inserted into a hstore column
     *
     * @param source string which should be escaped
     * @param destination string where the escaped string has to be appended (later usually used for INSERT query)
     */
    static void escape4hstore(const char* source, std::stringstream& destination);

};


#endif /* TABLE_HPP_ */
