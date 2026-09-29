# ESP32-C5 Lichess Handheld Simulator v5.5

## Puzzle history behavior corrected

`< Prev` / `Next >` now means **actual played history only**.

- It never steps into a future solution move that has not happened yet.
- At the live/latest position, `Next >` is disabled.
- After pressing `< Prev`, `Next >` can move forward only as far as the latest move that actually occurred.
- History review does not reveal the answer and does not change solving progress.
- `Resume` returns to the exact live puzzle position.
- The Answer page still prints the full solution as text, but no longer lets board-history navigation expose future moves.

## Two-stage Hint

Press Hint once:

- highlights the piece that should move.

Press Hint a second time:

- keeps the source highlighted;
- draws an arrow from source to destination.

Hints reset automatically after a correct move.

## Very fast move animation

Moves no longer visually teleport.

- duration: about **100 ms**
- simulator: roughly **3 frames at 30 FPS**
- smoothstep interpolation for a short, clean slide
- your online moves animate immediately with optimistic UI
- opponent/AI Board API moves animate when received
- puzzle solver moves animate
- the automatic puzzle reply waits for the first 100 ms animation, then also animates

This timing is intentionally short for the future lower-refresh ESP32 display.
