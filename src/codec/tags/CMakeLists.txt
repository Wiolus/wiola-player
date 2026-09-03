wiola_add_library(wiola_codec_tags SOURCES tags.cpp LIBS tag)

# taglib says where its own headers are only for a build that installs it.
target_include_directories(
    wiola_codec_tags
    SYSTEM
    PRIVATE
        "${taglib_SOURCE_DIR}"
        "${taglib_SOURCE_DIR}/taglib"
        "${taglib_BINARY_DIR}"
)
