#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(pgimporter pgimporter.cpp myhandler.cpp columns.cpp table.cpp)
target_link_libraries(pgimporter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY})
install(TARGETS pgimporter DESTINATION bin)


