add_executable(wiola-player
    main.cpp
)

target_include_directories(wiola-player PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(wiola-player PRIVATE
    WIOLA_VERSION="${PROJECT_VERSION}"
)

target_compile_options(wiola-player PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
