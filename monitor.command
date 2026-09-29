#!/bin/zsh
set -e
cd "$(dirname "$0")"
PIO="$HOME/.platformio/penv/bin/pio"
[[ -x "$PIO" ]] || PIO="$(command -v pio)"
PORT="${UPLOAD_PORT:-$(ls /dev/cu.usbmodem* /dev/cu.wchusbserial* /dev/cu.usbserial* 2>/dev/null | head -n 1 || true)}"
[[ -n "$PORT" ]] || { echo "No serial port found"; exit 1; }
"$PIO" device monitor --port "$PORT" --baud 115200
