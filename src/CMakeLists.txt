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

add_library(wiola_core STATIC cli.cpp)
target_include_directories(wiola_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(wiola_core PRIVATE wiola_warnings)

add_executable(wiola-player main.cpp)
target_link_libraries(wiola-player PRIVATE wiola_core wiola_warnings)
target_compile_definitions(
    wiola-player
    PRIVATE WIOLA_VERSION="${PROJECT_VERSION}"
)
