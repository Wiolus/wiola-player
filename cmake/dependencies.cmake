# Everything the project fetches, and the versions it is fixed to. Declared in one place so that
# the set of dependencies can be read without opening every subdirectory.

include(FetchContent)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(MINIAUDIO_NO_LIBVORBIS ON)
set(MINIAUDIO_NO_LIBOPUS ON)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS NAMES CLI11
)

FetchContent_Declare(
    dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    # The repository tags each decoder on its own and holds all three in one tree, so any one
    # tag pins the other two as they stood beside it.
    GIT_TAG wav-0.14.5
    GIT_SHALLOW TRUE
)

# What a file says about itself, which every player needs and none should parse for itself: a
# tag is a stranger's bytes, and this is the library everyone else's fuzzing has already been
# through.
set(BUILD_TESTING OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_BINDINGS OFF)
set(WITH_ZLIB OFF)

FetchContent_Declare(
    taglib
    GIT_REPOSITORY https://github.com/taglib/taglib.git
    GIT_TAG v2.0.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(miniaudio cli11 dr_libs taglib)

# Taglib's headers sit in a directory each and include one another by bare name, and it says
# where they are only for a build that installs it. A build that fetches it has to say instead.
file(GLOB_RECURSE wiola_taglib_headers "${taglib_SOURCE_DIR}/taglib/*.h")

set(wiola_taglib_dirs "${taglib_SOURCE_DIR}" "${taglib_BINARY_DIR}")

foreach(header IN LISTS wiola_taglib_headers)
    get_filename_component(wiola_taglib_dir "${header}" DIRECTORY)
    list(APPEND wiola_taglib_dirs "${wiola_taglib_dir}")
endforeach()

# Built into this program rather than beside it, which its headers have to be told: without this
# they declare everything as coming from a library that is loaded, and nothing links on Windows.
target_compile_definitions(tag INTERFACE TAGLIB_STATIC)

list(REMOVE_DUPLICATES wiola_taglib_dirs)

# Only for what is built here: taglib exports this target for installing, and a path into a build
# directory is not one an installed target may carry.
foreach(wiola_taglib_dir IN LISTS wiola_taglib_dirs)
    target_include_directories(
        tag
        SYSTEM
        INTERFACE "$<BUILD_INTERFACE:${wiola_taglib_dir}>"
    )
endforeach()

# Nothing but the tests needs this, and a build without them should not go to the network for it.
if(WIOLA_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
        FIND_PACKAGE_ARGS NAMES GTest
    )

    FetchContent_MakeAvailable(googletest)
endif()
