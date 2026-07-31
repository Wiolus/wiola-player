set(SOURCES stream_spec_test.cpp)

wiola_add_test(audio_stream_spec_test ${SOURCES})
target_link_libraries(audio_stream_spec_test PRIVATE wiola_core wiola_warnings)

set(SOURCES tone_test.cpp)

wiola_add_test(audio_tone_test ${SOURCES})
target_link_libraries(
    audio_tone_test
    PRIVATE wiola_audio wiola_core wiola_warnings
)
