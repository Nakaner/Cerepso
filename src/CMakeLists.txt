#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(pgimporter
    pgimporter.cpp
    postgres_handler.cpp
    tables/postgres_table.cpp
    tables/features_table.cpp
    tables/node_locations_table.cpp
    tables/relations_table.cpp
    tables/relation_members_table.cpp
    tables/way_nodes_table.cpp
    relation_collector.cpp
    import_handler.cpp
    diff_handler1.cpp
    expire_tiles.cpp
    expire_tiles_factory.cpp
    expire_tiles_quadtree.cpp
    diff_handler2.cpp
    associated_street_relation_manager.cpp
    column_config_parser.cpp
    addr_interpolation_handler.cpp
    handler_collection.cpp
    tags_storage.cpp
    database_location_handler.cpp
)
target_link_libraries(pgimporter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
install(TARGETS pgimporter DESTINATION bin)

