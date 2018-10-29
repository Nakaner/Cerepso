# Look for the header files.
find_path(PGARRAYHSTOREPARSER_INCLUDE_DIR postgres_parser.hpp
    PATH_SUFFIXES include
    PATHS
        ../pg-array-hstore-parser
)

set(PGARRAYHSTOREPARSER_INCLUDE_DIRS "${PGARRAYHSTOREPARSER_INCLUDE_DIR}")
include(FindPackageHandleStandardArgs)

# if all listed variables are TRUE
find_package_handle_standard_args(catch  DEFAULT_MSG
                                  PGARRAYHSTOREPARSER_LIBRARY PGARRAYHSTOREPARSER_INCLUDE_DIR)
