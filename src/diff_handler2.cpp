/*
 * diff_handler2.cpp
 *
 *  Created on: 04.10.2016
 *      Author: michael
 */

#include <geos/geom/CoordinateArraySequenceFactory.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/io/WKBWriter.h>
#include <geos/geom/LineString.h>
#include <osmium/osm/relation.hpp>

#include "diff_handler2.hpp"
#include "postgres_table.hpp"


DiffHandler2::DiffHandler2(CerepsoConfig& config, PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
        PostgresTable& relations_table, PostgresTable& node_ways_table, PostgresTable& node_relations_table,
        PostgresTable& way_relations_table, ExpireTiles* expire_tiles, UpdateLocationHandler& location_index,
        PostgresTable* areas_table /*= nullptr*/, osmium::area::MultipolygonManager<osmium::area::Assembler>* mp_manager /*= nullptr*/) :
        PostgresHandler(config, nodes_table, untagged_nodes_table, ways_table, nullptr, areas_table, &node_ways_table,
                &node_relations_table, &way_relations_table),
        m_relations_table(relations_table),
        m_location_index(location_index),
        m_expire_tiles(expire_tiles),
        m_pending_ways(),
        m_pending_relations(),
        m_pending_ways_idx(0),
        m_pending_relations_idx(0),
        m_mp_manager(mp_manager),
        m_relation_buffer(100000, osmium::memory::Buffer::auto_grow::yes),
        m_new_areas_buffer(),
        m_updated_areas_buffer(),
#ifdef GEOS_36
        m_geom_factory(geos::geom::GeometryFactory::create().release(), GEOSGeometryFactoryDeleter())
#else
        m_geom_factory(new geos::geom::GeometryFactory{})
#endif
{
    m_untagged_nodes_table->start_copy();
    m_nodes_table.start_copy();
    m_new_areas_buffer.set_callback([&](osmium::memory::Buffer&& buffer) {
        for (auto& item : buffer) {
            if (item.type() == osmium::item_type::area) {
                area(static_cast<const osmium::Area&>(item));
            }
        }
    });
    m_updated_areas_buffer.set_callback([&](osmium::memory::Buffer&& buffer) {
        for (auto& item : buffer) {
            if (item.type() == osmium::item_type::area) {
                update_area_geometry(static_cast<const osmium::Area&>(item));
            }
        }
    });
}

/**
 * \brief constructor for testing purposes, will not establish database connections
 */
DiffHandler2::DiffHandler2(PostgresTable& nodes_table, PostgresTable* untagged_nodes_table, PostgresTable& ways_table,
        PostgresTable& relations_table, PostgresTable& node_ways_table, PostgresTable& node_relations_table,
        PostgresTable& way_relations_table, CerepsoConfig& config, ExpireTiles* expire_tiles,
        UpdateLocationHandler& location_index, PostgresTable* areas_table /*= nullptr*/,
        osmium::area::MultipolygonManager<osmium::area::Assembler>* mp_manager /*= nullptr*/) :
    PostgresHandler(nodes_table, untagged_nodes_table, ways_table, config, nullptr, areas_table, &node_ways_table,
            &node_relations_table, &way_relations_table),
    m_relations_table(relations_table),
    m_location_index(location_index),
    m_expire_tiles(expire_tiles),
    m_pending_ways_idx(0),
    m_pending_relations_idx(0),
    m_mp_manager(mp_manager),
    m_relation_buffer(100000, osmium::memory::Buffer::auto_grow::yes),
    m_new_areas_buffer(),
    m_updated_areas_buffer(),
#ifdef GEOS_36
        m_geom_factory(geos::geom::GeometryFactory::create().release(), GEOSGeometryFactoryDeleter())
#else
        m_geom_factory(new geos::geom::GeometryFactory{})
#endif
{
    m_new_areas_buffer.set_callback([&](osmium::memory::Buffer&& buffer) {
        for (auto& item : buffer) {
            if (item.type() == osmium::item_type::area) {
                area(static_cast<const osmium::Area&>(item));
            }
        }
    });
    m_updated_areas_buffer.set_callback([&](osmium::memory::Buffer&& buffer) {
        for (auto& item : buffer) {
            if (item.type() == osmium::item_type::area) {
                update_area_geometry(static_cast<const osmium::Area&>(item));
            }
        }
    });
}

