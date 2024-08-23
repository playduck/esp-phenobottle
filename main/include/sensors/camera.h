#pragma once
#include "esp_err.h"

esp_err_t camera_init();
esp_err_t camera_start();
esp_err_t camera_update();
esp_err_t camera_publish();
esp_err_t camera_end();
