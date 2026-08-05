include(FetchContent)

FetchContent_Declare(
    dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    # The repository tags each decoder on its own and holds all three in one tree, so any one
    # tag pins the other two as they stood beside it.
    GIT_TAG wav-0.14.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(dr_libs)

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
