# ESP32-C5 Lichess Handheld Mac Simulator v4

This is a 480×320 Mac-side prototype for the future ESP32-C5 handheld.

## New in v4

- Real **Lichess puzzle API** (`GET /api/puzzle/next`)
- **Online random matchmaking** through Board API seek
- Selectable online time controls:
  - 10+0
  - 10+5
  - 15+10
  - 30+0
- **AI difficulty 1–8**
- Selectable AI clocks:
  - 3+0
  - 5+3
  - 10+0
  - 15+10
- Casual / rated online toggle
- Chess pieces are embedded directly inside `simulator.py`
- No piece-image folder is needed

## Why not copied Chess.com assets?

The exact Chess.com artwork is proprietary. This build uses the open Cburnett Staunton set instead.
It is embedded in the source, so there is still only one program file to carry around.

## Token permissions

Recommended:

- `board:play`
- `challenge:write`
- `puzzle:read`

`challenge:write` is needed for AI challenge creation.
`board:play` is needed for Board API gameplay and random seeks.
Puzzle mode can retry anonymously if `puzzle:read` is missing.

## Install

```bash
python3.13 -m pip install requests pygame python-chess \
  -i https://pypi.org/simple \
  --trusted-host pypi.org \
  --trusted-host files.pythonhosted.org
```

Once your Python certificate setup is fixed, remove the two `--trusted-host` flags.

## Run

```bash
export LICHESS_TOKEN='YOUR_TOKEN'
python3.13 simulator.py
```

Puzzle mode also works without a token:

```bash
unset LICHESS_TOKEN
python3.13 simulator.py
```

## Board API timing note

For **random Board API seeks**, Lichess currently restricts normal matching to Rapid,
Classical and Correspondence. Blitz is allowed for direct challenges and AI games.
That is why the online random-match presets begin at 10+0, while AI includes 3+0 and 5+3.
