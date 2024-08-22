#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>

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
#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <cJSON.h>

#include "esp_tls.h"
#include "sdkconfig.h"

#include "time_sync.h"
#include "b64.h"

static const char *TAG = "MAIN";

#define WEB_PORT "443"
#define WEB_SERVER "warr.robin-prillwitz.de"
#define API_STATE_ENDPOINT "https://warr.robin-prillwitz.de/api/v1/state/1"

/* Timer interval once every day (24 Hours) */
#define TIME_PERIOD (86400000000ULL)

static char USER_AGENT[] = "esp-idf/1.0 esp32";

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

char requestBuffer[1024];
char outputBuffer[2048 * 2];

static int generateHttpRequest(const char* messageType, const char* webUrl, const char* webServer,
                         const char* authUsername, const char* authPassword, const char* messageContent) {

    int ret_val = 1;

    // Clear the request buffer
    memset(requestBuffer, 0, sizeof(requestBuffer));

    // Set the message type and URL
    sprintf(requestBuffer, "%s %s HTTP/1.1\r\n", messageType, webUrl);

    // Set the Host header
    sprintf(requestBuffer + strlen(requestBuffer), "Host: %s\r\n", webServer);

    // Set the User-Agent header
    sprintf(requestBuffer + strlen(requestBuffer), "User-Agent: %s\r\n", USER_AGENT);

    // Set the Authorization header if credentials are provided
    if (authUsername && authPassword) {
        char authString[256];
        sprintf(authString, "%s:%s", authUsername, authPassword);
        char* encodedAuthString = b64_encode((unsigned char*)authString, strnlen(authString, 256));

        sprintf(requestBuffer + strlen(requestBuffer), "Authorization: Basic %s\r\n", encodedAuthString);
    }

    // Set the Content-Length header if message content is provided
    if (messageContent) {
        sprintf(requestBuffer + strlen(requestBuffer), "Content-Length: %zu\r\n", strlen(messageContent));
    }

    // Add a blank line to indicate the end of headers
    sprintf(requestBuffer + strlen(requestBuffer), "\r\n");

    // Add the message content if provided
    if (messageContent) {
        sprintf(requestBuffer + strlen(requestBuffer), "%s", messageContent);
    }

    ESP_LOG_BUFFER_HEXDUMP(TAG, requestBuffer, strlen(requestBuffer), ESP_LOG_INFO);
    ret_val = 0;
    return ret_val;
}

