// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "badge_bsp_input_hooks.h"
#include "bsp/i2c.h"
#include "bsp/input.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "why2025_hardware.h"

static char const* TAG = "BSP INPUT";

// TCA8418 register addresses
#define TCA8418_REG_INTERRUPT_STATUS   0x02
#define TCA8418_REG_KEY_LOCK_EVT_COUNT 0x03
#define TCA8418_REG_KEY_EVENT_A        0x04
#define TCA8418_REG_GPIO_INT_EN1       0x1A
#define TCA8418_REG_GPIO_INT_EN2       0x1B
#define TCA8418_REG_GPIO_INT_EN3       0x1C
#define TCA8418_REG_GPI_EM1            0x20
#define TCA8418_REG_GPI_EM2            0x21
#define TCA8418_REG_GPI_EM3            0x22
#define TCA8418_REG_GPIO_DIRECTION_1   0x23
#define TCA8418_REG_GPIO_DIRECTION_2   0x24
#define TCA8418_REG_GPIO_DIRECTION_3   0x25
#define TCA8418_REG_GPIO_INT_STAT1     0x11
#define TCA8418_REG_GPIO_INT_STAT2     0x12
#define TCA8418_REG_GPIO_INT_STAT3     0x13
#define TCA8418_REG_DEBOUNCE_DIS1      0x29
#define TCA8418_REG_DEBOUNCE_DIS2      0x2A
#define TCA8418_REG_DEBOUNCE_DIS3      0x2B

