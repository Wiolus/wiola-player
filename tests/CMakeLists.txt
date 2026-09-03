include(GoogleTest)

function(wiola_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "DATA" "SOURCES;LIBS")

    add_executable(${name} ${arg_SOURCES})
    target_link_libraries(
        ${name}
        PRIVATE
            GTest::gtest_main
            wiola_core
            wiola_coverage
            wiola_sanitizer
            wiola_warnings
            ${arg_LIBS}
    )
    set_target_properties(
        ${name}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )

    # What the tests share among themselves. The project's own headers arrive with wiola_core.
    target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}/tests")

    if(arg_DATA)
        target_compile_definitions(
            ${name}
            PRIVATE WIOLA_TEST_DATA_DIR="${arg_DATA}"
        )
    endif()

    gtest_discover_tests(${name})
endfunction()

add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(core)
add_subdirectory(engine)
add_subdirectory(gui)
add_subdirectory(lockfree)
add_subdirectory(utils)
