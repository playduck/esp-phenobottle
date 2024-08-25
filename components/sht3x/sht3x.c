#include "esp_log.h"
#include "esp_private/i2c_platform.h"
#include "driver/i2c_master.h"

#include "sht3x.h"
#include "i2c_user.h"

static const char* TAG = "SHT3x";

uint8_t crc8_le(uint8_t crc, uint8_t* buf, size_t length) {
    for (size_t i = 0; i < length; i++) {
        crc = crc ^ buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

uint8_t crc8(uint8_t init, uint8_t* buf, size_t length) {
    uint8_t crc = ~init;
    for (size_t i = 0; i < length; i++) {
        crc = crc8_le(crc, &buf[i], 1);
    }
    return ~crc;
}

float temperature_to_float(uint16_t temperature_raw)    {
    return -45.0 + 175.0 * ((float)temperature_raw / (float)UINT16_MAX);
}

float humidity_to_float(uint16_t humidity_raw)  {
    return 100.0 * ((float)humidity_raw / (float)UINT16_MAX);
}

esp_err_t sht3x_init(sht3x_device_t* dev, sht3x_address_t address, i2c_port_num_t i2c_port) {
    dev->initilized = false;

    // FIXME remove this
    uint8_t test_data[] = {
        0xBE, 0xEF
    };
    uint8_t calc_crc= crc8(0xFF, test_data + 3, 2);
    ESP_LOGI(TAG, "CRC8 Test: %02x", calc_crc);


    if(address != address_A && address != address_B )   {
        ESP_LOGE(TAG, "Invalid address %02x", address);
        return ESP_FAIL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t err = i2c_master_get_bus_handle(i2c_port, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get I2C Bus handle");
        return err;
    }

    err = i2c_master_probe(bus_handle, address, I2C_USER_TIMEOUT_MS);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot find device at address %02x", address);
        return err;
    }

    err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &(dev->i2c_dev));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register I2C device");
        return err;
    }

    ESP_LOGI(TAG, "Initilized device");
    dev->initilized = true;
    return ESP_OK;
}

esp_err_t sht3x_start_measurement(sht3x_device_t* dev, sht3x_measurement_command_t measurement_mode)    {
    if(!dev->initilized)    {
        return ESP_FAIL;
    }

    union {
        uint16_t value;
        uint8_t bytes[2];
    } tx_buffer;

    tx_buffer.value = measurement_mode;

    esp_err_t ret = i2c_master_transmit(dev->i2c_dev, tx_buffer.bytes, 2, I2C_USER_TIMEOUT_MS);

    return ret;
}

esp_err_t sht3x_read_measurement(sht3x_device_t* dev, sht3x_measurement_t* measurement) {
    if(!dev->initilized)    {
        return ESP_FAIL;
    }

    uint8_t rx_buffer[6];

    esp_err_t ret = i2c_master_receive(dev->i2c_dev, rx_buffer, 6, I2C_USER_TIMEOUT_MS);

    if(ret != ESP_OK)   {
        ESP_LOGE(TAG, "Failed to get measurement");
        return ret;
    }

    uint8_t temp_crc = crc8(0xFF, rx_buffer + 0, 2);
    uint8_t humid_crc = crc8(0xFF, rx_buffer + 3, 2);

    if(temp_crc != rx_buffer[2])    {
        ESP_LOGE(TAG, "Temperature CRC mismatch! Got %02x, Expected %02x", temp_crc, rx_buffer[2]);
        return ESP_FAIL;
    }
    if(humid_crc != rx_buffer[5])    {
        ESP_LOGE(TAG, "Humidity CRC mismatch! Got %02x, Expected %02x", humid_crc, rx_buffer[5]);
        return ESP_FAIL;
    }

    measurement->temperature_raw = (rx_buffer[0] << 8) | rx_buffer[1];
    measurement->humidity_raw = (rx_buffer[3] << 8) | rx_buffer[4];

    measurement->temperature_degC = temperature_to_float(measurement->temperature_raw);
    measurement->relative_humidity = humidity_to_float(measurement->humidity_raw);

    return ESP_OK;
}

esp_err_t sht3x_read_measurements(sht3x_device_t* dev, sht3x_measurement_t measurements[], uint8_t count)   {
    return  ESP_OK;
}

esp_err_t sht3x_read_status(sht3x_device_t* dev, sht3x_status_t* status)    {
    return  ESP_OK;
}

esp_err_t sht3x_send_command(sht3x_device_t* dev, sht3x_command_t command)  {
    return  ESP_OK;
}
