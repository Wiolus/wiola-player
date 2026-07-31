include(FetchContent)

set(MINIAUDIO_NO_LIBVORBIS ON)
set(MINIAUDIO_NO_LIBOPUS ON)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

set(SOURCES device.cpp)

add_library(wiola_audio STATIC ${SOURCES})
target_include_directories(wiola_audio PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(wiola_audio PRIVATE wiola_warnings miniaudio)
