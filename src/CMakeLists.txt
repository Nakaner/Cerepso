#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(pgimporter pgimporter.cpp myhandler.cpp columns.cpp table.cpp relation_collector.cpp postgres_handler.cpp append_handler.cpp)
target_link_libraries(pgimporter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY})
install(TARGETS pgimporter DESTINATION bin)

