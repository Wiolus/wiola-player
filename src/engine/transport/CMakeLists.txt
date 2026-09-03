# Keeps one track fed to one output. Nothing here shapes samples, so the DSP is not among these.
wiola_add_library(
    wiola_engine_transport
    SOURCES decode_loop.cpp playback.cpp player.cpp playhead.cpp
    LIBS wiola_audio_device wiola_codec_decode
)