// WHY2025 keymap: TCA8418 scancodes 0x01-0x50 -> BSP scancodes
// Index 0 corresponds to TCA8418 scancode 0x01
static bsp_input_scancode_t const tca8418_keymap[80] = {
    BSP_INPUT_SCANCODE_ESC,        // 0x01
    BSP_INPUT_SCANCODE_NONE,       // 0x02 (SQUARE - WHY2025 specific key, no PC equivalent)
    BSP_INPUT_SCANCODE_NONE,       // 0x03 (TRIANGLE - WHY2025 specific key)
    BSP_INPUT_SCANCODE_NONE,       // 0x04 (CROSS - WHY2025 specific key)
    BSP_INPUT_SCANCODE_NONE,       // 0x05 (CIRCLE - WHY2025 specific key)
    BSP_INPUT_SCANCODE_NONE,       // 0x06 (CLOUD - WHY2025 specific key)
    BSP_INPUT_SCANCODE_NONE,       // 0x07 (DIAMOND - WHY2025 specific key)
    BSP_INPUT_SCANCODE_BACKSPACE,  // 0x08
    BSP_INPUT_SCANCODE_0,          // 0x09
    BSP_INPUT_SCANCODE_MINUS,      // 0x0a
    BSP_INPUT_SCANCODE_GRAVE,      // 0x0b
    BSP_INPUT_SCANCODE_1,          // 0x0c
    BSP_INPUT_SCANCODE_2,          // 0x0d
    BSP_INPUT_SCANCODE_3,          // 0x0e
    BSP_INPUT_SCANCODE_4,          // 0x0f

    BSP_INPUT_SCANCODE_5,    // 0x10
    BSP_INPUT_SCANCODE_6,    // 0x11
    BSP_INPUT_SCANCODE_7,    // 0x12
    BSP_INPUT_SCANCODE_8,    // 0x13
    BSP_INPUT_SCANCODE_9,    // 0x14
    BSP_INPUT_SCANCODE_TAB,  // 0x15
    BSP_INPUT_SCANCODE_Q,    // 0x16
    BSP_INPUT_SCANCODE_W,    // 0x17
    BSP_INPUT_SCANCODE_E,    // 0x18
    BSP_INPUT_SCANCODE_R,    // 0x19
    BSP_INPUT_SCANCODE_T,    // 0x1a
    BSP_INPUT_SCANCODE_Y,    // 0x1b
    BSP_INPUT_SCANCODE_U,    // 0x1c
    BSP_INPUT_SCANCODE_I,    // 0x1d
    BSP_INPUT_SCANCODE_O,    // 0x1e
    BSP_INPUT_SCANCODE_FN,   // 0x1f

    BSP_INPUT_SCANCODE_A,          // 0x20
    BSP_INPUT_SCANCODE_S,          // 0x21
    BSP_INPUT_SCANCODE_D,          // 0x22
    BSP_INPUT_SCANCODE_F,          // 0x23
    BSP_INPUT_SCANCODE_G,          // 0x24
    BSP_INPUT_SCANCODE_H,          // 0x25
    BSP_INPUT_SCANCODE_J,          // 0x26
    BSP_INPUT_SCANCODE_K,          // 0x27
    BSP_INPUT_SCANCODE_L,          // 0x28
    BSP_INPUT_SCANCODE_LEFTSHIFT,  // 0x29
    BSP_INPUT_SCANCODE_Z,          // 0x2a
    BSP_INPUT_SCANCODE_X,          // 0x2b
    BSP_INPUT_SCANCODE_C,          // 0x2c
    BSP_INPUT_SCANCODE_V,          // 0x2d
    BSP_INPUT_SCANCODE_B,          // 0x2e
    BSP_INPUT_SCANCODE_N,          // 0x2f

    BSP_INPUT_SCANCODE_M,                   // 0x30
    BSP_INPUT_SCANCODE_COMMA,               // 0x31
    BSP_INPUT_SCANCODE_DOT,                 // 0x32
    BSP_INPUT_SCANCODE_ESCAPED_GREY_LEFT,   // 0x33
    BSP_INPUT_SCANCODE_ESCAPED_GREY_DOWN,   // 0x34
    BSP_INPUT_SCANCODE_ESCAPED_GREY_RIGHT,  // 0x35
    BSP_INPUT_SCANCODE_SLASH,               // 0x36
    BSP_INPUT_SCANCODE_ESCAPED_GREY_UP,     // 0x37
    BSP_INPUT_SCANCODE_RIGHTSHIFT,          // 0x38
    BSP_INPUT_SCANCODE_SEMICOLON,           // 0x39
    BSP_INPUT_SCANCODE_APOSTROPHE,          // 0x3a
    BSP_INPUT_SCANCODE_ENTER,               // 0x3b
    BSP_INPUT_SCANCODE_EQUAL,               // 0x3c
    BSP_INPUT_SCANCODE_LEFTCTRL,            // 0x3d
    BSP_INPUT_SCANCODE_ESCAPED_LEFTMETA,    // 0x3e (LGUI)
    BSP_INPUT_SCANCODE_LEFTALT,             // 0x3f

    BSP_INPUT_SCANCODE_BACKSLASH,     // 0x40
    BSP_INPUT_SCANCODE_SPACE,         // 0x41 (space bar left section)
    BSP_INPUT_SCANCODE_SPACE,         // 0x42 (space bar middle section)
    BSP_INPUT_SCANCODE_SPACE,         // 0x43 (space bar right section)
    BSP_INPUT_SCANCODE_ESCAPED_RALT,  // 0x44
    BSP_INPUT_SCANCODE_P,             // 0x45
    BSP_INPUT_SCANCODE_LEFTBRACE,     // 0x46
    BSP_INPUT_SCANCODE_NONE,          // 0x47
    BSP_INPUT_SCANCODE_NONE,          // 0x48
    BSP_INPUT_SCANCODE_NONE,          // 0x49
    BSP_INPUT_SCANCODE_NONE,          // 0x4a
    BSP_INPUT_SCANCODE_NONE,          // 0x4b
    BSP_INPUT_SCANCODE_NONE,          // 0x4c
    BSP_INPUT_SCANCODE_NONE,          // 0x4d
    BSP_INPUT_SCANCODE_NONE,          // 0x4e
    BSP_INPUT_SCANCODE_NONE,          // 0x4f

    BSP_INPUT_SCANCODE_RIGHTBRACE,  // 0x50
};

static QueueHandle_t           event_queue = NULL;
static i2c_master_dev_handle_t tca8418_dev = NULL;
static uint32_t                modifiers   = 0;

