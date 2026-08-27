# The reader tests reach headers that are private to the codec layer.
set(readers "${PROJECT_SOURCE_DIR}/src/codec")
set(fixtures "${CMAKE_CURRENT_SOURCE_DIR}/data")

wiola_add_test(
    codec_wav_reader_test
    SOURCES wav_reader_test.cpp
    LIBS wiola_codec
    INCLUDES "${readers}"
    DATA "${fixtures}"
)
wiola_add_test(
    codec_flac_reader_test
    SOURCES flac_reader_test.cpp
    LIBS wiola_codec
    INCLUDES "${readers}"
    DATA "${fixtures}"
)
wiola_add_test(
    codec_mp3_reader_test
    SOURCES mp3_reader_test.cpp
    LIBS wiola_codec
    INCLUDES "${readers}"
    DATA "${fixtures}"
)
wiola_add_test(
    codec_format_test
    SOURCES format_test.cpp
    LIBS wiola_codec
    INCLUDES "${readers}"
)
wiola_add_test(
    codec_decoder_test
    SOURCES decoder_test.cpp
    LIBS wiola_codec
    DATA "${fixtures}"
)
