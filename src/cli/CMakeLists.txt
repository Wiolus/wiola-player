include(FetchContent)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS NAMES CLI11
)
FetchContent_MakeAvailable(cli11)

set(SOURCES cli.cpp)

add_library(wiola_cli STATIC ${SOURCES})
target_include_directories(wiola_cli PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(wiola_cli PRIVATE wiola_warnings CLI11::CLI11)
target_compile_definitions(wiola_cli PRIVATE WIOLA_VERSION="${PROJECT_VERSION}")
