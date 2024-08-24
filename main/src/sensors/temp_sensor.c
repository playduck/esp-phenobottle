#include <math.h>

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

#include "timer.h"
#include "measurement.h"
#include "ringbuffer.h"
#include "sensors/temp_sensor.h"
#include "i2c_user.h"

#include "sht3x.h"

static const char *TAG = "TEMP";

extern QueueHandle_t xMeasurementQueue;
static ringbuffer_t buffer;

static sht3x_device_t sht3x_dev;

esp_err_t temp_init()
{
    // TODO: init temp sensor and tec driver
    sht3x_init(&sht3x_dev, address_A, I2C_USER_PORT);


    return ESP_OK;
}

esp_err_t temp_start()
{
    return ESP_OK;
}

static uint32_t tmp_counter = 0;
esp_err_t temp_update()
{
    // take temp sensor reading
    sht3x_start_measurement(&sht3x_dev, SINGLE_SHOT_CLOCK_STRETCHING_ENABLED_HIGH_RELIABILITY);

    sht3x_measurement_t measurement;
    sht3x_read_measurement(&sht3x_dev, &measurement);

    // float temp = sinf(tmp_counter++ / 250.0) * 20.0 + 10.0; // FIXME
    float temp = measurement.temperature_degC;

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
