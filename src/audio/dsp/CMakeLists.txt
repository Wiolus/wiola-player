# Shaping needs no audio stack. The one thing holding miniaudio here is the equalizer's peaking
# filter; a biquad of our own would leave this list empty.
wiola_add_library(
    wiola_audio_dsp
    SOURCES
        buffer_source.cpp
        chain.cpp
        equalizer.cpp
        relay.cpp
        shaped_source.cpp
        volume.cpp
    LIBS miniaudio
)