DiffHandler2::~DiffHandler2() {
    m_expire_tiles->output_and_destroy();
    delete m_expire_tiles;
}

void DiffHandler2::node(const osmium::Node& node) {
    if (node.deleted()) { // we are finish now
        return;
    }
    handle_node(node);
    // check if ways have to be updated
    std::vector<osmium::object_id_type> new_way_ids = m_node_ways_table->get_way_ids(node.id());
    for (auto id : new_way_ids) {
        m_pending_ways.push_back(id);
    }
    // check if relations have to be updated
    std::vector<osmium::object_id_type> rel_ids = m_node_relations_table->get_relation_ids(node.id());
    for (auto id : rel_ids) {
        m_pending_relations.push_back(id);
    }
}

osmium::Location DiffHandler2::get_point_from_tables(osmium::object_id_type id) {
    return m_location_index.get_node_location(id);
}

void DiffHandler2::update_relation(const osmium::object_id_type id) {
    // get relation members from relations table
    std::vector<osmium::item_type> member_types = m_relations_table.get_member_types(id);
    std::vector<osmium::object_id_type> member_ids = m_relations_table.get_member_ids(id);
    assert(member_types.size() == member_ids.size());
    std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
    std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
    std::vector<osmium::item_type>::iterator it_type = member_types.begin();
    std::vector<osmium::object_id_type>::iterator it_id = member_ids.begin();
    for (; it_type != member_types.end(), it_id != member_ids.end(); ++it_type, ++it_id) {
        if (*it_type == osmium::item_type::node) {
            osmium::Location loc = get_point_from_tables(*it_id);
            std::unique_ptr<geos::geom::Point> point (m_geom_factory->createPoint(geos::geom::Coordinate(loc.lon(), loc.lat())));
            points->push_back(point.release());
        } else if (*it_type == osmium::item_type::way) {
            //TODO not DRY with insert_relation()
            std::vector<MemberNode> nodes = m_node_ways_table->get_way_nodes(*it_id);
            std::vector<geos::geom::Coordinate> coordinates;
            coordinates.reserve(nodes.size());
            std::vector<MemberNode>::iterator itn = nodes.begin();
            std::vector<geos::geom::Coordinate>::iterator itc = coordinates.begin();
            for (; itn != nodes.end() && itc != coordinates.end(); ++itn, ++itc) {
                osmium::Location loc = get_point_from_tables(itn->node_ref.ref());
                itc->x = loc.lon();
                itc->y = loc.lat();
            }
            geos::geom::CoordinateArraySequenceFactory coord_sequence_factory;
            std::unique_ptr<geos::geom::CoordinateSequence> coord_sequence {coord_sequence_factory.create(&coordinates)};
            std::unique_ptr<geos::geom::LineString> linestring {m_geom_factory->createLineString(coord_sequence.release())};
            if (linestring) {
                linestrings->push_back(linestring.release());
            }
        }
        // We do not add the geometry of this relation to the GeometryCollection.
        /// \todo support one level of nested relations
    }
    geos::geom::MultiPoint* multipoints = m_geom_factory->createMultiPoint(points);
    geos::geom::MultiLineString* multilinestrings = m_geom_factory->createMultiLineString(linestrings);
    // add multipoints to query string
    // convert to WKB
    std::stringstream multipoint_stream;
    geos::io::WKBWriter wkb_writer;
    wkb_writer.writeHEX(*multipoints, multipoint_stream);
    delete multipoints;
    // add multilinestrings to query string
    // convert to WKB
    std::stringstream multilinestring_stream;
    wkb_writer.writeHEX(*multilinestrings, multilinestring_stream);
    delete multilinestrings;
    m_relations_table.update_relation_member_geometry(id, multipoint_stream.str().c_str(), multilinestring_stream.str().c_str());
    //TODO code before this "if" becomes unnecessary once ways are not written to lines table any more if they are considered as areas only.
    if (m_config.m_areas) {
        update_multipolygon_geometry(id, member_types, member_ids);
    }
}

