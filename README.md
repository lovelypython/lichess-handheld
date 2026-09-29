# Lichess Handheld

ESP32-C5 handheld chess terminal project, recovered as a chronological Git history from the Mac/Python simulator through touchscreen testing and the final firmware refinements.

## Development stages

- Python/Mac simulator: board interaction, online flow, clocks, optimistic moves, history navigation, hints, and animation.
- ESP32-C5 PlatformIO port: display, backlight, sleep/wake, Wi-Fi, Lichess client, chess rules, puzzle and online-game screens.
- Touch testing: XPT2046 baseline, standalone diagnostic, independent software SPI, and heartbeat diagnostics.
- Firmware refinement: regional display refresh, TLS setup, landscape rendering, calibrated touch, mirrored-X correction, stale-cell prevention, and puzzle feedback.

## Hardware

Target hardware is a Waveshare ESP32-C5 board with a 480x320 SPI display and XPT2046 touch controller. The current enclosure design is in [hardware/enclosure](hardware/enclosure/).

The enclosure package includes the v05 OpenSCAD source and printable package, plus the v06 rear-shell update. The handoff document records the fit, screw, battery-door, ESP clip, antenna, and Bambu Lab A1 mini printing constraints.

## Build

Install PlatformIO and build the selected historical revision locally:

```bash
pio run -e esp32c5
```

Keep Wi-Fi and Lichess credentials in a local, untracked configuration. This repository contains no passwords, network names, access tokens, compiled firmware images, checksum bundles, or upload wrappers.

## Safety

Do not commit credentials or distribute precompiled firmware. Review the wiring and battery protection before powering the handheld.
