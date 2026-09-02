wiola_add_library(
    wiola_codec
    LIBS tag
    SOURCES
        decoder.cpp
        file_stream.cpp
        flac_reader.cpp
        format.cpp
        mp3_reader.cpp
        open.cpp
        tags.cpp
        wav_reader.cpp
)

# dr_libs is a set of headers with no target of its own, and taglib says where its own are only
# for a build that installs it.
target_include_directories(
    wiola_codec
    SYSTEM
    PRIVATE
        "${dr_libs_SOURCE_DIR}"
        "${taglib_SOURCE_DIR}"
        "${taglib_SOURCE_DIR}/taglib"
        "${taglib_BINARY_DIR}"
)
