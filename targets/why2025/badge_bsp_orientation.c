#include "bsp/orientation.h"
#include "esp_err.h"

esp_err_t bsp_orientation_initialize(void) {
    return ESP_OK;
}

esp_err_t bsp_orientation_enable_gyroscope(void) {
    return ESP_OK;
}

esp_err_t bsp_orientation_disable_gyroscope(void) {
    return ESP_OK;
}

esp_err_t bsp_orientation_enable_accelerometer(void) {
    return ESP_OK;
}

esp_err_t bsp_orientation_disable_accelerometer(void) {
    return ESP_OK;
}

esp_err_t bsp_orientation_get(bool* out_gyro_ready, bool* out_accel_ready, float* out_gyro_x, float* out_gyro_y,
                              float* out_gyro_z, float* out_accel_x, float* out_accel_y, float* out_accel_z) {
    if (out_gyro_ready) *out_gyro_ready = false;
    if (out_accel_ready) *out_accel_ready = false;
    if (out_gyro_x) *out_gyro_x = 0.0f;
    if (out_gyro_y) *out_gyro_y = 0.0f;
    if (out_gyro_z) *out_gyro_z = 0.0f;
    if (out_accel_x) *out_accel_x = 0.0f;
    if (out_accel_y) *out_accel_y = 0.0f;
    if (out_accel_z) *out_accel_z = 0.0f;
    return ESP_OK;
}
