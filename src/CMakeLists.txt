#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(pgimporter pgimporter.cpp myhandler.cpp columns.cpp table.cpp relation_collector.cpp postgres_handler.cpp diff_handler1.cpp expire_tiles_classic.cpp expire_tiles.cpp expire_tiles_factory.cpp expire_tiles_dummy.hpp expire_tiles_quadtree.cpp)
target_link_libraries(pgimporter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
install(TARGETS pgimporter DESTINATION bin)

