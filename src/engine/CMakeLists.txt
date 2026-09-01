wiola_add_library(
    wiola_engine
    SOURCES
        decode_loop.cpp
        loader.cpp
        playback.cpp
        playhead.cpp
        player.cpp
        playlist.cpp
        session.cpp
        track.cpp
    LIBS wiola_audio wiola_codec
)
