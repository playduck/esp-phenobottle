#include "math.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_random.h" // FIXME

#include "temp_sensor.h"

static const char* TAG = "TEMP";

#define QUEUE_ELEMENTS 100
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)
int32_t Queue[QUEUE_SIZE];
uint16_t QueueIn, QueueOut;


esp_err_t QueuePut(int32_t new)
{
    /*
    if(QueueIn == (( QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
    {
        return ESP_FAIL;
    }
    */

    Queue[QueueIn] = new;
    QueueIn = (QueueIn + 1) % QUEUE_SIZE;

    return ESP_OK;
}

esp_err_t QueueGet(int32_t *old)
{
    if(QueueIn == QueueOut)
    {
        return ESP_FAIL;
    }

    *old = Queue[QueueOut];
    QueueOut = (QueueOut + 1) % QUEUE_SIZE;

    return ESP_OK;
}

int32_t toFixed(float val) {
    return (int32_t)roundf(val * 1000.0f);
}

float toFloat(int32_t val) {

    return (float)val / 1000.0f;
}


esp_err_t temp_init()   {
    // TODO: init temp sensor and tec driver
    return ESP_OK;
}

esp_err_t temp_start()  {
    return ESP_OK;
}
uint32_t tmp_counter = 0;
esp_err_t temp_update() {
    // take temp sensor reading
    float temp = sinf(tmp_counter++ / 250.0)  * 20.0 + 10.0;    // FIXME
    QueuePut(toFixed(temp));
    ESP_LOGD(TAG, "Temp: %f", temp);

    // perform tec control loop
    return ESP_OK;
}

esp_err_t temp_publish()    {
    // decimation filter
    uint16_t count = 0;
    float accumulator = 0.0f;
    int32_t tmp = 0;
    while(QueueGet(&tmp) == ESP_OK) {
        accumulator += toFloat(tmp);
        count++;
    }

    // simple variable MVA
    float averageTemp = accumulator / (float)count;

    // post downsapled sensor reading
    ESP_LOGI(TAG, "Average Temp since last pub: %f", averageTemp);

    return ESP_OK;
}

esp_err_t temp_end()    {
    return ESP_OK;
}
