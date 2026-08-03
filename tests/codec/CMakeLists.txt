set(SOURCES wav_reader_test.cpp)

wiola_add_test(codec_wav_reader_test ${SOURCES})
target_link_libraries(
    codec_wav_reader_test
    PRIVATE wiola_codec wiola_core wiola_warnings
)

set(SOURCES mp3_reader_test.cpp)

wiola_add_test(codec_mp3_reader_test ${SOURCES})
target_link_libraries(
    codec_mp3_reader_test
    PRIVATE wiola_codec wiola_core wiola_warnings
)
target_compile_definitions(
    codec_mp3_reader_test
    PRIVATE WIOLA_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
