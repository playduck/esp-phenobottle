#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "measurement.h"

static const char *TAG = "Measure";

#define QUEUE_SIZE 10
QueueHandle_t xMeasurementQueue;

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
        }
    }
}
