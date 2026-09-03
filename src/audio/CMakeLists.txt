add_subdirectory(device)
add_subdirectory(dsp)

wiola_add_module(wiola_audio LIBS wiola_audio_device wiola_audio_dsp)
