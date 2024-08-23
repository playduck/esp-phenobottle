#include <math.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

#include "timer.h"
#include "measurement.h"
#include "ringbuffer.h"
#include "sensors/od_sensor.h"

static const char *TAG = "OD";

extern QueueHandle_t xMeasurementQueue;
static ringbuffer_t buffer;

esp_err_t od_init()
{
    return ESP_OK;
}

esp_err_t od_start()
{
    return ESP_OK;
}

static uint32_t tmp_counter = 0;
esp_err_t od_update()
{
    // take od sensor reading
    vTaskDelay(pdMS_TO_TICKS(1000));
    float od = cosf(tmp_counter++ / 500.0) * 100;
    buffer_write(&buffer, toFixed(od));
    ESP_LOGD(TAG, "od: %f", od);
    vTaskDelay(pdMS_TO_TICKS(1000));

    return ESP_OK;
}

esp_err_t od_publish()
{
    uint32_t time = 0;
    get_current_time(&time);

    // decimation filter
    uint16_t count = 0;
    float accumulator = 0.0f;
    int32_t tmp = 0;
    while (buffer_read(&buffer, &tmp) == ESP_OK)
    {
        accumulator += toFloat(tmp);
        count++;
    }

    // simple variable MVA
    float averageOD = accumulator / (float)count;

    // post downsapled sensor reading
    measurement_t measurement = {
        .timestamp = time,
        .type = "OD",
        .value = averageOD};
    if (xQueueSend(xMeasurementQueue, &measurement, pdTICKS_TO_MS(500)) != pdPASS)
    {
        ESP_LOGE(TAG, "Cannot insert message into queue");
    }

    return ESP_OK;
}

esp_err_t od_end()
{
    return ESP_OK;
}
