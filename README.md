# ESP32-C5 Lichess Handheld Simulator v4.1

Bug-fix release focused on puzzle correctness.

## Fixed

- Lichess `initialPly` was interpreted one ply too early.
  - Correct reconstruction replays `initialPly + 1` PGN plies.
  - This fixes puzzles where **Black is the solver**.
- Board orientation now follows the actual puzzle side to move.
- Puzzle sidebar explicitly says **White to move** or **Black to move**.
- First solution move is validated against the reconstructed position.
- Automatic puzzle replies are validated instead of silently failing.
- Mate-in-1 accepts any legal move that immediately checkmates.
- Online/AI color handling is more robust if the account lookup and game event race.

## Run

```bash
export LICHESS_TOKEN='YOUR_TOKEN'
python3.13 simulator.py
```

Dependencies remain:

```bash
python3.13 -m pip install requests pygame python-chess
```
