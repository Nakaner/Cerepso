/*
 * table.hpp
 *
 *  Created on: 23.06.2016
 *      Author: michael
 */

#ifndef POSTGRES_TABLE_HPP_
#define POSTGRES_TABLE_HPP_

//#include <sstream>
#include <string>
#include <postgres_drivers/table.hpp>
#include "../cerepsoconfig.hpp"

/**
 * \brief This class manages connection to a database table. We have one connection per table,
 * therefore this class is called PostgresTable, not DBConnection.
 */
class PostgresTable : public postgres_drivers::Table {

    /**
     * A PostgreSQL Prepared Statement
     */
    struct PreparedStatement {
        /// name of the statement
        std::string name;

        /// the query
        std::string query;

        /// number of parameters
        int param_count;

        PreparedStatement() = delete;

        PreparedStatement(std::string name, std::string query, int param_count) :
            name(std::move(name)),
            query(std::move(query)),
            param_count(param_count) {
        }
    };

    /// \brief reference to program configuration
    CerepsoConfig& m_program_config;

    /**
     * Prepared statements which should be created.
     *
     * Classes derived from this class add the prepared statements they need to this vector. This
     * class will then take care to create the statements.
     */
    std::vector<PreparedStatement> m_prepared_statements;

    bool m_initialized = false;

    /**
     * \brief Create index on `osm_id` column.
     */
    void create_id_index();

    /**
     * Create all registered prepared statements.
     */
    void create_prepared_statements();

protected:
    /**
     * Register a prepared statement which should be created.
     *
     * We do not create the prepared statements immediately because this method is called from the
     * constructor of the child classes but the database table is created in init() and init() is
     * inteded to be called by the user of the class. If we wanted to avoid this registration of
     * prepared statements to be created, we would have to use the Curiously recurring template
     * pattern (https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) and inline
     * code in this header file. The performance cost is negligible because the code to create
     * tables is called only once.
     */
    void register_prepared_statement(std::string name, std::string query, int param_count);

    /**
     * Discared all registered prepared statements. This method is useful for children of child
     * classes which have a different set of columns.
     */
    void discard_all_prepared_statements();

    /**
     * Create all the prepared statements a table needs.
     */
    void request_prepared_statements();

    /**
     * Commit open transaction and stop copying if any.
     *
     * Call this function during teardown.
     */
    void stop();

    /**
     * Has the table been initialized?
     */
    bool initialized();

public:
    const CerepsoConfig& config();

    PostgresTable() = delete;

    /**
     * \brief constructor for production, establishes database connection
     */
    PostgresTable(const char* table_name, CerepsoConfig& config, postgres_drivers::Columns columns);

    /**
     * \brief constructor for testing, does not establishes database connection
     */
    PostgresTable(postgres_drivers::Columns& columns, CerepsoConfig& config);

    ~PostgresTable();

    /**
     * Initialize table (create it in the database).
     */
    void init();

    const CerepsoConfig& config() const;

    /**
     * \brief Add a TAB at the end of the string.
     *
     * \param ss String where the TAB should be appended.
     */
    static void add_separator_to_stringstream(std::string& ss);

    /**
     * \brief Add a key or value of an OSM tag to a hstore column and escape forbidden characters before appending.
     *
     * This method is taken from osm2pgsql/table.cpp, void table_t::escape4hstore(const char *src, string& dst).
     * Use it not only to escape forbidden characters but also to prevent SQL injections.
     *
     * \param source C string which to be escaped and appended
     * \param destination string where the escaped string has to be appended (later usually used for INSERT query or COPY)
     */
    static void escape4hstore(const char* source, std::string& destination);

    /**
     * \brief Escape a string from an insecure source and append it to another string.
     *
     * Use this method if you want to insert a string into a PostgreSQL array but have to escape certain characters to
     * prevent SQL injections.
     *
     * This method is based on osm2pgsql/middle-pgsql.cpp, char *escape_tag(char *ptr, const std::string &in, bool escape).
     *
     * \param source C string which should be escaped
     * \param destination string where the escaped string will be appended
     */
    static void escape4array(const char* source, std::string& destination);

    /**
     * \brief Escape a string from an insecure source and append it to another string.
     *
     * Use this method if you want to insert a string into the database but have to escape certain characters to
     * prevent SQL injections.
     *
     * \param source C string which should be escaped
     * \param destination string where the escaped string will be appended
     */
    static void escape(const char* source, std::string& destination);
};


#endif /* POSTGRES_TABLE_HPP_ */
