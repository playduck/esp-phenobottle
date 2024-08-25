#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"

#define MAGIC_BYTE 0xf2

typedef enum
{
    CAT_PORT_0 = 0,
    CAT_PORT_1 = 1
} cat_port_t;

typedef enum
{
    CAT_PIN_0 = 0,
    CAT_PIN_1 = 1,
    CAT_PIN_2 = 2,
    CAT_PIN_3 = 3,
    CAT_PIN_4 = 4,
    CAT_PIN_5 = 5,
    CAT_PIN_6 = 6,
    CAT_PIN_7 = 7
} cat_pin_t;

typedef enum
{
    CAT_CMD_INPUT_0 = 0x00,
    CAT_CMD_INPUT_1,
    CAT_CMD_OUTPUT_0,
    CAT_CMD_OUTPUT_1,
    CAT_CMD_POLARITY_0,
    CAT_CMD_POLARITY_1,
    CAT_CMD_CONFIG_0,
    CAT_CMD_CONFIG_1,
} cat_command_t;

typedef enum
{
    CAT_DIR_INPUT,
    CAT_DIR_output
} cat_direction_t;

typedef enum
{
    CAT_LEVEL_LOW = 0,
    CAT_LEVEL_HIGH = 1
} cat_level_t;

typedef enum
{
    CAT_POLARITY_NORMAL = 0,
    CAT_POLARITY_INVERT = 1
} cat_polarity_t;

typedef struct
{
    uint8_t dev_address;
    SemaphoreHandle_t mutex;
    i2c_master_dev_handle_t i2c_dev;
    cat_level_t outputs[16];
    cat_polarity_t polarities[16];
    cat_direction_t directions[16];
} cat_state_t;

esp_err_t initlizeCat(cat_state_t *dev, uint8_t address, i2c_port_num_t i2c_port);
esp_err_t setDirection(cat_state_t *dev, cat_port_t port, cat_pin_t pin, cat_direction_t dir);
esp_err_t setPolarity(cat_state_t *dev, cat_port_t port, cat_pin_t pin, cat_polarity_t pol);
esp_err_t setLevel(cat_state_t *dev, cat_port_t port, cat_pin_t pin, cat_level_t level);
cat_level_t getLevel(cat_state_t *dev, cat_port_t port, cat_pin_t pin);
