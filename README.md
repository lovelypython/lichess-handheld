# ESP32-C5 Lichess Handheld Simulator v5

This build adds the interaction flow requested for the future handheld.

## New

- **Network connection screen**
  - Tests real Lichess reachability from the Mac
  - Shows connected/offline state and latency
  - Home-screen network indicator
  - Future ESP32 version can replace this screen with Wi-Fi scan/SSID/password UI

- **Online match confirmation**
  - Pick time control first
  - Pick Casual / Rated
  - Review selected time, game type, account and network
  - Explicit **Confirm & Search** before matchmaking begins
  - Searching screen with Cancel

- **Puzzle flow improvements**
  - Uses the Lichess puzzle batch API when possible
  - `Next` avoids redisplaying the current puzzle
  - With `puzzle:write`, the previous unrated result is recorded before loading the next puzzle
  - **Hint** highlights the exact piece that should move
  - **Answer** shows the full solution in SAN steps
  - Answer screen supports Back / Hint / Next puzzle
  - Wrong attempts and answer reveals do not count as solved in the prototype

## Recommended token scopes

- `board:play`
- `challenge:write`
- `puzzle:read`
- `puzzle:write`

## Run

```bash
export LICHESS_TOKEN='YOUR_TOKEN'
python3.13 simulator.py
```
