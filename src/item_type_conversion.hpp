/*
 * item_type_conversion.hpp
 *
 *  Created on:  2016-11-28
 *      Author: Michael Reichert
 */

#ifndef SRC_ITEM_TYPE_CONVERSION_HPP_
#define SRC_ITEM_TYPE_CONVERSION_HPP_

#include <type_conversion.hpp>
#include <osmium/osm/item_type.hpp>


/**
 * \brief TypeConversionImpl implementation to be used with TypeConversion class if the output format should be osmium::item_type.
 *
 * This class extends the functionality of the pg-array-hstore-parser library. This class is not shipped with that library because
 * it is Osmium-specific and Osmium should not be a dependency of that library.
 */
class ItemTypeConversionImpl {
public:
    using output_type = osmium::item_type;

    /**
     * \brief Convert to the output format (osmium::item_type).
     *
     * \param str string to be converted
     *
     * \return str converted to the output format
     */
    output_type to_output_format(std::string& str) {
        if (str.length() > 1) {
            std::string message = "\"";
            message += str;
            message += "\" is no valid encoding for osmium::item_type.\n";
            throw std::runtime_error(message);
        }
        switch (str.at(0)) {
        case 'n':
            return osmium::item_type::node;
        case 'w':
            return osmium::item_type::way;
        case 'r':
            return osmium::item_type::relation;
        default:
            return return_null_value();
        }
    }

    /**
     * \brief Return the NULL value of output_type.
     *
     * This method is called if the database returns NULL (not "NULL").
     *
     * \returns osmium::item_type::undefined
     */
    output_type return_null_value() {
        return osmium::item_type::undefined;
    }
};

using ItemTypeConversion = pg_array_hstore_parser::TypeConversion<ItemTypeConversionImpl>;

#endif /* SRC_ITEM_TYPE_CONVERSION_HPP_ */
