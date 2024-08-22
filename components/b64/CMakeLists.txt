file(GLOB SOURCE_FILES "b64.c/buffer.c" "b64.c/decode.c" "b64.c/encode.c")

idf_component_register(SRCS ${SOURCE_FILES} INCLUDE_DIRS "b64.c")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=char-subscripts)