// Inject an input event into the queue (bypasses hooks)
esp_err_t bsp_input_inject_event(bsp_input_event_t* event) {
    if (event == NULL || event_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xQueueSend(event_queue, event, pdMS_TO_TICKS(10)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t tca8418_write_register(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(tca8418_dev, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t tca8418_read_register(uint8_t reg, uint8_t* out_value) {
    return i2c_master_transmit_receive(tca8418_dev, &reg, 1, out_value, 1, pdMS_TO_TICKS(100));
}

static void send_navigation_event(bsp_input_navigation_key_t key, bool state) {
    bsp_input_event_t event = {
        .type                      = INPUT_EVENT_TYPE_NAVIGATION,
        .args_navigation.key       = key,
        .args_navigation.modifiers = modifiers,
        .args_navigation.state     = state,
    };
    if (!bsp_input_hooks_process(&event)) {
        xQueueSend(event_queue, &event, 0);
    }
}

static void send_scancode_event(bsp_input_scancode_t scancode, bool pressed) {
    bsp_input_event_t event = {
        .type                   = INPUT_EVENT_TYPE_SCANCODE,
        .args_scancode.scancode = scancode | (pressed ? 0 : BSP_INPUT_SCANCODE_RELEASE_MODIFIER),
    };
    if (!bsp_input_hooks_process(&event)) {
        xQueueSend(event_queue, &event, 0);
    }
}

static void tca8418_poll_task(void* pvParameters) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10));

        uint8_t evt_count = 0;
        if (tca8418_read_register(TCA8418_REG_KEY_LOCK_EVT_COUNT, &evt_count) != ESP_OK) {
            continue;
        }
        evt_count &= 0x0F;

        for (uint8_t i = 0; i < evt_count; i++) {
            uint8_t raw = 0;
            if (tca8418_read_register(TCA8418_REG_KEY_EVENT_A, &raw) != ESP_OK) {
                break;
            }

            bool    pressed = (raw >> 7) & 0x01;
            uint8_t keycode = raw & 0x7F;

            if (keycode == 0 || keycode > 0x50) {
                ESP_LOGD(TAG, "Scancode out of range: 0x%02x, skipping", keycode);
                continue;
            }

            bsp_input_scancode_t scancode = tca8418_keymap[keycode - 1];

            // Update modifier bitmask
            switch (scancode) {
                case BSP_INPUT_SCANCODE_LEFTSHIFT:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_SHIFT_L;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_SHIFT_L;
                    break;
                case BSP_INPUT_SCANCODE_RIGHTSHIFT:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_SHIFT_R;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_SHIFT_R;
                    break;
                case BSP_INPUT_SCANCODE_LEFTCTRL:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_CTRL_L;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_CTRL_L;
                    break;
                case BSP_INPUT_SCANCODE_LEFTALT:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_ALT_L;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_ALT_L;
                    break;
                case BSP_INPUT_SCANCODE_ESCAPED_RALT:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_ALT_R;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_ALT_R;
                    break;
                case BSP_INPUT_SCANCODE_FN:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_FUNCTION;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_FUNCTION;
                    break;
                case BSP_INPUT_SCANCODE_ESCAPED_LEFTMETA:
                    if (pressed)
                        modifiers |= BSP_INPUT_MODIFIER_SUPER_L;
                    else
                        modifiers &= ~BSP_INPUT_MODIFIER_SUPER_L;
                    break;
                default:
                    break;
            }

            if (scancode == BSP_INPUT_SCANCODE_NONE) {
                continue;
            }

            // Always emit the scancode event
            send_scancode_event(scancode, pressed);

            // Also emit navigation event for navigation keys
            switch (scancode) {
                case BSP_INPUT_SCANCODE_ESC:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_ESC, pressed);
                    break;
                case BSP_INPUT_SCANCODE_ENTER:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_RETURN, pressed);
                    break;
                case BSP_INPUT_SCANCODE_BACKSPACE:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_BACKSPACE, pressed);
                    break;
                case BSP_INPUT_SCANCODE_TAB:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_TAB, pressed);
                    break;
                case BSP_INPUT_SCANCODE_SPACE:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_SPACE_M, pressed);
                    break;
                case BSP_INPUT_SCANCODE_ESCAPED_GREY_UP:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_UP, pressed);
                    break;
                case BSP_INPUT_SCANCODE_ESCAPED_GREY_DOWN:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_DOWN, pressed);
                    break;
                case BSP_INPUT_SCANCODE_ESCAPED_GREY_LEFT:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_LEFT, pressed);
                    break;
                case BSP_INPUT_SCANCODE_ESCAPED_GREY_RIGHT:
                    send_navigation_event(BSP_INPUT_NAVIGATION_KEY_RIGHT, pressed);
                    break;
                default:
                    break;
            }
        }
    }
}

