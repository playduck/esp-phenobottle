#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <esp_err.h>

typedef struct {
    char* name;
    uint32_t update_interval;
    uint32_t publish_interval;
    uint32_t task_interval;
    uint8_t force_publish;
    bool disable_update; // if set, only publish interval will be used and update will be ignored
    esp_err_t(*init)(void);
    esp_err_t(*update)(void);
    esp_err_t(*publish)(void);
    esp_err_t(*start)(void);
    esp_err_t(*end)(void);
} interval_task_interface_t;

void task(void* pvparameters);
