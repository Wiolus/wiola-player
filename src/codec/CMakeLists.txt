add_subdirectory(decode)
add_subdirectory(tags)

wiola_add_module(wiola_codec LIBS wiola_codec_decode wiola_codec_tags)
