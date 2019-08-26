/*
 * type_conversion.hpp
 *
 *  Created on: 2016-11-21
 *      Author: Michael Reichert
 */

#ifndef SRC_TYPE_CONVERSION_HPP_
#define SRC_TYPE_CONVERSION_HPP_

#include <string>

namespace pg_array_hstore_parser {
    /**
     * \brief TypeConversionImpl implementation to be used with TypeConversion class if the output format should be std::string.
     *
     * This implements the Null Object Pattern.
     */
    class StringConversionImpl {
    public:
        using output_type = std::string;

        output_type to_output_format(std::string& str) {
            return str;
        }

        /**
         * \brief Return the NULL value of output_type. That's an empty string.
         *
         * This method is called if the database returns NULL (not "NULL").
         *
         * \returns empty string
         */
        output_type return_null_value() {
            return "";
        }
    };

    /**
     * \brief TypeConversionImpl implementation to be used with TypeConversion class if the output format should be char.
     */
    class CharConversionImpl {
    public:
        using output_type = char;

        /**
         * \brief Convert to the output format.
         *
         * \param str string to be converted
         *
         * \return first character of the string. If the string is empty, return_null_value() is called.
         */
        output_type to_output_format(std::string& str) {
            if (str.length() > 0) {
                return str.at(0);
            }
            return return_null_value();
        }

        /**
         * \brief Return the NULL value of output_type. That's an empty string.
         *
         * This method is called if the database returns NULL (not "NULL").
         *
         * \returns a null character (\0)
         */
        output_type return_null_value() {
            return '\0';
        }
    };

    /**
     * \brief TypeConversionImpl implementation to be used with the TypeConversion class if the output format should be int64_t.
     */
    class Int64ConversionImpl {
    public:
        using output_type = int64_t;

        output_type to_output_format(std::string& str) {
            return std::strtoll(str.c_str(), NULL, 10);
        }

        /**
         * \brief Return the NULL value of output_type. That's 0.
         *
         * This method is called if the database returns NULL (not "NULL").
         *
         * \returns 0
         */
        output_type return_null_value() {
            return 0;
        }
    };

    /**
     * \brief Template class for type conversions during parsing arrays received from a PostgreSQL query response (arrays are
     * represented as strings).
     *
     * This uses the same pattern as Osmium's osmium::geom::GeometryFactory class.
     * https://github.com/osmcode/libosmium/blob/master/include/osmium/geom/factory.hpp
     */
    template <typename TypeConversionImpl>
    class TypeConversion {
        TypeConversionImpl m_impl;

    public:
        using output_type = typename TypeConversionImpl::output_type;

        /**
         * \brief Convert to the output format.
         *
         * \param str string to be converted
         *
         * \return str converted to the output format
         */
        output_type to_output_format(std::string& str) {
            return m_impl.to_output_format(str);
        }

        /**
         * \brief Return the NULL value of output_type.
         *
         * This method is called if the database returns NULL (not "NULL").
         */
        output_type return_null_value() {
            return m_impl.return_null_value();
        }
    };

    using StringConversion = TypeConversion<StringConversionImpl>;

    using CharConversion = TypeConversion<CharConversionImpl>;

    using Int64Conversion = TypeConversion<Int64ConversionImpl>;
} // namespace pg_array_hstore_parser

#endif /* SRC_TYPE_CONVERSION_HPP_ */
