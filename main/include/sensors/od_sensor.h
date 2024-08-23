#pragma once
#include "esp_err.h"

esp_err_t od_init();
esp_err_t od_start();
esp_err_t od_update();
esp_err_t od_publish();
esp_err_t od_end();
