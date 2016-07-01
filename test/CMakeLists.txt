enable_testing()

#find_package(Catch REQUIRED)
#include_directories(${CATCH_INCLUDE_DIRS})
#set(LIBS ${LIBS} ${CATCH_LIBRARIES} pgimporter)
#include_directories(. ../src)
#add_executable(test_hstore_escape test_hstore_escape.cpp)
#target_link_libraries(test_hstore_escape ${LIBS})
#add_test(test_hstore_escape ${CMAKE_CURRENT_BINARY_DIR}/test_hstore_escape)




#add_executable(test_hstore_escape test_hstore_escape.cpp)
#add_test(test_hstore_escape test_hstore_escape)

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
target_link_libraries(test_hstore_escape testlib "-lboost_program_options -lpq")
add_test(NAME test_hstore_escape
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_hstore_escape)


