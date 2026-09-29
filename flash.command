#!/bin/zsh
set -e
cd "$(dirname "$0")"

if [[ -x "$HOME/.platformio/penv/bin/pio" ]]; then
  PIO="$HOME/.platformio/penv/bin/pio"
elif command -v pio >/dev/null 2>&1; then
  PIO="$(command -v pio)"
else
  echo "PlatformIO CLI not found."
  echo "Install PlatformIO first, then run this file again."
  exit 1
fi

PORT="${UPLOAD_PORT:-}"
if [[ -z "$PORT" ]]; then
  PORT="$(ls /dev/cu.usbmodem* /dev/cu.wchusbserial* /dev/cu.usbserial* 2>/dev/null | head -n 1 || true)"
fi
if [[ -z "$PORT" ]]; then
  echo "No ESP serial port detected."
  echo "If needed: hold BOOT, tap RESET, release RESET, then release BOOT."
  exit 1
fi

echo "Using port: $PORT"
"$PIO" run -e esp32c5 -t upload --upload-port "$PORT"
echo
echo "Flash complete."
echo "Opening serial monitor at 115200..."
"$PIO" device monitor --port "$PORT" --baud 115200
