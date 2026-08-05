add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(codec)
add_subdirectory(engine)

add_executable(wiola-player main.cpp)
target_link_libraries(
    wiola-player
    PRIVATE
        wiola_audio
        wiola_cli
        wiola_codec
        wiola_core
        wiola_engine
        wiola_warnings
)
