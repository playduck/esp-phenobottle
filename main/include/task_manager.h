#pragma once
#include "esp_err.h"
#include "esp_log.h"

esp_err_t task_manager_init();
esp_err_t task_manager_start();
esp_err_t task_manager_update();
esp_err_t task_manager_publish();
esp_err_t task_manager_end();
