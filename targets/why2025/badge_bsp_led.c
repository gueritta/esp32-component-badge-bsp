#include "bsp/led.h"
#include "esp_err.h"

esp_err_t bsp_led_initialize(void) {
    return ESP_OK;
}

esp_err_t bsp_led_write(const uint8_t* data, uint32_t length) {
    return ESP_OK;
}

esp_err_t bsp_led_set_brightness(uint8_t percentage) {
    return ESP_OK;
}

esp_err_t bsp_led_get_brightness(uint8_t* out_percentage) {
    if (out_percentage) *out_percentage = 100;
    return ESP_OK;
}

esp_err_t bsp_led_set_mode(bool automatic) {
    return ESP_OK;
}

esp_err_t bsp_led_get_mode(bool* out_automatic) {
    if (out_automatic) *out_automatic = false;
    return ESP_OK;
}

esp_err_t bsp_led_send(void) {
    return ESP_OK;
}

esp_err_t bsp_led_clear(void) {
    return ESP_OK;
}

esp_err_t bsp_led_set_pixel(uint32_t index, uint32_t color) {
    return ESP_OK;
}

esp_err_t bsp_led_set_pixel_rgb(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) {
    return ESP_OK;
}

esp_err_t bsp_led_set_pixel_rgbw(uint32_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    return ESP_OK;
}

esp_err_t bsp_led_set_pixel_hsv(uint32_t index, uint16_t hue, uint8_t saturation, uint8_t value) {
    return ESP_OK;
}

esp_err_t bsp_led_get_count(uint32_t* out_count) {
    if (out_count) *out_count = 0;
    return ESP_OK;
}
