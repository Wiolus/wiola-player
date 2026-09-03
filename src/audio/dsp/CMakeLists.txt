# Shaping needs no audio stack, and links nothing.
wiola_add_library(
    wiola_audio_dsp
    SOURCES
        biquad.cpp
        buffer_source.cpp
        chain.cpp
        clip.cpp
        equalizer.cpp
        relay.cpp
        shaped_source.cpp
        volume.cpp
)
