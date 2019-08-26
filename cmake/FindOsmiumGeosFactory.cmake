
#----------------------------------------------------------------------
#
#  FindOsmiumGeosFactory.cmake
#
#  Find the headers of osmium-geos-factory.
#
#----------------------------------------------------------------------
#
#  Usage:
#
#    Copy this file somewhere into your project directory, where cmake can
#    find it. Usually this will be a directory called "cmake" which you can
#    add to the CMake module search path with the following line in your
#    CMakeLists.txt:
#
#      list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
#
#    Then add the following in your CMakeLists.txt:
#
#      find_package(OsmiumGeosFactory REQUIRED )
#      include_directories(SYSTEM ${OSMIUMGEOSFACTORY_INCLUDE_DIRS})
#
#    You can check for success with something like this:
#
#      if(NOT OSMIUMGEOSFACTORY_FOUND)
#          message(WARNING "osmium-geos-factory not found!\n")
#      endif()
#
#----------------------------------------------------------------------
#
#  Variables:
#
#    OSMIUMGEOSFACTORY_FOUND         - True if Osmium found.
#    OSMIUMGEOSFACTORY_INCLUDE_DIRS  - Where to find include files.
#
#----------------------------------------------------------------------

# Look for the header file.
find_path(OSMIUMGEOSFACTORY_INCLUDE_DIR osmium_geos_factory/geos_factory.hpp 
    PATH_SUFFIXES include
    PATHS
        ../osmium-geos-factory
        ~/Library/Frameworks
        /Library/Frameworks
        /opt/local # DarwinPorts
        /opt
)

set(OSMIUMGEOSFACTORY_INCLUDE_DIRS "${OSMIUMGEOSFACTORY_INCLUDE_DIR}")

#----------------------------------------------------------------------
# geos
find_path(GEOS_INCLUDE_DIR geos_c.h)
find_library(GEOS_LIBRARY NAMES geos)

list(APPEND OSMIUMGEOSFACTORY_EXTRA_FIND_VARS GEOS_INCLUDE_DIR GEOS_LIBRARY)
if(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)
    SET(GEOS_FOUND 1)
    list(APPEND OSMIUMGEOSFACTORY_LIBRARIES ${GEOS_LIBRARY})
    list(APPEND OSMIUMGEOSFACTORY_INCLUDE_DIRS ${GEOS_INCLUDE_DIR})
else()
    message(WARNING "Osmium: GEOS library is required but not found, please install it or configure the paths.")
endif()


#----------------------------------------------------------------------
# proj
# The proj library is already provided by Osmium


#----------------------------------------------------------------------

list(REMOVE_DUPLICATES OSMIUMGEOSFACTORY_INCLUDE_DIRS)

if(OSMIUMGEOSFACTORY_LIBRARIES)
    list(REMOVE_DUPLICATES OSMIUMGEOSFACTORY_LIBRARIES)
endif()

#----------------------------------------------------------------------
#
#  Check that all required libraries are available
#
#----------------------------------------------------------------------
if (OSMIUMGEOSFACTORY_EXTRA_FIND_VARS)
    list(REMOVE_DUPLICATES OSMIUMGEOSFACTORY_EXTRA_FIND_VARS)
endif()
# Handle the QUIETLY and REQUIRED arguments and set OSMIUMGEOSFACTORY_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OsmiumGeosFactory REQUIRED_VARS OSMIUMGEOSFACTORY_INCLUDE_DIR ${OSMIUMGEOSFACTORY_EXTRA_FIND_VARS})
unset(OSMIUMGEOSFACTORY_EXTRA_FIND_VARS)

#----------------------------------------------------------------------
#
#  Add compiler flags
#
#----------------------------------------------------------------------
add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)

if(MSVC)
    add_definitions(-wd4996)

    # Disable warning C4068: "unknown pragma" because we want it to ignore
    # pragmas for other compilers.
    add_definitions(-wd4068)

    # Disable warning C4715: "not all control paths return a value" because
    # it generates too many false positives.
    add_definitions(-wd4715)

    # Disable warning C4351: new behavior: elements of array '...' will be
    # default initialized. The new behaviour is correct and we don't support
    # old compilers anyway.
    add_definitions(-wd4351)

    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

if(APPLE)
# following only available from cmake 2.8.12:
#   add_compile_options(-stdlib=libc++)
# so using this instead:
    add_definitions(-stdlib=libc++)
    set(LDFLAGS ${LDFLAGS} -stdlib=libc++)
endif()


if(OsmiumGeosFactory_DEBUG)
    message(STATUS "OSMIUMGEOSFACTORY_LIBRARIES=" ${OSMIUMGEOSFACTORY_LIBRARIES})
    message(STATUS "OSMIUMGEOSFACTORY_INCLUDE_DIRS=" ${OSMIUMGEOSFACTORY_INCLUDE_DIRS})
endif()

