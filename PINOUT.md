# MSP4021 / ST7796S + XPT2046 wiring

This first build only needs the Waveshare ESP32-C5 board and the 4.0" display module.

| LCD pin | Connect to ESP32-C5 | Notes |
|---|---|---|
| VCC | **5V** | Module accepts 3.3–5V. 5V is preferred while the dev board is USB-powered. |
| GND | GND | Common ground |
| CS | GPIO23 | LCD chip select |
| RESET | GPIO0 | LCD reset |
| DC/RS | GPIO24 | LOW command / HIGH data |
| SDI (MOSI) | GPIO8 | Shared SPI MOSI |
| SCK | GPIO10 | Shared SPI clock |
| LED | **3V3** | Backlight always on for first test |
| SDO (MISO) | GPIO9 | Shared SPI MISO |
| T_CLK | GPIO10 | Touch shares SPI clock |
| T_CS | GPIO1 | Touch chip select |
| T_DIN | GPIO8 | Touch shares MOSI |
| T_DO | GPIO9 | Touch shares MISO |
| T_IRQ | GPIO4 | Touch interrupt, active LOW |

SD-card pins on the module are not connected in v0.1.

Why these GPIOs:
- GPIO6 and GPIO15 are occupied on the Waveshare board.
- GPIO13/14 are left alone for native USB.
- GPIO27 is the onboard RGB LED.
- GPIO28 is BOOT.
- The ESP32-C5 GPIO matrix allows SPI signals on available GPIOs; this build stays at 30 MHz.
