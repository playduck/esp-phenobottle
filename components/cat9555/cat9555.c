#include "esp_log.h"
#include "esp_private/i2c_platform.h"
#include "driver/i2c_master.h"

#include "cat9555.h"
#include "i2c_user.h"

static const char *TAG = "CAT";

esp_err_t initlizeCat(cat_state_t *dev, uint8_t address, i2c_port_num_t i2c_port)
{
    if ((address <= 0b0100111) && (address >= 0b0100000))
    {
        dev->dev_address = address;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid address (%02x)", address);
        return ESP_FAIL;
    }

    dev->mutex = xSemaphoreCreateMutex();
    if (dev->mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create Mutex");
        return ESP_FAIL;
    }
    xSemaphoreTake(dev->mutex, portMAX_DELAY);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev->dev_address,
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

    // TODO do this more elegantly
    uint8_t tx_buffer[3];
    tx_buffer[0] = CAT_CMD_CONFIG_0;
    tx_buffer[1] = 0x00;
    tx_buffer[2] = 0x00;
    i2c_master_transmit(dev->i2c_dev, tx_buffer, 3, I2C_USER_TIMEOUT_MS);
    tx_buffer[0] = CAT_CMD_POLARITY_0;
    i2c_master_transmit(dev->i2c_dev, tx_buffer, 3, I2C_USER_TIMEOUT_MS);
    tx_buffer[0] = CAT_CMD_OUTPUT_0;
    i2c_master_transmit(dev->i2c_dev, tx_buffer, 3, I2C_USER_TIMEOUT_MS);

    ESP_LOGI(TAG, "Initilized device");

    xSemaphoreGive(dev->mutex);

    return ESP_OK;
}

esp_err_t setDirection(cat_state_t *dev, cat_port_t port, cat_pin_t pin, cat_direction_t dir)
{
    xSemaphoreTake(dev->mutex, portMAX_DELAY);

    dev->directions[port * 8 + pin] = dir;

    uint8_t tx_buffer[2];

    if (port == CAT_PORT_0)
    {
        tx_buffer[0] = CAT_CMD_CONFIG_0;
    }
    else
    {
        tx_buffer[0] = CAT_CMD_CONFIG_1;
    }

    tx_buffer[1] = 0x00;
    for (uint8_t lpin = 0; lpin <= 7; lpin++)
    {
        uint8_t bit = dev->directions[port * 8 + lpin] == CAT_DIR_INPUT ? 1 : 0;
        tx_buffer[1] |= bit << lpin;
    }

    ESP_LOGD(TAG, "Direction: %04x %04x", tx_buffer[0], tx_buffer[1]);

    esp_err_t ret = i2c_master_transmit(dev->i2c_dev, tx_buffer, 2, I2C_USER_TIMEOUT_MS);

    xSemaphoreGive(dev->mutex);

    return ret;
}

esp_err_t setPolarity(cat_state_t *dev, cat_port_t port, cat_pin_t pin, cat_polarity_t pol)
{
    xSemaphoreTake(dev->mutex, portMAX_DELAY);

    dev->polarities[port * 8 + pin] = pol;

    uint8_t tx_buffer[2];

    if (port == CAT_PORT_0)
    {
        tx_buffer[0] = CAT_CMD_POLARITY_0;
    }
    else
    {
        tx_buffer[0] = CAT_CMD_POLARITY_1;
    }

    tx_buffer[1] = 0x00;
    for (uint8_t lpin = 0; lpin <= 7; lpin++)
    {
        uint8_t bit = dev->polarities[port * 8 + lpin] == CAT_POLARITY_NORMAL ? 1 : 0;
        tx_buffer[1] |= bit << lpin;
    }

    ESP_LOGD(TAG, "Polarity: %04x %04x", tx_buffer[0], tx_buffer[1]);

    esp_err_t ret = i2c_master_transmit(dev->i2c_dev, tx_buffer, 2, I2C_USER_TIMEOUT_MS);

    xSemaphoreGive(dev->mutex);

    return ret;
}

esp_err_t setLevel(cat_state_t *dev, cat_port_t port, cat_pin_t pin, cat_level_t level)
{
    xSemaphoreTake(dev->mutex, portMAX_DELAY);

    dev->outputs[port * 8 + pin] = level;

    uint8_t tx_buffer[2];

    if (port == CAT_PORT_0)
    {
        tx_buffer[0] = CAT_CMD_OUTPUT_0;
    }
    else
    {
        tx_buffer[0] = CAT_CMD_OUTPUT_1;
    }

    tx_buffer[1] = 0x00;
    for (uint8_t lpin = 0; lpin <= 7; lpin++)
    {
        uint8_t bit = dev->outputs[port * 8 + lpin] == CAT_LEVEL_HIGH ? 1 : 0;
        tx_buffer[1] |= bit << lpin;
    }

    ESP_LOGD(TAG, "Outputs: %04x %04x", tx_buffer[0], tx_buffer[1]);

    esp_err_t ret = i2c_master_transmit(dev->i2c_dev, tx_buffer, 2, I2C_USER_TIMEOUT_MS);

    xSemaphoreGive(dev->mutex);

    return ret;
}

cat_level_t getLevel(cat_state_t *dev, cat_port_t port, cat_pin_t pin)
{
    xSemaphoreTake(dev->mutex, portMAX_DELAY);

    uint8_t tx_buffer[1];
    uint8_t rx_buffer[1];

    rx_buffer[0] = MAGIC_BYTE;

    if (port == CAT_PORT_0)
    {
        tx_buffer[0] = CAT_CMD_INPUT_0;
    }
    else
    {
        tx_buffer[0] = CAT_CMD_INPUT_1;
    }

    esp_err_t err = i2c_master_transmit_receive(dev->i2c_dev, tx_buffer, 1, rx_buffer, 1, I2C_USER_TIMEOUT_MS);

    if(err != ESP_OK)   {
        ESP_LOGE(TAG, "I2C Transaction failed");
    }

    if (rx_buffer[0] == MAGIC_BYTE)
    {
        ESP_LOGW(TAG, "Rx buffer was not changed!");
    }

    uint8_t bit = rx_buffer[0] & (1 << pin);

    xSemaphoreGive(dev->mutex);

    if (bit >= 1)
    {
        return CAT_LEVEL_HIGH;
    }
    else
    {
        return CAT_LEVEL_LOW;
    }
}