static int https_get_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *REQUEST)
{
    int ret_val = 1;
    int j = 0;
    char buf[128];
    int ret, len;

    memset(outputBuffer, 0x00, sizeof(outputBuffer));

    esp_tls_t *tls = esp_tls_init();
    if (!tls)
    {
        ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
        goto exit;
    }

    if (esp_tls_conn_http_new_sync(WEB_SERVER_URL, &cfg, tls) == 1)
    {
        ESP_LOGI(TAG, "Connection established...");
    }
    else
    {
        ESP_LOGE(TAG, "Connection failed...");
        int esp_tls_code = 0, esp_tls_flags = 0;
        esp_tls_error_handle_t tls_e = NULL;
        esp_tls_get_error_handle(tls, &tls_e);
        /* Try to get TLS stack level error and certificate failure flags, if any */
        ret = esp_tls_get_and_clear_last_error(tls_e, &esp_tls_code, &esp_tls_flags);
        if (ret == ESP_OK)
        {
            ESP_LOGE(TAG, "TLS error = -0x%x, TLS flags = -0x%x", esp_tls_code, esp_tls_flags);
        }
        goto cleanup;
    }

    size_t written_bytes = 0;
    do
    {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 strlen(REQUEST) - written_bytes);
        if (ret >= 0)
        {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        }
        else if (ret != ESP_TLS_ERR_SSL_WANT_READ && ret != ESP_TLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto cleanup;
        }
    } while (written_bytes < strlen(REQUEST));

    ESP_LOGI(TAG, "Reading HTTP response...");
    do
    {
        len = sizeof(buf) - 1;
        memset(buf, 0x00, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE || ret == ESP_TLS_ERR_SSL_WANT_READ)
        {
            ESP_LOGW(TAG, "continuation ret val [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            continue;
        }
        else if (ret < 0)
        {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        }
        else if (ret == 0)
        {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        for (int i = 0; i < len; i++)
        {
            outputBuffer[j] = buf[i];
            j = j + 1;
        }

        ssize_t available = esp_tls_get_bytes_avail(tls);
        ESP_LOGD(TAG, "%d bytes available", available);
        if (available <= 0)
        {
            break;
        }
    } while (1);
    ret_val = 0;

cleanup:
    esp_tls_conn_destroy(tls);
exit:
    return ret_val;
}

static int parseState()
{
    char *jsonString = NULL;
    char *headerEnd = NULL;
    char *jsonStart = NULL;

    cJSON *root = NULL;

    int ret_val = 1;

    // Find the end of the HTTP response header
    headerEnd = strstr(outputBuffer, "\r\n\r\n");
    if (!headerEnd)
    {
        ESP_LOGE(TAG, "Error: unable to find end of HTTP response header\n");
        goto end;
    }

    // Extract the JSON string
    jsonStart = headerEnd + 4; // to skip the newline characters
    jsonString = malloc(strlen(jsonStart) + 1);
    strcpy(jsonString, jsonStart);

    printf(jsonString);

    root = cJSON_ParseWithLength(jsonString, 4096);
    if (!root)
    {
        ESP_LOGE(TAG, "Error parsing JSON\n");
        goto end;
    }

    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (!state)
    {
        ESP_LOGE(TAG, "Error: 'state' not found\n");
        goto end;
    }

    ESP_LOGI(TAG, "State: %s\n", cJSON_Print(state));

    cJSON *tasks = cJSON_GetObjectItem(root, "tasks");
    if (!tasks)
    {
        ESP_LOGE(TAG, "Error: 'tasks' not found\n");
        goto end;
    }

    cJSON *task;
    int task_count = cJSON_GetArraySize(tasks);
    for (int i = 0; i < task_count; i++)
    {
        task = cJSON_GetArrayItem(tasks, i);

        cJSON *task_id = cJSON_GetObjectItem(task, "task_id");
        cJSON *device_id = cJSON_GetObjectItem(task, "device_id");
        cJSON *task_name = cJSON_GetObjectItem(task, "task_name");
        cJSON *task_type = cJSON_GetObjectItem(task, "task_type");
        cJSON *task_start = cJSON_GetObjectItem(task, "task_start");
        cJSON *task_duration = cJSON_GetObjectItem(task, "task_duration");

        printf("Task:\n");
        printf("  task_id: %d\n", task_id->valueint);
        printf("  device_id: %d\n", device_id->valueint);
        printf("  task_name: %s\n", task_name->valuestring);
        printf("  task_type: %s\n", task_type->valuestring);
        printf("  task_start: %s\n", task_start->valuestring);
        printf("  task_duration: %s\n", task_duration->valuestring);
        cJSON *task_period = cJSON_GetObjectItem(task, "task_period");
        if (cJSON_IsNull(task_period))
        {
            printf("  task_period: null\n");
        }
        else
        {
            printf("  task_period: %s\n", task_period->valuestring);
        }

        printf("\n");
    }

    cJSON *settings = cJSON_GetObjectItem(root, "settings");
    if (settings)
    {
        printf("Settings: %s\n", cJSON_Print(settings));
    }

    cJSON *actions = cJSON_GetObjectItem(root, "actions");
    if (actions)
    {
        printf("Actions: %s\n", cJSON_Print(actions));
    }
    ret_val = 0;

end:
    if (root)
    {
        cJSON_Delete(root);
    }
    free(jsonString);
    return ret_val;
}

static void https_request_task(void *pvparameters)
{
    ESP_LOGI(TAG, "Start https_request example");

    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *)server_root_cert_pem_start,
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };
    generateHttpRequest("GET", API_STATE_ENDPOINT, WEB_SERVER, CONFIG_USERNAME, CONFIG_PASSWORD, NULL);
    if(https_get_request(cfg, WEB_SERVER, requestBuffer) == 0)  {
        parseState();
    }

    ESP_LOGI(TAG, "Finish https_request example");
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        ESP_LOGI(TAG, "Updating time from NVS");
        ESP_ERROR_CHECK(update_time_from_nvs());
    }

    const esp_timer_create_args_t nvs_update_timer_args = {
        .callback = (void *)&fetch_and_store_time_in_nvs,
    };

    esp_timer_handle_t nvs_update_timer;
    ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, TIME_PERIOD));

    xTaskCreate(&https_request_task, "https_get_task", 8192 * 4, NULL, 5, NULL);
}
