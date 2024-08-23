#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <cJSON.h>

#include "client.h"
#include "endpoints.h"
#include "measurement.h"

static const char *TAG = "Measure";

#define QUEUE_SIZE 10
#define TMP_BUFFER_LENGTH 128
QueueHandle_t xMeasurementQueue;

cJSON *serialize_measurement(measurement_t *measurement)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL)
    {
        return NULL;
    }

    cJSON *measurement_type = cJSON_CreateString(measurement->type);
    cJSON *value = cJSON_CreateNumber(measurement->value);
    cJSON *timestamp = cJSON_CreateNumber(measurement->timestamp);

    if (measurement_type == NULL || value == NULL || timestamp == NULL)
    {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_AddItemToObject(json, "measurement_type", measurement_type);
    cJSON_AddItemToObject(json, "value", value);
    cJSON_AddItemToObject(json, "timestamp", timestamp);

    return json;
}

esp_err_t post_measurement(measurement_t *measurement)
{
    esp_err_t ret = ESP_FAIL;
    char tmp_buf[TMP_BUFFER_LENGTH];
    char *json_string = NULL;

    cJSON *json = serialize_measurement(measurement);
    if (json == NULL)
    {
        goto end;
    }

    json_string = cJSON_PrintUnformatted(json);
    if (json_string == NULL)
    {
        goto end;
    }

    ESP_LOGI(TAG, "Serialized JSON: %s", json_string);

    // get config (blocking)
    esp_http_client_config_t* config = get_config();
    config->url = API_V1_POST_MEASUREMENT;
    esp_http_client_handle_t client = esp_http_client_init(config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);

    // assemble request headers
    sprintf(tmp_buf, "%d", 1); // FIXME
    esp_http_client_set_header(client, "Device-Id", tmp_buf);
    sprintf(tmp_buf, "%lu", measurement->timestamp);
    esp_http_client_set_header(client, "Timestamp", tmp_buf);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    // excecute request and wait for response
    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(ret));
    }

    // finalize
    esp_http_client_cleanup(client);
    release_config();

end:
    if (json != NULL)
    {
        cJSON_Delete(json);
    }

    if (json_string != NULL)
    {
        free(json_string);
    }

    return ret;
}

void send_measurement_task(void *pvparameters)
{
    ESP_LOGI(TAG, "Starting measurement task");

    xMeasurementQueue = xQueueCreate(QUEUE_SIZE, sizeof(measurement_t));
    if (xMeasurementQueue == NULL)
    {
        ESP_LOGE(TAG, "Cannot create Queue");
    }

    while (1)
    {
        measurement_t measurement;
        if (xQueueReceive(xMeasurementQueue, &measurement, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to get measurement from Queue");
        }
        else
        {
            ESP_LOGI(TAG, "[%lu] %s: %f", measurement.timestamp, measurement.type, measurement.value);
            post_measurement(&measurement);
        }
    }
}
