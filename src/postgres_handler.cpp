/*
 * postgres_handler.cpp
 *
 *  Created on: 12.07.2016
 *      Author: michael
 */


#include <sstream>
#include "postgres_handler.hpp"

void PostgresHandler::add_separator_to_stringstream(std::string& ss) {
    ss.push_back('\t');
}

void PostgresHandler::add_username(std::string& ss, const char* username) {
    PostgresTable::escape(username, ss);
}

void PostgresHandler::add_uid(std::string& ss, const osmium::user_id_type uid) {
    static char idbuffer[20];
    sprintf(idbuffer, "%u", uid);
    ss.append(idbuffer);
}

void PostgresHandler::add_version(std::string& ss, const osmium::object_version_type version) {
    static char idbuffer[20];
    sprintf(idbuffer, "%u", version);
    ss.append(idbuffer);
}

void PostgresHandler::add_timestamp(std::string& ss, const osmium::Timestamp& timestamp) {
    ss.append(timestamp.to_iso());
}

void PostgresHandler::add_changeset(std::string& ss, const osmium::changeset_id_type changeset) {
    static char idbuffer[20];
    sprintf(idbuffer, "%u", changeset);
    ss.append(idbuffer);
}

const osmium::TagList* PostgresHandler::get_relation_tags_to_apply(const osmium::object_id_type id,
        const osmium::item_type type) {
    if (!m_assoc_manager) {
        return nullptr;
    }
    // get street tags (associatedStreet relation)
    if (m_config.m_associated_streets) {
        return m_assoc_manager->get_relation_tags(id, type);
    }
    return nullptr;
}

/*static*/ void PostgresHandler::add_way_nodes(const osmium::WayNodeList& nodes, std::string& query) {
    query.append("{");
    for (osmium::WayNodeList::const_iterator i = nodes.begin(); i < nodes.end(); ++i) {
        if (i != nodes.begin()) {
            query.append(", ");
        }
        static char idbuffer[20];
        sprintf(idbuffer, "%ld", i->ref());
        query.append(idbuffer);
    }
    query.push_back('}');
}

/*static*/ void PostgresHandler::add_member_ids(const osmium::RelationMemberList& members, std::string& query) {
    query.push_back('{');
    for (auto it = members.begin(); it != members.end(); ++it) {
        if (it != members.begin()) {
            query.append(", ");
        }
        static char idbuffer[20];
        sprintf(idbuffer, "%ld", it->ref());
        query.append(idbuffer);
    }
    query.push_back('}');
}

/*static*/ void PostgresHandler::add_member_roles(const osmium::RelationMemberList& members, std::string& query) {
    query.push_back('{');
    for (auto it = members.begin(); it != members.end(); ++it) {
        if (it != members.begin()) {
            query.append(", ");
        }
        query.push_back('"');
        PostgresTable::escape4array(it->role(), query);
        query.push_back('"');
    }
    query.push_back('}');
}

/*static*/ void PostgresHandler::add_member_types(const osmium::RelationMemberList& members, std::string& query) {
    query.push_back('{');
    for (auto it = members.begin(); it != members.end(); ++it) {
        if (it != members.begin()) {
            query.append(", ");
        }
        query.push_back(item_type_to_char(it->type()));
    }
    query.push_back('}');
}

