idf_component_register(SRCS "https_request_example_main.c" "time_sync.c"
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES server_root_cert.pem)
