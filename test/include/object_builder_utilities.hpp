/*
 * object_builder_utilities.hpp
 *
 *  Created on: 12.10.2016
 *      Author: michael
 */

/**
 * Helper methods used by various tests to build OSM objects with libosmium.
 */

#ifndef TEST_INCLUDE_OBJECT_BUILDER_UTILITIES_HPP_
#define TEST_INCLUDE_OBJECT_BUILDER_UTILITIES_HPP_

#include <map>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/builder/osm_object_builder.hpp>

using tagmap = std::map<std::string, std::string>;

namespace test_utils {

    void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder, tagmap& tags) {
        osmium::builder::TagListBuilder tl_builder(buffer, &builder);
        for (tagmap::const_iterator it = tags.begin(); it != tags.end(); it++) {
            tl_builder.add_tag(it->first, it->second);
        }
    }

    void set_dummy_osm_object_attributes(osmium::OSMObject& object) {
        object.set_version("1");
        object.set_changeset("5");
        object.set_uid("140");
        object.set_timestamp(osmium::Timestamp("2016-01-05T01:22:45Z"));
    }

    osmium::Node& create_new_node(osmium::memory::Buffer& buffer, osmium::object_id_type id, double lon, double lat,
            tagmap& tags) {
        osmium::builder::NodeBuilder node_builder(buffer);
        static_cast<osmium::Node&>(node_builder.object()).set_id(id);
        osmium::Node& node = static_cast<osmium::Node&>(node_builder.object());
        set_dummy_osm_object_attributes(static_cast<osmium::OSMObject&>(node_builder.object()));
        node_builder.add_user("foo");
        osmium::Location location (lon, lat);
        node.set_location(location);
        add_tags(buffer, node_builder, tags);

        buffer.commit();
        return node_builder.object();
    }

}


#endif /* TEST_INCLUDE_OBJECT_BUILDER_UTILITIES_HPP_ */
