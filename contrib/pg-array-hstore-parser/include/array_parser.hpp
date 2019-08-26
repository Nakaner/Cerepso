/*
 * array_parser.hpp
 *
 *  Created on: 2016-11-21
 *      Author: Michael Reichert
 */

#ifndef SRC_ARRAY_PARSER_HPP_
#define SRC_ARRAY_PARSER_HPP_

#include <assert.h>
#include "type_conversion.hpp"
#include "postgres_parser.hpp"

namespace pg_array_hstore_parser {
    using ArrayPartsType = char;

    /**
     * \brief track where we are during parsing an array
     */
    enum class ArrayParts : ArrayPartsType {
        NONE = 0,
        ELEMENT = 1,
        END = 2
    };

    /**
     * \brief Class to parse one dimensional arrays received from a PostgreSQL database query response which are encoded as strings.
     *
     * This class needs a type conversion implementation as template argument. Use the aliases defined by type_conversion.hpp.
     *
     * See tests/t/test_array_parser.cpp for usage examples of this class.
     *
     * \todo use C strings instead of std::string
     */
    template <typename TConversion>
    class ArrayParser : public PostgresParser<typename TConversion::output_type> {

    protected:
        /**
         * \brief length of the string to be parsed
         *
         * We store this in a special variable to reduce the number of calls of string::size().
         */
        size_t m_max_length;

        /// shortcut
        using postgres_parser_type = PostgresParser<typename TConversion::output_type>;

        /// track progress of hstore parsing
        ArrayParts m_parse_progress = ArrayParts::NONE;

        TConversion m_type_conversion;

        /**
         * Current position inside #m_string_repr
         *
         * We always start parsing at the second character (i.e. index 1).
         */
        size_t m_current_position = 1;

        /**
         * Check if we have reached the end of the element of an arry.
         *
         * \param next_char next character of the string
         * \param inside_quotation_marks true if the character before was inside a "" section; false otherwise
         *
         * \returns true if next_char does not belong to an element of the array any more (i.e. ',' outside
         *          '""' or '}' outside '""').
         */
        bool element_finished(const char next_char, const bool inside_quotation_marks) {
            if (inside_quotation_marks && next_char == '"') {
                return true;
            } else if (!inside_quotation_marks && next_char == ',') {
                return true;
            } else if (!inside_quotation_marks && next_char == '}') {
                return true;
            }
            return false;
        }

        void invalid_syntax(std::string error) {
            std::string message = "Invalid array syntax at character ";
            message += std::to_string(m_current_position + 1);
            message += " of \"";
            message += postgres_parser_type::m_string_repr;
            message += "\". \"\\";
            message += postgres_parser_type::m_string_repr.at(m_current_position);
            message += "\" ";
            message += error;
            message += '\n';
            throw std::runtime_error(message);
        }

        /**
         * \brief Build the string which will be returned by get_next().
         *
         * This method goes from a given position to another given position over the string representation of the
         * array and copies it to a new string.
         *
         * This is much faster than calling std::string::push_back(char) at evey character which is not escaped during
         * parsing the array character by character. If we used push_back(char), we would std::string::reserve() to often.
         * This implementation (remove_escape_sequences) reserves the necessary amount of memory at the beginning (usually
         * a few bytes more than necessary).
         *
         * \param source source string to copy from
         * \param start first character to be copied
         * \param end last character to be copied
         *
         * \returns array element as string without surrounding double quotes
         *
         * \throws std::runtime_error in the case of invalid escape sequences
         */
        std::string remove_escape_sequences(const std::string& source, const size_t start, const size_t end) {
            if (start > end) { // indicator for an empty string as array element
                return "";
            }
            std::string result (end - start + 1, '\0'); // initial fill because we know the maximum size
            size_t current_position_result = 0;
            size_t current_position_src = start;
            bool backslashes_before = false;

            while (current_position_result <= end - start && current_position_src <= end) {
                if (backslashes_before) {
                    // Handling of escaped characters. Elements containing escape sequences must be surrounded by double quotes.
                    switch (source.at(current_position_src)) {
                    case '"':
                        result.at(current_position_result) = '"';
                        backslashes_before = false;
                        current_position_result++;
                        break;
                    case '\\':
                        result.at(current_position_result) = '\\';
                        backslashes_before = false;
                        current_position_result++;
                        break;
                    default:
                        invalid_syntax("is no valid escape sequence in a hstore key or value.");
                    }
                } else {
                    switch (source.at(current_position_src)) {
                    case '\\':
                        backslashes_before = true;
                        break;
                    default:
                        result.at(current_position_result) = source.at(current_position_src);
                        current_position_result++;
                    }
                }
                current_position_src++;
            }
            result.resize(current_position_result);
            return result;
        }

