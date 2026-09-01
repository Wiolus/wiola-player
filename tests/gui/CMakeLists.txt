wiola_add_test(
    gui_playlist_control_test
    SOURCES playlist_control_test.cpp
    LIBS wiola_gui
)
wiola_add_test(
    gui_equalizer_control_test
    SOURCES equalizer_control_test.cpp
    LIBS wiola_gui
)
wiola_add_test(
    gui_volume_control_test
    SOURCES volume_control_test.cpp
    LIBS wiola_gui
)
