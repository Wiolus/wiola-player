wiola_add_test(engine_playlist_test SOURCES playlist_test.cpp LIBS wiola_engine)
wiola_add_test(engine_loader_test SOURCES loader_test.cpp LIBS wiola_engine)
wiola_add_test(engine_playback_test SOURCES playback_test.cpp LIBS wiola_engine)
wiola_add_test(engine_playhead_test SOURCES playhead_test.cpp LIBS wiola_engine)
wiola_add_test(
    engine_player_test
    SOURCES player_test.cpp
    LIBS wiola_codec wiola_engine
    DATA "${PROJECT_SOURCE_DIR}/tests/codec/data"
)
wiola_add_test(engine_session_test SOURCES session_test.cpp LIBS wiola_engine)
