set(SOURCES device.cpp tone.cpp)

add_library(wiola_audio STATIC ${SOURCES})
target_include_directories(wiola_audio PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(wiola_audio PRIVATE wiola_warnings miniaudio)
