cmake_minimum_required(VERSION 3.28)

project(
    wiola-player
    VERSION 0.1.0
    DESCRIPTION "Wiola media player"
    LANGUAGES CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Use: cmake -B build")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(ENABLE_UNIT_TESTS "Build unit tests" ON)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING AND ENABLE_UNIT_TESTS)
    add_subdirectory(tests)
endif()
