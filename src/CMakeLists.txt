#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(pgimporter pgimporter.cpp postgres_handler.cpp postgres_table.cpp relation_collector.cpp import_handler.cpp diff_handler1.cpp expire_tiles_classic.cpp expire_tiles.cpp expire_tiles_factory.cpp expire_tiles_quadtree.cpp diff_handler2.cpp associated_street_relation_manager.cpp column_config_parser.cpp)
target_link_libraries(pgimporter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
install(TARGETS pgimporter DESTINATION bin)

