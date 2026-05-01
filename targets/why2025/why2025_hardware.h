// SPDX-License-Identifier: MIT

// Private include file, not intended to be included by end users

#pragma once

// Internal I2C bus
#define BSP_I2C_BUS     0
#define BSP_I2C_SDA_PIN 18
#define BSP_I2C_SCL_PIN 20

// IP5306 power management IC
#define BSP_IP5306_I2C_ADDRESS 0x75

// MIPI DSI display
#define BSP_DSI_LDO_CHAN       3
#define BSP_DSI_LDO_VOLTAGE_MV 2500
#define BSP_LCD_RESET_PIN      17  // Note: low for normal operation, high for reset

// TCA8418 keyboard scanner
#define BSP_TCA8418_I2C_ADDRESS 0x34
#define BSP_TCA8418_INT_PIN     GPIO_NUM_NC  // Interrupt pin not connected

// SD card slot
#define BSP_SDCARD_CLK   -1
#define BSP_SDCARD_CMD   -1
#define BSP_SDCARD_D0    -1
#define BSP_SDCARD_D1    -1
#define BSP_SDCARD_D2    -1
#define BSP_SDCARD_D3    -1
#define BSP_SDCARD_D4    -1
#define BSP_SDCARD_D5    -1
#define BSP_SDCARD_D6    -1
#define BSP_SDCARD_D7    -1
#define BSP_SDCARD_CD    -1
#define BSP_SDCARD_WP    -1
#define BSP_SDCARD_WIDTH 4
#define BSP_SDCARD_FLAGS 0
