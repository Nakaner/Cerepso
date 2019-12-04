#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  taken from github.com/osmcode/osm-gis-export
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

project(pgimporter)

set(PGIMPORTER_VERSION_MAJOR 0)
set(PGIMPORTER_VERSION_MINOR 0)
set(PGIMPORTER_VERSION_PATCH 1)

set(PGIMPORTER_VERSION
    ${PGIMPORTER_VERSION_MAJOR}.${PGIMPORTER_VERSION_MINOR}.${PGIMPORTER_VERSION_PATCH})

set(AUTHOR "Michael Reichert <nakaner@gmx.net>")


#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

if (NOT EXTERNAL_CEREPSO_POSTGRES_DRIVER)
  set(POSTGRESDRIVERS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/cerepso-postgres-backend/include")
endif()

if (NOT EXTERNAL_PG_ARRAY_HSTORE_PARSER)
  set(PGARRAYHSTOREPARSER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/pg-array-hstore-parser/include")
endif()

if (NOT EXTERNAL_WKBHPP)
  set(WKBHPP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/wkbhpp/include")
endif()

if (NOT EXTERNAL_PG_ARRAY_HSTORE_PARSER)
  set(OSMIUMGEOSFACTORY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/osmium-geos-factory/include")
endif()

# set include path to patched version of Libosmium RelationManager and MemberDatabaseCommon
set(OSMIUMPATCHED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/contrib/libosmium_patched/include")
include_directories(${OSMIUMPATCHED_INCLUDE_DIR})

find_package(Boost)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(Osmium COMPONENTS io geos proj)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})

# PostgreSQL stuff taken from osm2pgsql cmake config
if (NOT WIN32 AND NOT APPLE)
  # No need for this path, just a workaround to make cmake check pass on all systems
  set(PostgreSQL_TYPE_INCLUDE_DIR /usr/include)
endif()
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

find_package(CerepsoPostgresBackend REQUIRED)
include_directories(${CEREPSOPOSTGRESBACKEND_INCLUDE_DIRS})

find_package(Wkbhpp REQUIRED)
include_directories(${WKBHPP_INCLUDE_DIRS})

find_package(PgArrayHstoreParser REQUIRED)
include_directories(${PGARRAYHSTOREPARSER_INCLUDE_DIRS})

find_package(OsmiumGeosFactory REQUIRED)
include_directories(${OSMIUMGEOSFACTORY_INCLUDE_DIR})


#-----------------------------------------------------------------------------
#
#  Decide which C++ version to use (Minimum/default: C++11).
#
#-----------------------------------------------------------------------------
if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
set(USUAL_COMPILE_OPTIONS "-O3 -g")

set(CMAKE_CXX_FLAGS_DEV "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV ""
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------
set(CMAKE_CONFIGURATION_TYPES "Debug Release RelWithDebInfo MinSizeRel Dev")

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(NOT MSVC)
        add_definitions(-Werror -fno-omit-frame-pointer)
    endif()
    add_definitions(${OSMIUM_WARNING_OPTIONS})
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)

#-----------------------------------------------------------------------------
#
#  Unit and data tests
#
#-----------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)


#-----------------------------------------------------------------------------
#
#  Documentation
#
#-----------------------------------------------------------------------------

add_subdirectory(doc)


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS --enable=all)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB ALL_CODE *.cpp)

    set(CPPCHECK_FILES ${ALL_CODE})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif(CPPCHECK)


#-----------------------------------------------------------------------------

add_definitions(${OSMIUM_WARNING_OPTIONS})

add_subdirectory(src)


#-----------------------------------------------------------------------------
