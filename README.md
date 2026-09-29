# ESP32-C5 Lichess Handheld — hardware firmware v0.1

Target:
- Waveshare ESP32-C5-WIFI6-KIT-N32R8-UM
- MSP4021 4.0" 480×320 ST7796S
- XPT2046 resistive touch

This is the **first real hardware burn**. It intentionally concentrates on the hardware path that is physically connected right now:

1. ST7796S display bring-up
2. XPT2046 touch + first-boot 3-point affine calibration
3. 320×320 chessboard + 160×320 sidebar
4. 100 ms piece-move animation
5. built-in Wi-Fi auto-connect
6. extra Wi-Fi saved in NVS
7. 32 MB Flash / 8 MB PSRAM PlatformIO configuration

It is the base for the Lichess API layer. Do not diagnose Lichess/game-stream bugs until this build has proven the screen, touch and Wi-Fi wiring.

## Built-in Wi-Fi order

1. REPLACE_WITH_YOUR_WIFI_SSID
2. REPLACE_WITH_YOUR_WIFI_SSID
3. REPLACE_WITH_YOUR_WIFI_SSID

An additional network can be stored from USB serial without recompiling:

```text
wifi add YourSSID|YourPassword
wifi reconnect
```

Other useful commands:

```text
wifi scan
wifi clear
touch recalibrate
status
```

## One-command flash on macOS

```bash
cd lichess_handheld_esp32c5_v01
chmod +x flash.command
./flash.command
```

The script:
- uses `$HOME/.platformio/penv/bin/pio` when present;
- auto-detects `/dev/cu.usbmodem*`, `/dev/cu.wchusbserial*`, or `/dev/cu.usbserial*`;
- builds, uploads, then opens the serial monitor.

If upload cannot connect:
1. hold BOOT,
2. tap RESET,
3. release RESET,
4. release BOOT,
5. run `./flash.command` again.

## Chess-piece artwork hook

The program includes a simple vector fallback so the first burn works immediately.

A converter is included for **locally supplied** PNGs:
- put `wp.png ... bk.png` in `assets_user/neo/`
- run `python3 tools/encode_pieces.py`
- rebuild/flash

It converts each piece to 36×36 RGB565 plus a 1-bit transparency mask and compiles it into firmware.

## If the panel is mirrored/rotated

Edit only this line in `include/config.h`:

```cpp
static constexpr uint8_t ST7796_MADCTL = 0x28;
```

The wiring does not change.

## First-boot test

Expected sequence:
1. backlight turns on;
2. dark splash screen appears;
3. Wi-Fi starts scanning;
4. touch calibration shows three orange targets;
5. chessboard appears;
6. tap a piece and another square to see the ~100 ms move animation.

If the backlight is on but the LCD stays white, stop and send:
- a photo of the wiring,
- the serial log from boot,
- whether the calibration targets ever appeared.