    public:
        ArrayParser(const std::string& string_repr) : PostgresParser<typename TConversion::output_type>(string_repr) {
            m_max_length = string_repr.length();
        };

        /**
         * \brief Has the parser reached the end of the hstore?
         */
        bool has_next() {
            if (m_current_position >= m_max_length - 1) {
                // We have reached the last character which is a '}'.
                return false;
            }
            if (postgres_parser_type::m_string_repr.at(m_current_position) == '}') {
                return false;
            }
            return true;
        }

        /**
         * \brief Return the next key value pair as a pair of strings.
         *
         * Parsing follows following rules of the Postgres documentation:
         * The array output routine will put double quotes around element values if they are empty strings,
         * contain curly braces, delimiter characters, double quotes, backslashes, or white space, or match
         * the word NULL. Double quotes and backslashes embedded in element values will be backslash-escaped.
         */
        typename TConversion::output_type get_next() {
            m_parse_progress = ArrayParts::NONE;
            bool backslashes_before = false; // counts preceding backslashes
            bool quoted_element = false; // track if the key/value is surrounded by quotation marks
            size_t start = 0;
            size_t end = 0;
            while (m_parse_progress != ArrayParts::END && m_current_position < m_max_length) {
                if (m_parse_progress == ArrayParts::NONE) {
                    if (postgres_parser_type::m_string_repr.at(m_current_position) == '"') {
                        quoted_element = true;
                        start = m_current_position + 1;
                        m_parse_progress = ArrayParts::ELEMENT;
                    } else if (postgres_parser_type::m_string_repr.at(m_current_position) != ','
                            && postgres_parser_type::m_string_repr.at(m_current_position) != ' '){
                        m_parse_progress = ArrayParts::ELEMENT;
                        start = m_current_position;
                    }
                } else if (m_parse_progress == ArrayParts::ELEMENT && quoted_element) {
                    // Handling of array elements surrounded by double quotes
                    if (backslashes_before) {
                        // Handling of escaped characters. Elements containing escape sequences must be surrounded by double quotes.
                        switch (postgres_parser_type::m_string_repr.at(m_current_position)) {
                        case '"':
                        case '\\':
                            backslashes_before = false;
                            break;
                        default:
                            invalid_syntax("is no valid escape sequence in a hstore key or value.");
                        }
                    } else if (postgres_parser_type::m_string_repr.at(m_current_position) == '\\') {
                        backslashes_before = true;
                    } else if (postgres_parser_type::m_string_repr.at(m_current_position) == '"') {
                        end = m_current_position - 1;
                        m_parse_progress = ArrayParts::END;
                    } else {
                    }
                } else if (m_parse_progress == ArrayParts::ELEMENT && !quoted_element) {
                    // Handling of array elements not surrounded by double quotes
                    switch (postgres_parser_type::m_string_repr.at(m_current_position)) {
                    case ' ':
                        end = m_current_position - 1;
                        // We will go on until we reach the comma. Therefore, we ignore this space and don't report a syntax error
                        // if there is a space in an array element which is not surrounded by double quotes.
                        break;
                    case ',':
                    case '}':
                        if (end == 0) { // end has still its default value
                            end = m_current_position - 1;
                        }
                        m_parse_progress = ArrayParts::END;
                        break;
                    }
                }
                m_current_position++;
            }
            std::string to_convert = remove_escape_sequences(postgres_parser_type::m_string_repr, start, end);
            if (!quoted_element && to_convert == "NULL") {
                return m_type_conversion.return_null_value();
            }
            return m_type_conversion.to_output_format(to_convert);
        }
    };
} // namespace pg_array_hstore_parser

#endif /* SRC_ARRAY_PARSER_HPP_ */
