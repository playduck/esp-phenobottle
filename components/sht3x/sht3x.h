#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

typedef enum {
    address_A = 0x44, // addr pin low
    address_B = 0x45 // addr pin high
} sht3x_address_t;

#define COMMAND_GENERAL_RESET 0x06

typedef enum {
    SINGLE_SHOT_CLOCK_STRETCHING_ENABLED_HIGH_RELIABILITY = 0x2C06,
    SINGLE_SHOT_CLOCK_STRETCHING_ENABLED_MEDIUM_RELIABILITY = 0x2C0D,
    SINGLE_SHOT_CLOCK_STRETCHING_ENABLED_LOW_RELIABILITY = 0x2C10,
    SINGLE_SHOT_CLOCK_STRETCHING_DISABLED_HIGH_RELIABILITY = 0x2400,
    SINGLE_SHOT_CLOCK_STRETCHING_DISABLED_MEDIUM_RELIABILITY = 0x240B,
    SINGLE_SHOT_CLOCK_STRETCHING_DISABLED_LOW_RELIABILITY = 0x2416,
    PERIODIC_POINT_FIVE_MPS_HIGH_RELIABILITY = 0x2032,
    PERIODIC_POINT_FIVE_MPS_MEDIUM_RELIABILITY = 0x2024,
    PERIODIC_POINT_FIVE_MPS_LOW_RELIABILITY = 0x202F,
    PERIODIC_ONE_MPS_HIGH_RELIABILITY = 0x2130,
    PERIODIC_ONE_MPS_MEDIUM_RELIABILITY = 0x2126,
    PERIODIC_ONE_MPS_LOW_RELIABILITY = 0x212D,
    PERIODIC_TWO_MPS_HIGH_RELIABILITY = 0x2236,
    PERIODIC_TWO_MPS_MEDIUM_RELIABILITY = 0x2220,
    PERIODIC_TWO_MPS_LOW_RELIABILITY = 0x222B,
    PERIODIC_FOUR_MPS_HIGH_RELIABILITY = 0x2334,
    PERIODIC_FOUR_MPS_MEDIUM_RELIABILITY = 0x2322,
    PERIODIC_FOUR_MPS_LOW_RELIABILITY = 0x2329,
    PERIODIC_TEN_MPS_HIGH_RELIABILITY = 0x2737,
    PERIODIC_TEN_MPS_MEDIUM_RELIABILITY = 0x2721,
    PERIODIC_TEN_MPS_LOW_RELIABILITY = 0x272A,
} sht3x_measurement_command_t;

typedef enum {
    COMMAND_FETCH = 0xE000,
    COMMAND_ART = 0x2B32,
    COMMAND_BREAK = 0x3093,
    COMMAND_SOFT_RESET = 0x30A2,
    COMMAND_HEATER_ENABLE = 0x306D,
    COMMAND_HEATER_DISABLE = 0x3066,
    COMMAND_READ_STATUS_REG = 0xF32D,
    COMMAND_CLEAR_STATUS_REG = 0x3041
} sht3x_command_t;

typedef union {
    uint16_t raw;
    struct {
        uint16_t write_data_checksum_status : 1;  // Bit 0 (LSB)
        uint16_t command_status : 1;
        uint16_t reserved2 : 1;
        uint16_t reserved3 : 1;
        uint16_t system_reset_detect : 1;
        uint16_t reserved5 : 1;
        uint16_t reserved6 : 1;
        uint16_t reserved7 : 1;
        uint16_t reserved8 : 1;
        uint16_t reserved9 : 1;
        uint16_t t_tracking_alert : 1;
        uint16_t rh_tracking_alert : 1;
        uint16_t reserved12 : 1;
        uint16_t heater_status : 1;
        uint16_t reserved14 : 1;
        uint16_t alert_pending : 1;  // Bit 15 (MSB)
    } bits;
} sht3x_status_t;

typedef struct {
    uint16_t temperature_raw;
    uint16_t humidity_raw;
    float relative_humidity;
    float temperature_degC;
} sht3x_measurement_t;

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
    bool initilized;
} sht3x_device_t;

esp_err_t sht3x_init(sht3x_device_t* dev, sht3x_address_t address, i2c_port_num_t i2c_port);

esp_err_t sht3x_start_measurement(sht3x_device_t* dev, sht3x_measurement_command_t measurement_mode);

esp_err_t sht3x_read_measurement(sht3x_device_t* dev, sht3x_measurement_t* measurement);
esp_err_t sht3x_read_measurements(sht3x_device_t* dev, sht3x_measurement_t measurements[], uint8_t count);

esp_err_t sht3x_read_status(sht3x_device_t* dev, sht3x_status_t* status);

esp_err_t sht3x_send_command(sht3x_device_t* dev, sht3x_command_t command);
