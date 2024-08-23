#include <stdint.h>
#include <math.h>
#include "esp_err.h"

#include "ringbuffer.h"

esp_err_t buffer_write(ringbuffer_t* buffer, int32_t new)
{
    // overwrite old data
    /*

    if(bufferIn == (( bufferOut - 1 + BUFFER_SIZE) % BUFFER_SIZE))
    {
        return ESP_FAIL;
    }
    */

    buffer->list[buffer->in] = new;
    buffer->in = (buffer->in + 1) % BUFFER_SIZE;

    return ESP_OK;
}

esp_err_t buffer_read(ringbuffer_t* buffer, int32_t *old)
{
    if (buffer->in == buffer->out)
    {
        return ESP_FAIL;
    }

    *old = buffer->list[buffer->out];
    buffer->out = (buffer->out + 1) % BUFFER_SIZE;

    return ESP_OK;
}

int32_t toFixed(float val)
{
    return (int32_t)roundf(val * 1000.0f);
}

float toFloat(int32_t val)
{
    return (float)val / 1000.0f;
}
