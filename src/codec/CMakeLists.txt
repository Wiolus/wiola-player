set(SOURCES
    decoder.cpp
    file_stream.cpp
    flac_reader.cpp
    format.cpp
    mp3_reader.cpp
    open.cpp
    wav_reader.cpp
)

add_library(wiola_codec STATIC ${SOURCES})
target_include_directories(wiola_codec PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(wiola_codec SYSTEM PRIVATE "${dr_libs_SOURCE_DIR}")
target_link_libraries(wiola_codec PRIVATE wiola_warnings)
