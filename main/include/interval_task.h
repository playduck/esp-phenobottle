#pragma once

#include <stdint.h>

typedef struct {
    uint32_t update_interval;
    uint32_t publish_interval;
    uint8_t force_publish;
    bool disable_update; // if set, only publish interval will be used and update will be ignored
} interval_task_interface_t;

typedef struct {
    uint32_t last_update;
    uint32_t last_publish;
} interval_task_state_t;
