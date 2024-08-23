#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

esp_err_t client_init();
esp_http_client_config_t* get_config();
void release_config();
