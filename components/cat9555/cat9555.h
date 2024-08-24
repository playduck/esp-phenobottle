#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"

#define MAGIC_BYTE 0xf2

typedef enum
{
    port0 = 0,
    port1 = 1
} cat_port_t;

typedef enum
{
    pin0 = 0,
    pin1 = 1,
    pin2 = 2,
    pin3 = 3,
    pin4 = 4,
    pin5 = 5,
    pin6 = 6,
    pin7 = 7
} cat_pin_t;

typedef enum
{
    input0 = 0x00,
    input1,
    output0,
    output1,
    polarity0,
    polarity1,
    config0,
    config1,
} cat_command_t;

typedef enum
{
    input,
    output
} cat_direction_t;

typedef enum
{
    low = 0,
    high = 1
} cat_level_t;

typedef enum
{
    normal = 0,
    invert = 1
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
