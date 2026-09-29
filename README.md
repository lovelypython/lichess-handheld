# ESP32-C5 Lichess Handheld Simulator v5.2

## Clock / latency fixes

The chess clock now behaves like a real client clock:

- Every `gameFull` / `gameState` clock value from Lichess is treated as an authoritative snapshot.
- Between snapshots, the active side counts down locally using `time.monotonic()`.
- The display redraws at 30 FPS instead of waiting for another server event.
- Under 20 seconds, the UI shows tenths of a second.
- Every incoming game state re-syncs the local clock.
- A small one-way network estimate is applied to the display after server snapshots.
- Your own move is shown immediately (optimistic UI) instead of waiting for the server echo.
- Fischer increment is applied locally, then corrected by the next authoritative state.
- If Lichess rejects the move, the board and clock roll back safely.

## Streaming latency

`requests.iter_lines()` now uses `chunk_size=1` for the Lichess NDJSON streams to minimize client-side buffering.

## Diagnostics

The Network page now separates:

- **Warm API TTFB** — closer to normal in-game API response latency.
- **Cold DNS/TCP/TLS/API** — first connection cost.
- **Last move POST** — touch-to-HTTP-response time.
- **stream sync** — touch-to-authoritative-gameState time.

The old version measured a complete fresh GET of the Lichess homepage, so the number could look much larger than the actual in-game API latency.

## Run

```bash
export LICHESS_TOKEN='YOUR_TOKEN'
python3.13 simulator.py
```