esp_err_t bsp_input_initialize(void) {
    if (event_queue == NULL) {
        event_queue = xQueueCreate(32, sizeof(bsp_input_event_t));
        ESP_RETURN_ON_FALSE(event_queue, ESP_ERR_NO_MEM, TAG, "Failed to create input event queue");
    }

    bsp_input_hooks_init();

    i2c_master_bus_handle_t i2c_bus_handle = NULL;
    ESP_RETURN_ON_ERROR(bsp_i2c_primary_bus_get_handle(&i2c_bus_handle), TAG, "Failed to get I2C bus handle");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BSP_TCA8418_I2C_ADDRESS,
        .scl_speed_hz    = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &tca8418_dev), TAG,
                        "Failed to add TCA8418 to I2C bus");

    // Set all GPIO pins to input
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPIO_DIRECTION_1, 0x00), TAG,
                        "Failed to configure GPIO direction 1");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPIO_DIRECTION_2, 0x00), TAG,
                        "Failed to configure GPIO direction 2");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPIO_DIRECTION_3, 0x00), TAG,
                        "Failed to configure GPIO direction 3");

    // Add all pins to key events (GPI event mode)
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPI_EM1, 0xFF), TAG, "Failed to configure GPI EM1");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPI_EM2, 0xFF), TAG, "Failed to configure GPI EM2");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPI_EM3, 0xFF), TAG, "Failed to configure GPI EM3");

    // Enable all pin interrupts
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPIO_INT_EN1, 0xFF), TAG,
                        "Failed to configure GPIO INT EN1");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPIO_INT_EN2, 0xFF), TAG,
                        "Failed to configure GPIO INT EN2");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_GPIO_INT_EN3, 0xFF), TAG,
                        "Failed to configure GPIO INT EN3");

    // Enable debounce on all pins
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_DEBOUNCE_DIS1, 0x00), TAG, "Failed to configure debounce 1");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_DEBOUNCE_DIS2, 0x00), TAG, "Failed to configure debounce 2");
    ESP_RETURN_ON_ERROR(tca8418_write_register(TCA8418_REG_DEBOUNCE_DIS3, 0x00), TAG, "Failed to configure debounce 3");

    // Flush any pending events from the FIFO (up to 16 events max per TCA8418 FIFO depth)
    uint8_t discard = 0;
    for (int flush_attempts = 0; flush_attempts < 16; flush_attempts++) {
        uint8_t key = 0;
        if (tca8418_read_register(TCA8418_REG_KEY_EVENT_A, &key) != ESP_OK || key == 0) {
            break;
        }
        discard++;
    }
    // Clear interrupt status registers
    uint8_t dummy = 0;
    tca8418_read_register(TCA8418_REG_GPIO_INT_STAT1, &dummy);
    tca8418_read_register(TCA8418_REG_GPIO_INT_STAT2, &dummy);
    tca8418_read_register(TCA8418_REG_GPIO_INT_STAT3, &dummy);
    tca8418_write_register(TCA8418_REG_INTERRUPT_STATUS, 0x03);

    ESP_LOGI(TAG, "TCA8418 initialized (flushed %u pending events)", discard);

    // Start polling task
    BaseType_t ret = xTaskCreate(tca8418_poll_task, "tca8418_poll", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    ESP_RETURN_ON_FALSE(ret == pdPASS, ESP_ERR_NO_MEM, TAG, "Failed to create TCA8418 polling task");

    return ESP_OK;
}

esp_err_t bsp_input_get_queue(QueueHandle_t* out_queue) {
    if (out_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (event_queue == NULL) {
        return ESP_FAIL;
    }
    *out_queue = event_queue;
    return ESP_OK;
}

bool bsp_input_needs_on_screen_keyboard(void) {
    return false;
}

esp_err_t bsp_input_read_navigation_key(bsp_input_navigation_key_t key, bool* out_state) {
    *out_state = false;
    return ESP_OK;
}

esp_err_t bsp_input_read_action(bsp_input_action_type_t action, bool* out_state) {
    *out_state = false;
    return ESP_OK;
}
