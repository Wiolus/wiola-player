include(FetchContent)

FetchContent_Declare(
    dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(dr_libs)

set(SOURCES decoder.cpp flac_reader.cpp mp3_reader.cpp wav_reader.cpp)

add_library(wiola_codec STATIC ${SOURCES})
target_include_directories(wiola_codec PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(wiola_codec SYSTEM PRIVATE "${dr_libs_SOURCE_DIR}")
target_link_libraries(wiola_codec PRIVATE wiola_warnings)
