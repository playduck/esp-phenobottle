#pragma once
#include "esp_err.h"

esp_err_t mixing_init();
esp_err_t mixing_start();
esp_err_t mixing_update();
esp_err_t mixing_publish();
esp_err_t mixing_end();
