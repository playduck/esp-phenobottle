#include <math.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

#include "timer.h"
#include "measurement.h"
#include "ringbuffer.h"
#include "sensors/gas_sensor.h"

static const char *TAG = "CO2";

extern QueueHandle_t xMeasurementQueue;
static ringbuffer_t buffer;

esp_err_t gas_init()
{
    return ESP_OK;
}

esp_err_t gas_start()
{
    return ESP_OK;
}

static uint32_t tmp_counter = 0;
esp_err_t gas_update()
{
    // take gas sensor reading
    float co2 = cosf(tmp_counter++ / 40.0) * 100;
    buffer_write(&buffer, toFixed(co2));
    ESP_LOGD(TAG, "gas: %f", co2);

    return ESP_OK;
}

esp_err_t gas_publish()
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
    float averageCO2 = accumulator / (float)count;

    // post downsapled sensor reading
    measurement_t measurement = {
        .timestamp = time,
        .type = "CO2",
        .value = averageCO2};
    if (xQueueSend(xMeasurementQueue, &measurement, pdTICKS_TO_MS(500)) != pdPASS)
    {
        ESP_LOGE(TAG, "Cannot insert message into queue");
    }

    return ESP_OK;
}

esp_err_t gas_end()
{
    return ESP_OK;
}
