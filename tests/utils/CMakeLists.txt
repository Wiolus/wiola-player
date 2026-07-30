set(SOURCES units_test.cpp)

wiola_add_test(utils_units_test ${SOURCES})
target_link_libraries(utils_units_test PRIVATE wiola_core wiola_warnings)
