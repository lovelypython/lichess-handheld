# ESP32-C5 Chess Handheld — UI/Wi-Fi firmware v0.3

Target hardware:

- Waveshare ESP32-C5-WIFI6-KIT-N32R8-UM
- MSP4021 4.0-inch 480x320 ST7796S display
- XPT2046 resistive touch controller

## What changed from v0.2

v0.2 was a hardware bring-up screen and did not match the Mac v5.5 preview. This revision replaces that debug-first UI with the preview layout:

- `Chess Handheld` home page
- Online, AI, and Puzzle cards
- matching selector pages and controls
- clickable network status and Network button
- on-device Wi-Fi scan list
- tap a network, type its password on screen, connect, and save it
Wi-Fi credentials are configured locally and are not stored in this repository.
- screen sleep after **5 minutes** of no touch or serial activity
- first touch wakes the screen without activating a button

The Lichess game/API engine is not included in this UI/Wi-Fi revision yet. The three mode pages intentionally report that their API action is not linked instead of pretending a game started.

## Flash on macOS

Connect the board through the `USB Single Serial` port, then run:

```bash
chmod +x flash.command
UPLOAD_PORT=/dev/cu.usbmodem5C940959191 ./flash.command
```

If the device number changes, replace the port with the current `/dev/cu.usbmodem...` value.

## Wi-Fi behavior

Wi-Fi credentials are configured locally and are not stored in this repository.

The Wi-Fi page supports:

- signal strength and security status
- five networks per page
Wi-Fi credentials are configured locally and are not stored in this repository.
- password entry with digits, lower-case letters, upper-case letters, `.`, `_`, `-`, and `@`
- storage of the selected custom network in NVS

Useful serial commands:

```text
wifi scan
wifi reconnect
wifi clear
touch recalibrate
screen sleep
screen wake
status
```

## Display sleep

`SCREEN_IDLE_MS` is `300000` milliseconds in `include/config.h`. The display and backlight turn off after five minutes; the XPT2046 interrupt remains active for touch-to-wake.

## Notes

- The first boot still runs the three-point touch calibration.
- GPIO and screen-controller settings are unchanged from v0.2.
- The firmware removes the normal first-boot `Preferences.cpp: nvs_open failed: NOT_FOUND` noise by creating the preference namespaces before reading them.
