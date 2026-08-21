add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(engine)
add_subdirectory(gui)

add_executable(wiola-player main.cpp)
target_link_libraries(
    wiola-player
    PRIVATE wiola_cli wiola_core wiola_gui wiola_warnings
)

# Windows opens a console for anything but a GUI binary. Ignored on other systems.
set_target_properties(wiola-player PROPERTIES WIN32_EXECUTABLE ON)

wiola_set_icon(wiola-player "${PROJECT_SOURCE_DIR}/assets/wiola-player.ico")

# A static Qt loads no plugins at runtime, so the platform plugin has to be linked in.
qt_finalize_target(wiola-player)
