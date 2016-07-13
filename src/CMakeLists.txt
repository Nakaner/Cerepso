#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(pgimporter pgimporter.cpp myhandler.cpp columns.cpp table.cpp relation_collector.cpp postgres_handler.cpp)
target_link_libraries(pgimporter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARY})
install(TARGETS pgimporter DESTINATION bin)

add_executable(applyosc applyosc.cpp)
target_link_libraries(applyosc ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES})
install(TARGETS applyosc DESTINATION bin)
