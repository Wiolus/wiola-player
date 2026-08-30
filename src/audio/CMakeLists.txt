wiola_add_library(
    wiola_audio
    SOURCES
        buffer_source.cpp
        chain.cpp
        device.cpp
        equalizer.cpp
        shaped_source.cpp
        volume.cpp
    LIBS miniaudio
)
