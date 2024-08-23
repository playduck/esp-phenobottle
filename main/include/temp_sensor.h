#pragma once
#include "esp_err.h"

esp_err_t temp_init();
esp_err_t temp_start();
esp_err_t temp_update();
esp_err_t temp_publish();
esp_err_t temp_end();
