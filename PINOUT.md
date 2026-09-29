# MSP4021 / ST7796S + XPT2046 wiring — v1.0.6

For the first build, only connect the Waveshare ESP32-C5 board and the 4.0" display module.

| LCD pin | Connect to ESP32-C5 | Notes |
|---|---|---|
| VCC | **5V** | Main module power |
| GND | GND | Common ground |
| CS | GPIO23 | LCD chip select |
| RESET | GPIO0 | LCD reset |
| DC/RS | GPIO24 | LOW command / HIGH data |
| SDI (MOSI) | GPIO8 | LCD hardware-SPI MOSI |
| SCK | GPIO10 | LCD hardware-SPI clock |
| **LED** | **GPIO3** | Backlight control; HIGH=on, LOW=off; no 3V3 splitter needed |
| SDO (MISO) | GPIO9 | LCD hardware-SPI MISO |
| T_CLK | **GPIO5** | Touch software-SPI clock |
| T_CS | GPIO1 | Touch chip select |
| T_DIN | **GPIO7** | Touch software-SPI MOSI |
| T_DO | **GPIO25** | Touch software-SPI MISO |
| T_IRQ | GPIO4 | Touch interrupt, active LOW |

SD-card pins are not connected in v1.0.

## Why LED goes to GPIO3

The MSP4021 schematic includes an onboard S8050 transistor and 1 kΩ drive resistor for the backlight.
The module's `LED` header pin is therefore a **logic/PWM control input**, marked as active HIGH by the
manufacturer. It does not need to occupy the ESP32-C5 board's only 3V3 header pin.

This also gives real screen blanking:
- firmware sends ST7796S `DISPOFF` + `SLPIN`;
- GPIO3 goes LOW and the backlight turns off;
- XPT2046 remains powered;
- a touch IRQ wakes the panel and backlight.

## GPIO choices

- GPIO6 and GPIO15 are occupied on the Waveshare board.
- GPIO13/14 are left untouched for native USB/JTAG.
- GPIO27 drives the onboard RGB LED.
- GPIO28 is BOOT.
- GPIO3 is exposed and is used only for LCD backlight control here.
- The ESP32-C5 has one general-purpose hardware SPI controller, so the LCD uses
  hardware SPI and XPT2046 uses an independent software SPI bus on GPIO5/7/25.
