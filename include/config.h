#pragma once
#include <Arduino.h>

// -------- Hardware: Waveshare ESP32-C5-WIFI6-KIT-N32R8-UM --------
// Avoid GPIO6 and GPIO15: occupied on the Waveshare board.
// Avoid GPIO13/14 so native USB remains untouched.
// Avoid GPIO27 (onboard RGB LED) and GPIO28 (BOOT).

static constexpr int PIN_SPI_SCK   = 10;
static constexpr int PIN_SPI_MOSI  = 8;
static constexpr int PIN_SPI_MISO  = 9;

static constexpr int PIN_TFT_CS    = 23;
static constexpr int PIN_TFT_DC    = 24;
static constexpr int PIN_TFT_RST   = 0;
static constexpr int PIN_TFT_BL    = 3;   // MSP4021 LED = active-HIGH backlight control input

static constexpr int PIN_TOUCH_CS  = 1;
static constexpr int PIN_TOUCH_IRQ = 4;

// MSP4021 has an onboard backlight transistor. LED is a logic/PWM control input,
// so it goes to GPIO3 rather than consuming the ESP board's only 3V3 header pin.
// LCD module VCC goes to 5V while the ESP32-C5 is USB-powered.

// 480 x 320 landscape.
static constexpr int SCREEN_W = 480;
static constexpr int SCREEN_H = 320;
static constexpr uint32_t TFT_SPI_HZ = 30000000;
static constexpr uint32_t TOUCH_SPI_HZ = 2000000;

// Normal screen blanking. Touch IRQ remains alive and wakes the display.
static constexpr uint32_t SCREEN_IDLE_MS = 300000;  // 5 min; set 0 to disable

// ST7796 MADCTL. 0x28 = MV + BGR, common 480x320 landscape orientation.
// If the image is mirrored on your exact panel, change this only; wiring stays the same.
static constexpr uint8_t ST7796_MADCTL = 0x28;

// Piece size on a 40x40 board square.
static constexpr int PIECE_W = 36;
static constexpr int PIECE_H = 36;

// -------- Built-in Wi-Fi --------
// Priority is the array order. The first visible network is tried first.
struct BuiltinWiFi {
  const char* ssid;
  const char* password;
};

static constexpr BuiltinWiFi BUILTIN_WIFI[] = {
  {"REPLACE_WITH_YOUR_WIFI_SSID", "REPLACE_WITH_YOUR_WIFI_PASSWORD"},
  {"REPLACE_WITH_YOUR_WIFI_SSID",        "REPLACE_WITH_YOUR_WIFI_PASSWORD"},
  {"REPLACE_WITH_YOUR_WIFI_SSID",        "REPLACE_WITH_YOUR_WIFI_PASSWORD"},
};
static constexpr size_t BUILTIN_WIFI_COUNT =
    sizeof(BUILTIN_WIFI) / sizeof(BUILTIN_WIFI[0]);

// One additional Wi-Fi can be selected on the touchscreen and is saved in
// NVS. It is tried after these three built-in networks.
