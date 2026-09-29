# ESP32-C5 Lichess Handheld Simulator v5.1

Adds puzzle **previous-step / next-step navigation**.

## Puzzle controls

During a puzzle:

- **Hint** — highlights the piece that should move.
- **Answer** — shows the full SAN solution list.
- **< Prev / Next >** — enters solution review and moves exactly one ply backward/forward.
- **Resume** — returns to the exact live solving position from before review.
- **Next Puz** — loads a different Lichess puzzle.

The review board is read-only so stepping through the answer cannot accidentally submit a move.
Opening step review counts as revealing the answer, so that attempt is not treated as a clean solve.

All v5 features remain: network page, online confirmation, time controls, AI levels, Lichess puzzle API, hints and full answer steps.
