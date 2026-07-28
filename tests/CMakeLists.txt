include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS NAMES GTest
)
FetchContent_MakeAvailable(googletest)

add_executable(cli_test cli_test.cpp)
target_link_libraries(
    cli_test
    PRIVATE wiola_core wiola_warnings GTest::gtest_main
)

add_executable(cache_line_test core/cache_line_test.cpp)
target_link_libraries(
    cache_line_test
    PRIVATE wiola_core wiola_warnings GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(cli_test)
gtest_discover_tests(cache_line_test)