void DiffHandler2::update_multipolygon_geometry(const osmium::object_id_type id,
        std::vector<osmium::item_type>& member_types,
        std::vector<osmium::object_id_type> member_ids) {
    std::vector<const osmium::Way*> ways;
    ways.reserve(member_types.size());
    for (std::size_t i = 0; i < member_types.size(); ++i) {
        const osmium::object_id_type member_id = member_ids.at(i);
        if (member_types.at(i) != osmium::item_type::way || member_id == 0) {
            continue;
        }
        const osmium::Way* way = m_mp_manager->get_member_way(member_id);
        if (way) {
            // The diff file contained the way.
            ways.push_back(way);
        } else {
            // Fetch the elements of the way from the database and reconstruct the OSM object.
            std::vector<MemberNode> nodes = m_node_ways_table->get_way_nodes(member_id);
            {
                osmium::builder::WayBuilder way_builder(m_relation_buffer);
                osmium::Way& reconstructeded_way = static_cast<osmium::Way&>(way_builder.object());
                reconstructeded_way.set_id(member_id);
                way_builder.set_user("");
                {
                    osmium::builder::WayNodeListBuilder wnl_builder{m_relation_buffer, &way_builder};
                    for (auto n : nodes) {
                        n.node_ref.set_location(m_location_index.get_node_location(n.node_ref.ref()));
                        wnl_builder.add_node_ref(n.node_ref);
                    }
                }
                ways.push_back(&reconstructeded_way);
            }
            m_relation_buffer.commit();
        }
    }
    // build relation
    const osmium::Relation* relation;
    {
        osmium::builder::RelationBuilder relation_builder(m_relation_buffer);
        osmium::Relation& reconstructed_relation = static_cast<osmium::Relation&>(relation_builder.object());
        reconstructed_relation.set_id(id);
        relation_builder.set_user("");
        {
            osmium::builder::RelationMemberListBuilder rml_builder{m_relation_buffer, &relation_builder};
            for (std::size_t i = 0; i < member_types.size(); ++i) {
                const osmium::object_id_type member_id = member_ids.at(i);
                if (member_types.at(i) != osmium::item_type::way || member_id == 0) {
                    continue;
                }
                rml_builder.add_member(osmium::item_type::way, member_id, "");
            }
        }
        m_relation_buffer.commit();
        relation = &reconstructed_relation;
    }
    try {
        //TODO make osmium::area::Assembler a template parameter
        osmium::area::AssemblerConfig assembler_config;
        osmium::area::Assembler assembler{assembler_config};
        assembler(*relation, ways, m_updated_areas_buffer.buffer());
    } catch (const osmium::invalid_location&) {
        // TODO delete object from database if building an area failed
        // XXX ignore
    }
}

