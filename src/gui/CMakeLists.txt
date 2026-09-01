wiola_add_library(
    wiola_gui
    SOURCES
        equalizer_control.cpp
        equalizer_panel.cpp
        main_window.cpp
        playlist_control.cpp
        playlist_view.cpp
        seek_bar.cpp
        volume_control.cpp
    LIBS wiola_codec wiola_engine
)

# Qt needs its own generated sources, and its headers are part of using this library.
set_target_properties(wiola_gui PROPERTIES AUTOMOC ON)
target_include_directories(wiola_gui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(wiola_gui PUBLIC Qt6::Widgets)
