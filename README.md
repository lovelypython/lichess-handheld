# ESP32-C5 Lichess Handheld Simulator v5.3

## Built-in Wi-Fi profiles

Three known networks are embedded in the requested strict priority order:

1. `REPLACE_WITH_YOUR_WIFI_SSID`
2. `REPLACE_WITH_YOUR_WIFI_SSID`
3. `REPLACE_WITH_YOUR_WIFI_SSID`

Passwords are embedded in the program but never displayed in the UI or logs.

### macOS simulator behavior

On launch and when **Auto-connect now** is pressed:

- it detects the Mac Wi-Fi interface;
- if the legacy macOS `airport -s` scanner exists, it scans and selects the highest-priority visible built-in SSID;
- if scanning is unavailable on a newer macOS, it keeps a working known network instead of unnecessarily disrupting it;
- if no known network is currently connected and scanning is unavailable, it tries the three profiles in priority order using `networksetup`;
- only after Wi-Fi handling does it check Lichess connectivity.

To prevent the simulator from changing the Mac's Wi-Fi:

```bash
export LICHESS_AUTO_WIFI=0
python3.13 simulator.py
```

### ESP32-C5 target behavior

The exact same profile order is ready to map to:

```text
WiFi.scanNetworks()
-> choose lowest priority number that is visible
-> WiFi.begin(selected_ssid, selected_password)
-> if connection fails, try the next known profile
```

## Security note

The password strings are Base64-obfuscated inside `simulator.py` only to avoid casual display. Base64 is **not encryption**. Anyone with the source or sufficiently motivated access to the final firmware can recover embedded Wi-Fi passwords.

All v5.2 features remain: continuous locally-interpolated chess clocks, authoritative Lichess re-sync, optimistic moves, puzzle navigation/hints/answers, online confirmation and latency diagnostics.
