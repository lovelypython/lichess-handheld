# Lichess Handheld Mac Simulator

A 480×320 macOS simulator for the planned ESP32-C5 handheld Lichess client.

## What it tests

- Lichess authentication with a Personal Access Token
- Exact 480×320 UI layout
- 320×320 chessboard + 160px sidebar
- Mouse clicks as touchscreen taps
- Create an AI game
- Stream the game with Lichess Board API NDJSON
- Submit real UCI moves through the Board API
- Resign a game

There is **no chess engine assistance** in this simulator.

## 1. Create a development token

On Lichess, create a Personal Access Token with these scopes:

- `board:play`
- `challenge:write`

Use a token only for local development. Do not paste it into the source file, Git, screenshots, or chat.

## 2. Install

```bash
cd lichess_handheld_mac_sim
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

## 3. Put the token in your shell

```bash
export LICHESS_TOKEN='PASTE_YOUR_TOKEN_HERE'
```

This only sets it for the current Terminal session.

Optional quick check:

```bash
curl https://lichess.org/api/account \
  -H "Authorization: Bearer $LICHESS_TOKEN"
```

## 4. Run

```bash
python3 simulator.py
```

Controls:

- Mouse click = touch
- Click a piece, then its destination = make a move
- `N` = new 5+3 AI game
- `R` = resign
- `Esc` = quit

The program defaults promotions to a queen for this first prototype.

## Architecture we can keep for the ESP32 version

```text
UI / chess state
      |
      +-- LichessClient
              |
              +-- GET /api/board/game/stream/{gameId}
              +-- POST /api/board/game/{gameId}/move/{uci}
              +-- POST /api/board/game/{gameId}/resign
```

Later on ESP32-C5:

- Pygame renderer -> LVGL / TFT_eSPI / esp_lcd renderer
- Mouse events -> resistive/capacitive touch events
- `requests` -> ESP-IDF `esp_http_client`
- Personal token -> OAuth2 PKCE account binding

The game/state logic can remain conceptually the same.
