# Look for the header files.
find_path(CEREPSOPOSTGRESBACKEND_INCLUDE_DIR postgres_drivers/table.hpp
    PATH_SUFFIXES include
    PATHS
        ../cerepso-postgres-backend
)

set(CEREPSOPOSTGRESBACKEND_INCLUDE_DIRS "${CEREPSOPOSTGRESBACKEND_INCLUDE_DIR}")
include(FindPackageHandleStandardArgs)

# if all listed variables are TRUE
find_package_handle_standard_args(catch  DEFAULT_MSG
                                  CEREPSOPOSTGRESBACKEND_LIBRARY CEREPSOPOSTGRESBACKEND_INCLUDE_DIR)
