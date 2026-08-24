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

# Coverage instrumentation, carried by a target of its own so that only what links it is measured:
# the project's code, never a fetched dependency. Empty unless asked for.
add_library(wiola_coverage INTERFACE)

if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(
            FATAL_ERROR
            "ENABLE_COVERAGE needs Clang: the flags are LLVM's, not GCC's."
        )
    endif()

    # Counts are per line, so the source has to still be laid out in lines: inlining and hoisting
    # move them elsewhere and the report stops matching what was written.
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(
            WARNING
            "ENABLE_COVERAGE outside a Debug build reports optimized line counts."
        )
    endif()

    target_compile_options(
        wiola_coverage
        INTERFACE -fprofile-instr-generate -fcoverage-mapping
    )
    target_link_options(wiola_coverage INTERFACE -fprofile-instr-generate)
endif()

# A sanitizer, carried the same way and for the same reason: what a fetched dependency does is not
# what we are asking about. Empty unless asked for.
add_library(wiola_sanitizer INTERFACE)

if(ENABLE_SANITIZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_SANITIZER needs GCC or Clang.")
    endif()

    # Frame pointers, so a report names the calls that led there.
    target_compile_options(
        wiola_sanitizer
        INTERFACE "-fsanitize=${ENABLE_SANITIZER}" -fno-omit-frame-pointer -g
    )
    target_link_options(
        wiola_sanitizer
        INTERFACE "-fsanitize=${ENABLE_SANITIZER}"
    )
endif()

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
        PRIVATE wiola_coverage wiola_sanitizer wiola_warnings ${arg_LIBS}
    )
endfunction()

# Gives `target` the icon Windows shows for it. Does nothing elsewhere, where an executable carries
# no resources of its own.
function(wiola_set_icon target icon)
    if(NOT WIN32)
        return()
    endif()

    enable_language(RC)

    set(resource "${PROJECT_BINARY_DIR}/generated/${target}.rc")

    configure_file("${PROJECT_SOURCE_DIR}/cmake/icon.rc.in" "${resource}" @ONLY)

    # The generated file does not change when the icon does, so the icon is named as a dependency.
    set_source_files_properties(
        "${resource}"
        PROPERTIES OBJECT_DEPENDS "${icon}"
    )
    target_sources(${target} PRIVATE "${resource}")
endfunction()
