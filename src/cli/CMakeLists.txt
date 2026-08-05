set(SOURCES cli.cpp)

add_library(wiola_cli STATIC ${SOURCES})
target_include_directories(wiola_cli PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(wiola_cli PRIVATE wiola_core wiola_warnings CLI11::CLI11)
