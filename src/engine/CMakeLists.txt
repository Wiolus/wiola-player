wiola_add_library(
    wiola_engine
    SOURCES playback.cpp playhead.cpp player.cpp session.cpp
    LIBS wiola_audio wiola_codec
)