void DiffHandler2::insert_relation(const osmium::Relation& relation, std::string& copy_buffer) {
    //TODO immediatedly return if multipolygon relations should not be written into the relations table (if areas are enabled)
    try {
        std::vector<geos::geom::Geometry*>* points = new std::vector<geos::geom::Geometry*>();
        std::vector<geos::geom::Geometry*>* linestrings = new std::vector<geos::geom::Geometry*>();
        // check if this relation should trigger a tile expiration
        bool trigger_tile_expiry = m_config.expire_this_relation(relation.tags());
        for (const auto& member : relation.members()) {
            if ((member.type() == osmium::item_type::node)) {
                osmium::Location loc = get_point_from_tables(member.ref());
                if (loc.valid()) {
                    if (trigger_tile_expiry) {
                        m_expire_tiles->expire_from_point(loc);
                    }
                    std::unique_ptr<geos::geom::Point> point (m_geom_factory->createPoint(geos::geom::Coordinate(loc.lon(), loc.lat())));
                    points->push_back(point.release());
                }
            }
            else if ((member.type() == osmium::item_type::way)) {
                std::vector<MemberNode> nodes = m_node_ways_table->get_way_nodes(member.ref());
                std::unique_ptr<std::vector<geos::geom::Coordinate>> coordinates {new std::vector<geos::geom::Coordinate>()};
                coordinates->reserve(nodes.size());
                std::vector<MemberNode>::iterator itn = nodes.begin();
                std::vector<geos::geom::Coordinate>::iterator itc = coordinates->begin();
                for (; itn != nodes.end() && itc != coordinates->end(); ++itn, ++itc) {
                    osmium::Location loc = get_point_from_tables(itn->node_ref.ref());
                    itc->x = loc.lon();
                    itc->y = loc.lat();
                }
                geos::geom::CoordinateArraySequenceFactory coord_sequence_factory;
                std::unique_ptr<geos::geom::CoordinateSequence> coord_sequence {coord_sequence_factory.create(coordinates.release())};
                if (trigger_tile_expiry) {
                    m_expire_tiles->expire_from_coord_sequence(coord_sequence.get());
                }
                std::unique_ptr<geos::geom::LineString> linestring {m_geom_factory->createLineString(coord_sequence.release())};
                if (linestring) {
                    linestrings->push_back(linestring.release());
                }
            }
            // We do not add the geometry of this relation to the GeometryCollection.
            /// \todo support one level of nested relations
        }
        // create GeometryCollection
        geos::geom::MultiPoint* multipoints = m_geom_factory->createMultiPoint(points);
        geos::geom::MultiLineString* multilinestrings = m_geom_factory->createMultiLineString(linestrings);
        // add multipoints to query string
        // convert to WKB
        std::stringstream multipoint_stream;
        geos::io::WKBWriter wkb_writer;
        wkb_writer.writeHEX(*multipoints, multipoint_stream);
        delete multipoints;
        // add multilinestrings to query string
        // convert to WKB
        std::stringstream multilinestring_stream;
        wkb_writer.writeHEX(*multilinestrings, multilinestring_stream);
        delete multilinestrings;
        PostgresHandler::prepare_relation_query(relation, copy_buffer, multipoint_stream, multilinestring_stream, m_config,
                m_relations_table);
        m_relations_table.send_line(copy_buffer);
    }
    catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    }
    // There is no need to let this method build an area and insert it into the areas table
    // because the MultipolygonManger assembles the areas for us.
}

void DiffHandler2::way(const osmium::Way& way) {
    if (m_progress == TypeProgress::POINT) {
        write_new_nodes();
    }
    if (way.deleted()) {
        return;
    }
    //TODO immediatedly return if multipolygon relations should not be written into the relations table (if areas are enabled)
    bool with_tags = m_ways_linear_table.has_interesting_tags(way.tags());
    if (with_tags) {
        m_ways_linear_table.send_line(prepare_query(way, m_ways_linear_table, m_config, nullptr));
    }
    // check if relations have to be updated
    std::vector<osmium::object_id_type> rel_ids = m_way_relations_table->get_relation_ids(way.id());
    for (auto id : rel_ids) {
        m_pending_relations.push_back(id);
    }
    // update list of member nodes
    m_node_ways_table->send_line(prepare_node_way_query(way));
    // expire tiles
    m_expire_tiles->expire_from_coord_sequence(way.nodes());
    // remove from list of pending ways
    std::vector<osmium::object_id_type>::size_type found = m_pending_ways_idx;
    for (; found != m_pending_ways.size(); ++found) {
        if (m_pending_ways.at(found) == way.id()) {
            // set to zero to indicate that the way has been processed.
            // The zero will be removed by the next call of sort() and unique().
            m_pending_ways.at(found) = 0;
            m_pending_ways_idx = found;
            break;
        }
        if (m_pending_ways.at(found) > way.id()) {
            break;
        }
    }
}

