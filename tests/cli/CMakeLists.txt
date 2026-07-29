set(SOURCES cli_test.cpp)

wiola_add_test(cli_test ${SOURCES})
target_link_libraries(cli_test PRIVATE wiola_cli wiola_warnings)
