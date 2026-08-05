wiola_add_test(
    engine_player_test
    SOURCES player_test.cpp
    LIBS wiola_codec wiola_engine
    DATA "${PROJECT_SOURCE_DIR}/tests/codec/data"
)
