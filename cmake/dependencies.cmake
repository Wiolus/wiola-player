# Everything the project fetches, and the versions it is fixed to. Declared in one place so that
# the set of dependencies can be read without opening every subdirectory.

include(FetchContent)

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

FetchContent_MakeAvailable(miniaudio cli11 dr_libs)

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