void DiffHandler2::update_way(const osmium::object_id_type id) {
    //TODO trigger tile expiry
    // Check if we have to update an area
    bool area_to_update = m_config.m_areas && (m_areas_table->count_osm_id(id) > 0);
    // get node list of that way
    std::vector<MemberNode> member_nodes = m_node_ways_table->get_way_nodes(id);
    //TODO PostgresTable::get_way_nodes should return std::vector<osmium::NodeRef> directly.
    std::vector<osmium::NodeRef> node_refs;
    // add locations
    std::string wkb = "010200000000000000";
    try {
        m_ways_linear_table.wkb_factory().linestring_start();
        for (auto& n : member_nodes) {
            n.node_ref.set_location(m_location_index.get_node_location(n.node_ref.ref()));
        }
        for (auto& n : member_nodes) {
            node_refs.push_back(n.node_ref);
        }
        size_t points = m_ways_linear_table.wkb_factory().fill_linestring(node_refs.begin(), node_refs.end());
        wkb = m_ways_linear_table.wkb_factory().linestring_finish(points);

        // This point is only reached if a valid geometry could be build. If so,
        // trigger a geometry update of all relations using this way.
        // Check if relations have to be updated:
        std::vector<osmium::object_id_type> rel_ids = m_way_relations_table->get_relation_ids(id);
        for (auto id : rel_ids) {
            m_pending_relations.push_back(id);
        }
    } catch (osmium::geometry_error& e) {
        std::cerr << e.what() << "\n";
    } catch (osmium::not_found& e) {
        std::cerr << e.what() << "\n";
    }
    m_ways_linear_table.update_geometry(id, wkb.c_str());
    //TODO code before this "if" becomes unnecessary once ways are not written to lines table any more if they are considered as areas only.
    if (!area_to_update) {
        return;
    }
    wkb = "010300000000000000";
    try {
        m_areas_table->wkb_factory().polygon_start();
        size_t points = m_ways_linear_table.wkb_factory().fill_polygon_unique(node_refs.begin(), node_refs.end());
        wkb = m_ways_linear_table.wkb_factory().polygon_finish(points);
    } catch (osmium::geometry_error& e) {
        //TODO delete entry if something failed
        std::cerr << e.what() << "\n";
    } catch (osmium::not_found& e) {
        std::cerr << e.what() << "\n";
    }
    m_areas_table->update_geometry(id, wkb.c_str());
}

void DiffHandler2::update_area_geometry(const osmium::Area& area) {
    std::string wkb = "010300000000000000";
    try {
        wkb = m_areas_table->wkb_factory().create_multipolygon(area);
    } catch (osmium::geometry_error& e) {
        //TODO delete entry if something failed
        std::cerr << e.what() << "\n";
    } catch (osmium::not_found& e) {
        std::cerr << e.what() << "\n";
    }
    m_areas_table->update_geometry(area.orig_id(), wkb.c_str());
}

void DiffHandler2::relation(const osmium::Relation& relation) {
    if (m_progress == TypeProgress::POINT) {
        // write_new_nodes is called if the way callback has not been called yet.
        write_new_nodes();
    }
    if (m_progress != TypeProgress::RELATION) {
        write_new_ways();
    }
    if (relation.deleted()) {
        return;
    }
    std::string copy_buffer;
    insert_relation(relation, copy_buffer);
    // remove from list of pending relations
    std::vector<osmium::object_id_type>::size_type found = m_pending_relations_idx;
    for (; found != m_pending_relations.size(); ++found) {
        if (m_pending_relations.at(found) == relation.id()) {
            // set to zero to indicate that the relation has been processed.
            // The zero will be removed by the next call of sort() and unique().
            m_pending_relations.at(found) = 0;
            m_pending_relations_idx = found;
            break;
        }
        if (m_pending_relations.at(found) > relation.id()) {
            break;
        }
    }
}

void DiffHandler2::area(const osmium::Area& area) {
    if (m_progress == TypeProgress::POINT) {
        write_new_nodes();
    }
    handle_area(area);
    // expire tiles
    // TODO expire whole polygon
    m_expire_tiles->expire_from_coord_sequence(*(area.outer_rings().begin()));
    // TODO remove from list of pending ways
}

void DiffHandler2::incomplete_relation(const osmium::relations::RelationHandle& rel_handle) {
    incomplete_relation(*rel_handle);
}

void DiffHandler2::incomplete_relation(const osmium::Relation& relation) {
    assert(m_mp_manager);
    std::vector<const osmium::Way*> ways;
    ways.reserve(relation.members().size());
    for (const auto& member : relation.members()) {
        if (member.ref() == 0) {
            continue;
        }
        const osmium::Way* way = m_mp_manager->get_member_way(member.ref());
        if (way) {
            ways.push_back(way);
        } else {
            //fetch elements from database
            std::vector<MemberNode> nodes = m_node_ways_table->get_way_nodes(member.ref());
            {
                osmium::builder::WayBuilder way_builder(m_relation_buffer);
                osmium::Way& reconstructeded_way = static_cast<osmium::Way&>(way_builder.object());
                reconstructeded_way.set_id(member.ref());
                way_builder.set_user("");
                {
                    osmium::builder::WayNodeListBuilder wnl_builder{m_relation_buffer, &way_builder};
                    for (auto n : nodes) {
                        n.node_ref.set_location(m_location_index.get_node_location(n.node_ref.ref()));
                        wnl_builder.add_node_ref(n.node_ref);
                    }
                }
                ways.push_back(&reconstructeded_way);
            }
            m_relation_buffer.commit();
        }
    }
    try {
        //TODO make osmium::area::Assembler a template parameter
        osmium::area::AssemblerConfig assembler_config;
        osmium::area::Assembler assembler{assembler_config};
        assembler(relation, ways, m_new_areas_buffer.buffer());
    } catch (const osmium::invalid_location&) {
        // TODO delete polygon if the new version were invalid
        // XXX ignore
    }
}

