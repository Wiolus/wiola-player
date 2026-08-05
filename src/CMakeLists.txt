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

# Written once at configure time, so the version lives in a typed constant rather than in a macro
# handed to every translation unit.
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/core/version.hpp"
    @ONLY
)

add_library(wiola_core INTERFACE)
target_include_directories(
    wiola_core
    INTERFACE "${PROJECT_SOURCE_DIR}/include" "${PROJECT_BINARY_DIR}/generated"
)

add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(engine)

add_executable(wiola-player main.cpp)
target_link_libraries(
    wiola-player
    PRIVATE
        wiola_audio
        wiola_cli
        wiola_codec
        wiola_core
        wiola_engine
        wiola_warnings
)
