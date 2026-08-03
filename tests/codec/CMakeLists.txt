set(SOURCES wav_reader_test.cpp)

wiola_add_test(codec_wav_reader_test ${SOURCES})
target_link_libraries(
    codec_wav_reader_test
    PRIVATE wiola_codec wiola_core wiola_warnings
)
