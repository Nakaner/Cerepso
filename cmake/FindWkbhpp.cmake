# Look for the header files.
find_path(WKBHPP_INCLUDE_DIR wkbhpp/wkbwriter.hpp
    PATH_SUFFIXES include
    PATHS
        ../wkbhpp
        ~/Library/Frameworks
        /Library/Frameworks
        /opt/local # DarwinPorts
        /opt
)

set(WKBHPP_INCLUDE_DIRS "${WKBHPP_INCLUDE_DIR}")
include(FindPackageHandleStandardArgs)

# if all listed variables are TRUE
find_package_handle_standard_args(catch  DEFAULT_MSG
                                  WKBHPP_LIBRARY WKBHPP_INCLUDE_DIR)
