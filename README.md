# Lichess Handheld

This is a recovered historical source snapshot for the ESP32-C5 Lichess handheld project.

## Build from source

Use PlatformIO to build the selected revision locally. No compiled firmware images are distributed in this repository.

```bash
pio run -e esp32c5
```

## Local configuration

Set Wi-Fi and Lichess credentials only in your local configuration. Do not commit passwords, network names, or tokens.

## History

Each commit preserves one recovered development milestone, from the Python simulator through touchscreen diagnostics and the ESP32-C5 firmware.