void DiffHandler2::flush() {
    m_new_areas_buffer.flush();
    m_updated_areas_buffer.flush();
}

void DiffHandler2::write_new_nodes() {
    std::cerr << "write_new_nodes\n";
    m_nodes_table.end_copy();
    m_untagged_nodes_table->end_copy();
    m_ways_linear_table.start_copy();
    if (m_areas_table) {
        m_areas_table->start_copy();
    }
    m_node_ways_table->start_copy();
    m_progress = TypeProgress::WAY;
    // sort list of pending ways
    std::cerr << "sorting list of pending ways ...";
    std::sort(m_pending_ways.begin(), m_pending_ways.end());
    //TODO check if way() can work with a non-unique container
    m_pending_ways.erase(std::unique(m_pending_ways.begin(), m_pending_ways.end()), m_pending_ways.end());
    std::cerr << " done\n";
}

void DiffHandler2::write_new_ways() {
    if (m_ways_linear_table.get_copy()) {
        m_ways_linear_table.end_copy();
    }
    if (m_node_ways_table->get_copy()) {
        m_node_ways_table->end_copy();
    }
    if (m_areas_table && m_areas_table->get_copy()) {
        // We do some UPDATEs to the areas table. That's why we have to stop copying now and restart it later.
        m_areas_table->end_copy();
    }
    work_on_pending_ways();
    // sort list of pending relations
    std::cerr << "sorting list of pending relations ...";
    std::sort(m_pending_relations.begin(), m_pending_relations.end());
    m_pending_relations.erase(std::unique(m_pending_relations.begin(), m_pending_relations.end()), m_pending_relations.end());
    std::cerr << " done\n";

    if (m_areas_table) {
        m_areas_table->start_copy();
    }
    m_relations_table.start_copy();
    m_progress = TypeProgress::RELATION;
}

void DiffHandler2::after_relations() {
    m_relations_table.end_copy();
    if (m_areas_table) {
        m_areas_table->end_copy();
    }
    std::cerr << "sorting list of relations ways ...";
//    using namespace std::placeholders;
//    std::function<void(osmium::object_id_type)> func = std::bind(&DiffHandler2::update_relation, this, _1);
    clean_up_container_and_work_on(m_pending_relations, [this](const osmium::object_id_type id){this->update_relation(id);});
}

void DiffHandler2::work_on_pending_ways() {
    std::cerr << "sorting list of pending ways ...";
//    using namespace std::placeholders;
//    std::function<void(osmium::object_id_type)> func = std::bind(&DiffHandler2::update_way, this, _1);
    clean_up_container_and_work_on(m_pending_ways, [this](const osmium::object_id_type id){this->update_way(id);});
}

// Can be converted in a template if we work on containers
void DiffHandler2::clean_up_container_and_work_on(std::vector<osmium::object_id_type>& container,
        std::function<void(const osmium::object_id_type)> func) {
    std::sort(container.begin(), container.end());
    std::cerr << " working on it ...";    // find first element after 0
    //TODO use std::unique
    std::vector<osmium::object_id_type>::iterator it = std::find_if(container.begin(),
            container.end(),
            [](const osmium::object_id_type x){return x > 0;});
    // check if vector of pending ways is empty or contains only zeros
    if (it == container.end()) {
        std::cerr << " nothing to do\n";
        return;
    }
    ++it;
    osmium::object_id_type last = 0;
    for (;it != container.end(); ++it) {
        if (last != *it) {
            func(*it);
            last = *it;
        }
    }
    std::cerr << " done\n";
}
