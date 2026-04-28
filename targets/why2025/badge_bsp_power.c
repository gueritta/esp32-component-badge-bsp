// Board support package API: WHY2025 implementation
// SPDX-FileCopyrightText: 2026 Nicolai Electronics
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>
#include "bsp/i2c.h"
#include "bsp/power.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "why2025_hardware.h"

static char const* TAG = "BSP: power";

static i2c_master_dev_handle_t ip5306_handle = NULL;

#define IP5306_REG_SYS_CTL0            0x00
#define IP5306_REG_READ0               0x70
#define IP5306_REG_READ1               0x71
#define IP5306_REG_READ4               0x78
#define IP5306_SYS_CTL0_BOOT_CONFIG    0x37
#define IP5306_MAX_CHARGING_CURRENT_MA 2100

static esp_err_t ip5306_read_reg(uint8_t reg, uint8_t* out_val) {
    ESP_RETURN_ON_FALSE(ip5306_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "IP5306 handle not initialized");
    ESP_RETURN_ON_FALSE(out_val != NULL, ESP_ERR_INVALID_ARG, TAG, "Output argument is NULL");
    return i2c_master_transmit_receive(ip5306_handle, &reg, 1, out_val, 1, -1);
}

static esp_err_t ip5306_write_reg(uint8_t reg, uint8_t val) {
    ESP_RETURN_ON_FALSE(ip5306_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "IP5306 handle not initialized");
    uint8_t buffer[2] = {reg, val};
    return i2c_master_transmit(ip5306_handle, buffer, sizeof(buffer), -1);
}

esp_err_t bsp_power_initialize(void) {
    if (ip5306_handle != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_handle_t i2c_bus_handle_internal = NULL;
    ESP_RETURN_ON_ERROR(bsp_i2c_primary_bus_get_handle(&i2c_bus_handle_internal), TAG, "Failed to get I2C bus handle");

    i2c_device_config_t ip5306_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BSP_IP5306_I2C_ADDRESS,
        .scl_speed_hz    = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus_handle_internal, &ip5306_config, &ip5306_handle), TAG,
                        "Failed to add IP5306 I2C device");

    // button shutdown enable | boost output normally open | auto power-on | charger enable | boost enable
    ESP_RETURN_ON_ERROR(ip5306_write_reg(IP5306_REG_SYS_CTL0, IP5306_SYS_CTL0_BOOT_CONFIG), TAG,
                        "Failed to configure IP5306");
    return ESP_OK;
}

esp_err_t bsp_power_get_battery_information(bsp_power_battery_information_t* out_information) {
    ESP_RETURN_ON_FALSE(out_information, ESP_ERR_INVALID_ARG, TAG, "Information output argument is NULL");

    uint8_t reg_read0 = 0;
    uint8_t reg_read1 = 0;
    uint8_t reg_read4 = 0;
    ESP_RETURN_ON_ERROR(ip5306_read_reg(IP5306_REG_READ0, &reg_read0), TAG, "Failed to read IP5306 READ0");
    ESP_RETURN_ON_ERROR(ip5306_read_reg(IP5306_REG_READ1, &reg_read1), TAG, "Failed to read IP5306 READ1");
    ESP_RETURN_ON_ERROR(ip5306_read_reg(IP5306_REG_READ4, &reg_read4), TAG, "Failed to read IP5306 READ4");

    bool charging    = (reg_read0 & (1 << 3)) != 0;
    bool charge_full = (reg_read1 & (1 << 3)) != 0;

    double remaining_percentage = 0.0;
    switch (reg_read4 & 0xF0) {
        case 0x00:
            remaining_percentage = 100.0;
            break;
        case 0x80:
            remaining_percentage = 75.0;
            break;
        case 0xC0:
            remaining_percentage = 50.0;
            break;
        case 0xE0:
            remaining_percentage = 25.0;
            break;
        default:
            remaining_percentage = 0.0;
            break;
    }

    out_information->type                     = "LiPo";
    out_information->power_supply_available   = charging || charge_full;
    out_information->battery_available        = true;
    out_information->charging_disabled        = false;
    out_information->battery_charging         = charging && !charge_full;
    out_information->maximum_charging_current = IP5306_MAX_CHARGING_CURRENT_MA;
    out_information->current_charging_current = 0;  // Not readable from IP5306
    out_information->voltage                  = 0;  // Not readable from IP5306
    out_information->charging_target_voltage  = 4200;
    out_information->remaining_percentage     = remaining_percentage;
    return ESP_OK;
}

esp_err_t bsp_power_get_battery_voltage(uint16_t* out_millivolt) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_get_system_voltage(uint16_t* out_millivolt) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_get_input_voltage(uint16_t* out_millivolt) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_get_charging_configuration(bool* out_disabled, uint16_t* out_current) {
    ESP_RETURN_ON_FALSE(out_disabled, ESP_ERR_INVALID_ARG, TAG, "Disabled output argument is NULL");
    ESP_RETURN_ON_FALSE(out_current, ESP_ERR_INVALID_ARG, TAG, "Current output argument is NULL");
    *out_disabled = false;
    *out_current  = 0;
    return ESP_OK;
}

esp_err_t bsp_power_configure_charging(bool disable, uint16_t current) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_get_usb_host_boost_enabled(bool* out_enabled) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_set_usb_host_boost_enabled(bool enable) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_get_radio_state(bsp_radio_state_t* out_state) {
    ESP_RETURN_ON_FALSE(out_state, ESP_ERR_INVALID_ARG, TAG, "State output argument is NULL");
    *out_state = BSP_POWER_RADIO_STATE_APPLICATION;
    return ESP_OK;
}

esp_err_t bsp_power_set_radio_state(bsp_radio_state_t state) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_power_off(bool enable_alarm_wakeup) {
    return ESP_ERR_NOT_SUPPORTED;
}
