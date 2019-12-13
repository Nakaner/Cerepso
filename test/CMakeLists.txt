enable_testing()

message(STATUS "Configuring unit tests")

include(CMakeParseArguments)
include_directories(include)
include_directories(../src)

#set(_test_path "--test-data-dir ${CMAKE_HOME_DIRECTORY}/test/data/")

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")

# Otherwise GCC throws a lot of warnings for REQUIRE(...) from Catch v.1.2.1
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-parentheses)
endif()


add_executable(test_hstore_escape
    t/test_hstore_escape.cpp
    ../src/tables/postgres_table.cpp
    ../src/associated_street_relation_manager.cpp
)
target_link_libraries(test_hstore_escape testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_hstore_escape
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_hstore_escape)

add_executable(test_node_handler
    t/test_node_handler.cpp
    ../src/import_handler.cpp
    ../src/tables/features_table.cpp
    ../src/tables/postgres_table.cpp
    ../src/tables/node_locations_table.cpp
    ../src/postgres_handler.cpp
    ../src/associated_street_relation_manager.cpp
)
target_link_libraries(test_node_handler testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_node_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_node_handler)

add_executable(test_diff_handler
    t/test_diff_handler.cpp
    ../src/diff_handler2.cpp
    ../src/tables/postgres_table.cpp
    ../src/tables/features_table.cpp
    ../src/tables/node_locations_table.cpp
    ../src/tables/way_nodes_table.cpp
    ../src/tables/relations_table.cpp
    ../src/tables/relation_members_table.cpp
    ../src/postgres_handler.cpp
    ../src/associated_street_relation_manager.cpp
    ../src/expire_tiles_factory.cpp
    ../src/expire_tiles_quadtree.cpp
    ../src/expire_tiles.cpp
    ../src/database_location_handler.cpp
)
target_link_libraries(test_diff_handler testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_diff_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_diff_handler)

add_executable(test_prepare_relation_query
    t/test_prepare_relation_query.cpp
    ../src/tables/postgres_table.cpp
    ../src/tables/features_table.cpp
    ../src/tables/node_locations_table.cpp
    ../src/tables/way_nodes_table.cpp
    ../src/tables/relations_table.cpp
    ../src/tables/relation_members_table.cpp
    ../src/postgres_handler.cpp
    ../src/associated_street_relation_manager.cpp
    ../src/expire_tiles_factory.cpp
    ../src/expire_tiles_quadtree.cpp
    ../src/expire_tiles.cpp
    ../src/diff_handler2.cpp
    ../src/database_location_handler.cpp
)
target_link_libraries(test_prepare_relation_query testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_prepare_relation_query
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_prepare_relation_query)

add_executable(test_qt_to_xy
    t/test_qt_to_xy.cpp
    ../src/expire_tiles_quadtree.cpp
    ../src/expire_tiles.cpp
)
target_link_libraries(test_qt_to_xy testlib ${GEOS_LIBRARY})
add_test(NAME test_qt_to_xy
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_qt_to_xy)

add_executable(test_xy_to_qt
    t/test_xy_to_qt.cpp
    ../src/expire_tiles_quadtree.cpp
    ../src/expire_tiles.cpp
)
target_link_libraries(test_xy_to_qt testlib ${GEOS_LIBRARY})
add_test(NAME test_xy_to_qt
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_xy_to_qt)

add_executable(test_expire_tiles_quadtree
    t/test_expire_tiles_quadtree.cpp
    ../src/expire_tiles_quadtree.cpp
    ../src/expire_tiles.cpp
)
target_link_libraries(test_expire_tiles_quadtree testlib ${GEOS_LIBRARY})
add_test(NAME test_expire_tiles_quadtree
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_expire_tiles_quadtree)

add_executable(test_addr_interpolation_handler
    t/test_addr_interpolation_handler.cpp
    ../src/addr_interpolation_handler.cpp
    ../src/tables/postgres_table.cpp
    ../src/tables/features_table.cpp
    ../src/tags_storage.cpp
)
target_compile_options(test_addr_interpolation_handler PUBLIC -DTEST_DATA_DIR=${CMAKE_HOME_DIRECTORY}/test/data/)
target_link_libraries(test_addr_interpolation_handler testlib ${OSMIUM_LIBRARIES} ${BOOST_LIBRARIES} ${PostgreSQL_LIBRARY}  ${GEOS_LIBRARY})
add_test(NAME test_addr_interpolation_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_addr_interpolation_handler)
