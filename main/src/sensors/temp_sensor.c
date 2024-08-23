#include <math.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

#include "timer.h"
#include "measurement.h"
#include "ringbuffer.h"
#include "sensors/temp_sensor.h"

static const char *TAG = "TEMP";

extern QueueHandle_t xMeasurementQueue;
ringbuffer_t buffer;

esp_err_t temp_init()
{
    // TODO: init temp sensor and tec driver
    return ESP_OK;
}

esp_err_t temp_start()
{
    return ESP_OK;
}
uint32_t tmp_counter = 0;
esp_err_t temp_update()
{
    // take temp sensor reading
    float temp = sinf(tmp_counter++ / 250.0) * 20.0 + 10.0; // FIXME
    buffer_write(&buffer, toFixed(temp));
    ESP_LOGD(TAG, "Temp: %f", temp);

    // perform tec control loop
    return ESP_OK;
}

esp_err_t temp_publish()
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
    float averageTemp = accumulator / (float)count;

    // post downsapled sensor reading
    measurement_t measurement = {
        .timestamp = time,
        .type = "Temperature",
        .value = averageTemp};
    if (xQueueSend(xMeasurementQueue, &measurement, pdTICKS_TO_MS(500)) != pdPASS)
    {
        ESP_LOGE(TAG, "Cannot insert message into queue");
    }

    return ESP_OK;
}

esp_err_t temp_end()
{
    return ESP_OK;
}
