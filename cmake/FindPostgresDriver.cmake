# Look for the header files.
find_path(POSTGRESDRIVERS_INCLUDE_DIR postgres_drivers/table.hpp
    PATH_SUFFIXES include
    PATHS
        ../postgres-drivers
)

set(POSTGRESDRIVERS_INCLUDE_DIRS "${POSTGRESDRIVERS_INCLUDE_DIR}")
include(FindPackageHandleStandardArgs)

# if all listed variables are TRUE
find_package_handle_standard_args(catch  DEFAULT_MSG
                                  POSTGRESDRIVERS_LIBRARY POSTGRESDRIVERS_INCLUDE_DIR)
