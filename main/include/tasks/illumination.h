#pragma once
#include "esp_err.h"

esp_err_t illumination_init();
esp_err_t illumination_start();
esp_err_t illumination_update();
esp_err_t illumination_publish();
esp_err_t illumination_end();
