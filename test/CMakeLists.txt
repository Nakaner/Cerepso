enable_testing()

message(STATUS "Configuring unit tests")

include(CMakeParseArguments)
include_directories(include)
include_directories(../src)

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")

# Otherwise GCC throws a lot of warnings for REQUIRE(...) from Catch v.1.2.1
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-parentheses)
endif()


add_executable(test_hstore_escape t/test_hstore_escape.cpp ../src/postgres_table.cpp)
target_link_libraries(test_hstore_escape testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_hstore_escape
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_hstore_escape)

add_executable(test_node_handler t/test_node_handler.cpp ../src/import_handler.cpp ../src/postgres_table.cpp ../src/postgres_handler.cpp)
target_link_libraries(test_node_handler testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_node_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_node_handler)

add_executable(test_diff_handler t/test_diff_handler.cpp ../src/diff_handler2.cpp ../src/postgres_table.cpp ../src/postgres_handler.cpp ../src/expire_tiles_factory.cpp ../src/expire_tiles_classic.cpp ../src/expire_tiles_dummy.hpp ../src/expire_tiles_quadtree.cpp  ../src/expire_tiles.cpp)
target_link_libraries(test_diff_handler testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_diff_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_diff_handler)

add_executable(test_prepare_relation_query t/test_prepare_relation_query.cpp ../src/postgres_table.cpp ../src/postgres_handler.cpp ../src/expire_tiles_factory.cpp ../src/expire_tiles_classic.cpp ../src/expire_tiles_dummy.hpp ../src/expire_tiles_quadtree.cpp  ../src/expire_tiles.cpp ../src/diff_handler2.cpp)
target_link_libraries(test_prepare_relation_query testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_prepare_relation_query
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_prepare_relation_query)

add_executable(test_qt_to_xy t/test_qt_to_xy.cpp ../src/expire_tiles_quadtree.cpp ../src/expire_tiles.cpp)
target_link_libraries(test_qt_to_xy testlib ${GEOS_LIBRARY})
add_test(NAME test_qt_to_xy
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_qt_to_xy)

add_executable(test_xy_to_qt t/test_xy_to_qt.cpp ../src/expire_tiles_quadtree.cpp ../src/expire_tiles.cpp)
target_link_libraries(test_xy_to_qt testlib ${GEOS_LIBRARY})
add_test(NAME test_xy_to_qt
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_xy_to_qt)

add_executable(test_expire_tiles_quadtree t/test_expire_tiles_quadtree.cpp ../src/expire_tiles_quadtree.cpp ../src/expire_tiles.cpp)
target_link_libraries(test_expire_tiles_quadtree testlib ${GEOS_LIBRARY})
add_test(NAME test_expire_tiles_quadtree
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_expire_tiles_quadtree)
