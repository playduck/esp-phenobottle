#pragma once

#include "esp_err.h"

#define I2C_USER_PORT 0
#define I2C_USER_MASTER_SCL_IO 14
#define I2C_USER_MASTER_SDA_IO 15

#define I2C_USER_TIMEOUT_MS 500

esp_err_t i2c_init();
