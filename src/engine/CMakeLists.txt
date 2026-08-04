set(SOURCES play.cpp)

add_library(wiola_engine STATIC ${SOURCES})
target_include_directories(wiola_engine PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(
    wiola_engine
    PRIVATE wiola_audio wiola_codec wiola_warnings
)
