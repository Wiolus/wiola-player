# How everything in this project is built. Stated once here rather than repeated per target, so
# that a new library is a list of sources and nothing else.

# A build with no type gets no optimisation at all. That is not a neutral default here: the
# decoder feeds a callback that must answer within a device period, and unoptimised it runs
# several times slower. Anyone who names a type still gets what they asked for.
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel
)

# What the project is compiled with. Private to us: a consumer's code is not our business.
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

# Written at configure time, so the version is a typed constant rather than a macro handed to
# every translation unit.
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/core/version.hpp"
    @ONLY
)

# Where the project's public headers are, the written ones and the generated one alike.
add_library(wiola_core INTERFACE)
target_include_directories(
    wiola_core
    INTERFACE "${PROJECT_SOURCE_DIR}/include" "${PROJECT_BINARY_DIR}/generated"
)

# Adds a library built the way this project builds them: it can see the public headers, and it is
# compiled with the project's warnings. `LIBS` is whatever else it needs, kept private, since a
# static library's dependencies are its own business.
function(wiola_add_library name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;LIBS")

    add_library(${name} STATIC ${arg_SOURCES})
    target_link_libraries(
        ${name}
        PUBLIC wiola_core
        PRIVATE wiola_warnings ${arg_LIBS}
    )
endfunction()
