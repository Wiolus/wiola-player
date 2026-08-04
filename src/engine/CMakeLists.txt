add_library(wiola_engine INTERFACE)
target_include_directories(
    wiola_engine
    INTERFACE "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(wiola_engine INTERFACE wiola_audio)
