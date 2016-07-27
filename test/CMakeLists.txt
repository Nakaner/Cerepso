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


add_executable(test_hstore_escape t/test_hstore_escape.cpp ../src/table.cpp ../src/columns.cpp)
target_link_libraries(test_hstore_escape testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_hstore_escape
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_hstore_escape)

add_executable(test_node_handler t/test_node_handler.cpp ../src/myhandler.cpp ../src/table.cpp ../src/columns.cpp ../src/postgres_handler.cpp)
target_link_libraries(test_node_handler testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_node_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_node_handler)

add_executable(test_diff_handler t/test_diff_handler.cpp ../src/diff_handler1.cpp ../src/table.cpp ../src/columns.cpp ../src/postgres_handler.cpp)
target_link_libraries(test_diff_handler testlib ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY} ${GEOS_LIBRARY})
add_test(NAME test_diff_handler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_diff_handler)
