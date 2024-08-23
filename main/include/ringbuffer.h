#pragma once
#include <stdint.h>
#include "esp_err.h"

#define BUFFER_ELEMENTS 100
#define BUFFER_SIZE (BUFFER_ELEMENTS + 1)

typedef struct {
    int32_t list[BUFFER_SIZE];
    uint16_t in;
    int16_t out;
} ringbuffer_t;

esp_err_t buffer_write(ringbuffer_t* buffer, int32_t new);
esp_err_t buffer_read(ringbuffer_t* buffer, int32_t *old);

int32_t toFixed(float val);
float toFloat(int32_t val);
