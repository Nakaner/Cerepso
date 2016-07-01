# Look for the header file.
find_path(CATCH_INCLUDE_DIR catch.hpp
    PATH_SUFFIXES include
    PATHS
        test
)

set(CATCH_INCLUDE_DIRS "${CATCH_INCLUDE_DIR}")
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(catch  DEFAULT_MSG
                                  CATCH_LIBRARY CATCH_INCLUDE_DIR)
