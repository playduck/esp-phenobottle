file(GLOB SOURCE_FILES "src/*.c" "src/**/*.c")

idf_component_register(SRCS ${SOURCE_FILES}
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES server_root_cert.pem
                    PRIV_REQUIRES nvs_flash esp_psram esp_wifi esp_timer esp-tls esp_http_client json cat9555
)

target_compile_options(${COMPONENT_LIB} PUBLIC -std=c++23)
