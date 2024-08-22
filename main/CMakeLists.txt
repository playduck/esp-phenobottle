file(GLOB SOURCE_FILES "src/*.c" "src/*.cpp" "include/*.h")

idf_component_register(SRCS ${SOURCE_FILES}
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES server_root_cert.pem
)

target_compile_options(${COMPONENT_LIB} PUBLIC -std=c++23)
