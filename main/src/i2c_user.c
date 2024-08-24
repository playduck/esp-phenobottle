#include "i2c_user.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char* TAG = "I2C";

esp_err_t i2c_init()    {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = SOC_MOD_CLK_XTAL,
        .i2c_port = I2C_USER_PORT,
        .scl_io_num = I2C_USER_MASTER_SCL_IO,
        .sda_io_num = I2C_USER_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = (i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    ESP_LOGI(TAG, "Installed I2C Bus");

    return ret;
}
