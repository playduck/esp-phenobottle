#pragma once
#include "esp_err.h"

esp_err_t gas_init();
esp_err_t gas_start();
esp_err_t gas_update();
esp_err_t gas_publish();
esp_err_t gas_end();
