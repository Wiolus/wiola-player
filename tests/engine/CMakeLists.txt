set(SOURCES player_test.cpp)

wiola_add_test(engine_player_test ${SOURCES})
target_link_libraries(
    engine_player_test
    PRIVATE wiola_codec wiola_core wiola_engine wiola_warnings
)
target_compile_definitions(
    engine_player_test
    PRIVATE WIOLA_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/codec/data"
)
