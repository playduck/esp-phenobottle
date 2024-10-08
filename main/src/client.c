#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_sntp.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_psram.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "client.h"

static const char *TAG = "CLIENT";

#define WEB_PORT "443"
#define WEB_SERVER "warr.robin-prillwitz.de"
#define API_STATE_ENDPOINT "https://warr.robin-prillwitz.de/api/v1/state/1"

static char USER_AGENT[32];

extern const char server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const char server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

char requestBuffer[1024];

#define OUTPUT_BUFFER_SIZE (4096)
char output_buffer[OUTPUT_BUFFER_SIZE];

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static int output_len = 0;
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // Clean the buffer in case of a new request
        if (output_len == 0 && evt->user_data)
        {
            // we are just starting to copy the output data into the use
            memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
        }
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data)
            {
                // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                if (copy_len)
                {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            }
            else
            {
                int content_len = esp_http_client_get_content_length(evt->client);
                if (output_len == 0)
                {
                    // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                    // output_buffer = (char *)calloc(content_len + 1, sizeof(char));
                    if (content_len > OUTPUT_BUFFER_SIZE)
                    {
                        ESP_LOGE(TAG, "Content length greater than buffer size");
                        return ESP_FAIL;
                    }
                    memset(output_buffer, '\0', OUTPUT_BUFFER_SIZE);

                    output_len = 0;
                }
                copy_len = MIN(evt->data_len, (content_len - output_len));
                if (copy_len)
                {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        ESP_LOGI(TAG, "Output Buffer:\n%s\n", output_buffer);
        output_len = 0;

        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }

        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

SemaphoreHandle_t xHttpSemaphore = NULL;
esp_http_client_config_t config = {
    .event_handler = _http_event_handler,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,
    .cert_pem = server_root_cert_pem_start,
    .tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_2,
    .username = CONFIG_USERNAME,
    .password = CONFIG_PASSWORD,
    .auth_type = HTTP_AUTH_TYPE_BASIC,
    .max_authorization_retries = -1,
};

esp_err_t client_init()
{
    esp_err_t ret = ESP_OK;

    // build user agent string
    sprintf(USER_AGENT, "esp-idf/%d.%d.%d esp32", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    config.user_agent = USER_AGENT;

    // init access control mutex
    if (xHttpSemaphore == NULL)
    {
        xHttpSemaphore = xSemaphoreCreateMutex();
        if (xHttpSemaphore == NULL)
        {
            ESP_LOGE(TAG, "Cannot create mutex");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Mutex already initilized");
    }

    ESP_LOGI(TAG, "HTTP Client init done");
    return ret;
}

esp_http_client_config_t* get_config()
{
    if (xHttpSemaphore != NULL)
    {
        // TODO: maybe timeout instead of spinlock?
        if (xSemaphoreTake(xHttpSemaphore, portMAX_DELAY) == pdTRUE)
        {
            return &config;
        }   else    {
            ESP_LOGW(TAG, "Mutex is unavailable");
        }
    }   else    {
        ESP_LOGE(TAG, "Mutex is uninitilized");
    }

    return NULL;
}

void release_config()
{
    config.user_data = NULL;
    config.url = "";

    if (xHttpSemaphore != NULL)
    {
        xSemaphoreGive(xHttpSemaphore);
    }
}
