wiola_add_test(audio_stream_spec_test SOURCES stream_spec_test.cpp)
wiola_add_test(audio_chain_test SOURCES chain_test.cpp LIBS wiola_audio)
wiola_add_test(audio_tone_test SOURCES tone_test.cpp LIBS wiola_audio)
wiola_add_test(audio_device_test SOURCES device_test.cpp LIBS wiola_audio)
