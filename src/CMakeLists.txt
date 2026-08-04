add_library(wiola_warnings INTERFACE)
target_compile_options(
    wiola_warnings
    INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall
        -Wextra
        -Wpedantic>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-interference-size>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_library(wiola_core INTERFACE)
target_include_directories(wiola_core INTERFACE "${PROJECT_SOURCE_DIR}/include")

add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(engine)

add_executable(wiola-player main.cpp)
target_link_libraries(
    wiola-player
    PRIVATE wiola_audio wiola_cli wiola_codec wiola_engine wiola_warnings
)
target_compile_definitions(
    wiola-player
    PRIVATE WIOLA_VERSION="${PROJECT_VERSION}"
)