/*static*/ bool PostgresHandler::fill_field(const osmium::OSMObject& object, postgres_drivers::ColumnsConstIterator it,
        std::string& query, bool column_added, std::vector<const char*>& written_keys, PostgresTable& table,
        const osmium::TagList* rel_tags_to_apply) {
    if (column_added) {
        add_separator_to_stringstream(query);
    }
    column_added = true;
    if (it->column_class() == postgres_drivers::ColumnClass::TAGS_OTHER) {
        bool first_tag = true;
        // add normal tags
        for (const auto& tag : object.tags()) {
            bool has_tag = false;
            for (const char* k : written_keys) {
                has_tag |= (strcmp(k, tag.key()) == 0);
            }
            if (!has_tag && !table.get_columns().drop_filter()(tag)) {
                if (!first_tag) {
                    query.push_back(',');
                }
                PostgresTable::escape4hstore(tag.key(), query);
                query.append("=>");
                PostgresTable::escape4hstore(tag.value(), query);
                first_tag = false;
            }
        }
        if (rel_tags_to_apply) {
            // add relation tags
            for (const auto& tag : *rel_tags_to_apply) {
                bool has_tag = false;
                for (const char* k : written_keys) {
                    has_tag |= (strcmp(k, tag.key()) == 0);
                }
                if (!has_tag && !table.get_columns().drop_filter()(tag)) {
                    if (!first_tag) {
                        query.push_back(',');
                    }
                    PostgresTable::escape4hstore(tag.key(), query);
                    query.append("=>");
                    PostgresTable::escape4hstore(tag.value(), query);
                    first_tag = false;
                }
            }
        }
    } else if (it->column_class() == postgres_drivers::ColumnClass::TAG) {
        const char* t = object.get_value_by_key(it->name().c_str());
        if (!t && rel_tags_to_apply && rel_tags_to_apply->has_key(it->name().c_str())) {
            PostgresTable::escape(rel_tags_to_apply->get_value_by_key(it->name().c_str()), query);
            written_keys.push_back(it->name().c_str());
        } else if (t) {
            PostgresTable::escape(t, query);
            written_keys.push_back(it->name().c_str());
        } else {
            query.append("\\N");
        }
    } else if (it->column_class() == postgres_drivers::ColumnClass::OSM_ID) {
        static char idbuffer[20];
        if (object.type() == osmium::item_type::node || object.type() == osmium::item_type::way
                || object.type() == osmium::item_type::relation) {
            sprintf(idbuffer, "%ld", object.id());
        } else if (object.type() == osmium::item_type::area
                && static_cast<const osmium::Area&>(object).from_way()) {
            sprintf(idbuffer, "%ld", static_cast<const osmium::Area&>(object).orig_id());
        } else if (object.type() == osmium::item_type::area) {
            sprintf(idbuffer, "%ld", -static_cast<const osmium::Area&>(object).orig_id());
        }
        query.append(idbuffer, strlen(idbuffer));
    } else if (it->column_class() == postgres_drivers::ColumnClass::VERSION) {
        add_version(query, object.version());
    } else if (it->column_class() == postgres_drivers::ColumnClass::UID) {
        add_uid(query, object.version());
    } else if (it->column_class() == postgres_drivers::ColumnClass::USERNAME) {
        add_username(query, object.user());
    } else if (it->column_class() == postgres_drivers::ColumnClass::CHANGESET) {
        add_changeset(query, object.version());
    } else if (it->column_class() == postgres_drivers::ColumnClass::TIMESTAMP) {
        add_timestamp(query, object.timestamp());
    } else if (it->column_class() == postgres_drivers::ColumnClass::WAY_NODES
            && object.type() == osmium::item_type::way
            && table.config().m_driver_config.updateable) {
        add_way_nodes(static_cast<const osmium::Way&>(object).nodes(), query);
    } else if (it->column_class() == postgres_drivers::ColumnClass::MEMBER_IDS
            && object.type() == osmium::item_type::relation) {
        add_member_ids(static_cast<const osmium::Relation&>(object).members(), query);
    } else if (it->column_class() == postgres_drivers::ColumnClass::MEMBER_ROLES
            && object.type() == osmium::item_type::relation) {
        add_member_roles(static_cast<const osmium::Relation&>(object).members(), query);
    } else if (it->column_class() == postgres_drivers::ColumnClass::MEMBER_TYPES
            && object.type() == osmium::item_type::relation) {
        add_member_types(static_cast<const osmium::Relation&>(object).members(), query);
    } else if (it->column_class() == postgres_drivers::ColumnClass::GEOMETRY) {
        std::string wkb; // initalize with MULTIPOLYGON EMPTY
        // If creating a geometry fails, we have to use an empty multipolygon.
        try {
            switch (object.type()) {
            case osmium::item_type::node :
                wkb = "010400000000000000";
                wkb = table.wkb_factory().create_point(static_cast<const osmium::Node&>(object));
                break;
            case osmium::item_type::way :
                wkb = "010200000000000000";
                wkb = table.wkb_factory().create_linestring(static_cast<const osmium::Way&>(object).nodes());
                break;
            case osmium::item_type::area :
                wkb = "0106000020E610000000000000";
                wkb = table.wkb_factory().create_multipolygon(static_cast<const osmium::Area&>(object));
                break;
            default :
                assert(false && "This OSM object type is not supported for a geometry field.");
            }
        } catch (osmium::geometry_error& e) {
            std::cerr << e.what() << "\n";
        }
        query.append("SRID=4326;");
        query.append(wkb);
    } else {
        column_added = false;
    }
    return column_added;
}

/*static*/ std::string PostgresHandler::prepare_query(const osmium::OSMObject& object, PostgresTable& table,
        CerepsoConfig& config, const osmium::TagList* rel_tags_to_apply) {
    std::string query;
    std::vector<const char*> written_keys;
    bool column_added = false;
    for (postgres_drivers::ColumnsConstIterator it = table.get_columns().cbegin(); it != table.get_columns().cend(); it++) {
        column_added = fill_field(object, it, query, column_added, written_keys, table, rel_tags_to_apply);
    }
    query.push_back('\n');
    return query;
}

/*static*/ void PostgresHandler::prepare_relation_query(const osmium::Relation& relation, std::string& query,
        std::stringstream& multipoint_wkb, std::stringstream& multilinestring_wkb, CerepsoConfig& config, PostgresTable& table) {
    std::vector<const char*> written_keys;
    bool column_added = false;
    for (postgres_drivers::ColumnsConstIterator it = table.get_columns().cbegin(); it != table.get_columns().cend(); ++it) {
        column_added = fill_field(relation, it, query, column_added, written_keys, table, nullptr);
        if (!column_added) {
            // special geometry columns for relations
            if (it->column_class() == postgres_drivers::ColumnClass::GEOMETRY_MULTIPOINT) {
                query.append("SRID=4326;");
                query.append(multipoint_wkb.str());
                add_separator_to_stringstream(query);
            } else if (it->column_class() == postgres_drivers::ColumnClass::GEOMETRY_MULTILINESTRING) {
                query.append("SRID=4326;");
                query.append(multilinestring_wkb.str());
                add_separator_to_stringstream(query);
            }
        }
    }
    query.append("\n");
}

void PostgresHandler::handle_node(const osmium::Node& node) {
    if (!node.location().valid()) {
        return;
    }
    if (!m_config.m_driver_config.untagged_nodes && node.tags().empty()) {
        return;
    }
    std::string query;
    bool with_tags = m_nodes_table.has_interesting_tags(node.tags());
    if (with_tags) {
        const osmium::TagList* rel_tags_to_apply = get_relation_tags_to_apply(node.id(), osmium::item_type::node);
        std::string query = prepare_query(node, m_nodes_table, m_config, rel_tags_to_apply);
        m_nodes_table.send_line(query);
    } else if (m_config.m_driver_config.untagged_nodes) {
        std::string query = prepare_query(node, *m_untagged_nodes_table, m_config, nullptr);
        m_untagged_nodes_table->send_line(query);
    }
}
