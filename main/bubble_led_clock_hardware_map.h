// bubble_led_clock_hardware_map.h — GPIO and I2C pin assignments for BubbleLEDClock2.
//
// The bubble LED clock uses an HT16K33-driven 8-digit 7-segment display and
// a DS1307 RTC, both sharing the same I2C bus.

#pragma once

#include "driver/i2c.h"

// --- I2C bus (HT16K33 display + DS1307 RTC) ----------------------------------
constexpr i2c_port_t  BLC_I2C_PORT         = I2C_NUM_0;
constexpr int         BLC_I2C_SDA          = 21;
constexpr int         BLC_I2C_SCL          = 22;

// --- HT16K33 8-digit 7-segment display ---------------------------------------
constexpr uint8_t     HT16K33_I2C_ADDR     = 0x70;
constexpr int         HT16K33_DISPLAY_SIZE = 8;

// --- DS1307 RTC --------------------------------------------------------------
constexpr uint8_t     RTC_I2C_ADDR         = 0x68;

// --- AP trigger --------------------------------------------------------------
// Double-reset via BootManager is used instead of a dedicated button.
constexpr int         AP_TRIGGER_GPIO      = -1;

// --- Built-in LED ------------------------------------------------------------
// GPIO 2 = blue LED on ESP32-WROOM dev board. Flashes on WiFi activity.
// Set to -1 to disable.
constexpr int         LED_GPIO             = 2;
