set(SOURCES cache_line_test.cpp)

wiola_add_test(cache_line_test ${SOURCES})
target_link_libraries(cache_line_test PRIVATE wiola_core wiola_warnings)
