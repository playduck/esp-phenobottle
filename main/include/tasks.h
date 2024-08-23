#pragma once

#ifndef TASKS_H
#define TASKS_H

#include "interval_task.h"

#include "task_manager.h"
#include "sensors/camera.h"
#include "sensors/temp_sensor.h"

interval_task_interface_t task_manager_interface = {
    .name = "task_manager",
    .force_publish = 0,
    .disable_update = false,
    .disable_publish = true,
    .publish_interval = 10ULL * 1000ULL,
    .update_interval = 10ULL * 1000ULL,
    .task_interval = 1ULL * 1000ULL,
    .init = task_manager_init,
    .start = task_manager_start,
    .update = task_manager_update,
    .publish = task_manager_publish,
    .end = task_manager_end
};

interval_task_interface_t camera_task_interface = {
    .name = "camera_task",
    .force_publish = 0,
    .disable_update = true,
    .disable_publish = false,
    .publish_interval = 30ULL * 1000ULL,
    .update_interval = 10ULL * 1000ULL,
    .task_interval = 1ULL * 1000ULL,
    .init = camera_init,
    .start = camera_start,
    .update = camera_update,
    .publish = camera_publish,
    .end = camera_end
};

interval_task_interface_t temp_task_interface = {
    .name = "temp_task",
    .force_publish = 0,
    .disable_update = false,
    .disable_publish = false,
    .publish_interval = 10ULL * 1000ULL,
    .update_interval = 1ULL * 1000ULL,
    .task_interval = 500ULL,
    .init = temp_init,
    .start = temp_start,
    .update = temp_update,
    .publish = temp_publish,
    .end = temp_end
};

#endif
