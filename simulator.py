#!/usr/bin/env python3
import os
import sys
import json
import queue
import threading
import time

import requests
import pygame
import chess

BASE = "https://lichess.org"
TOKEN = os.environ.get("LICHESS_TOKEN", "").strip()

W, H = 480, 320
BOARD_PX = 320
SQ = 40
SIDEBAR_X = 320

HEADERS = {
    "Authorization": f"Bearer {TOKEN}",
    "Accept": "application/json",
    "User-Agent": "ESP32-C5-Handheld-Chess-Prototype/0.1",
}

event_q = queue.Queue()


class LichessClient:
    def __init__(self):
        self.session = requests.Session()
        self.session.headers.update(HEADERS)
        self.account = None
        self.game_id = None
        self.stream_stop = threading.Event()

    def check_account(self):
        r = self.session.get(f"{BASE}/api/account", timeout=15)
        r.raise_for_status()
        self.account = r.json()
        return self.account

    def create_ai_game(self, level=1, minutes=5, increment=3):
        data = {
            "level": level,
            "clock.limit": minutes * 60,
            "clock.increment": increment,
            "color": "random",
            "variant": "standard",
        }
        r = self.session.post(f"{BASE}/api/challenge/ai", data=data, timeout=15)
        r.raise_for_status()
        payload = r.json()
        game_id = payload.get("id")
        if not game_id:
            raise RuntimeError(f"No game id in response: {payload}")
        self.game_id = game_id
        return payload

    def post_move(self, uci):
        if not self.game_id:
            return False, "No active game"
        try:
            r = self.session.post(
                f"{BASE}/api/board/game/{self.game_id}/move/{uci}",
                timeout=15,
            )
            if r.status_code == 200:
                return True, "Move sent"
            return False, f"HTTP {r.status_code}: {r.text[:120]}"
        except Exception as e:
            return False, str(e)

    def resign(self):
        if not self.game_id:
            return False, "No active game"
        try:
            r = self.session.post(
                f"{BASE}/api/board/game/{self.game_id}/resign",
                timeout=15,
            )
            return r.status_code == 200, f"HTTP {r.status_code}"
        except Exception as e:
            return False, str(e)

    def stop_stream(self):
        self.stream_stop.set()

    def start_game_stream(self, game_id):
        self.stream_stop.clear()

        def worker():
            url = f"{BASE}/api/board/game/stream/{game_id}"
            try:
                with self.session.get(
                    url,
                    stream=True,
                    timeout=(15, None),
                    headers={**HEADERS, "Accept": "application/x-ndjson"},
                ) as r:
                    r.raise_for_status()
                    for line in r.iter_lines(decode_unicode=True):
                        if self.stream_stop.is_set():
                            break
                        if not line:
                            continue
                        try:
                            event_q.put(("game_event", json.loads(line)))
                        except json.JSONDecodeError:
                            event_q.put(("status", f"Bad JSON: {line[:80]}"))
            except Exception as e:
                event_q.put(("status", f"Stream error: {e}"))

        threading.Thread(target=worker, daemon=True).start()


class App:
    def __init__(self, client):
        pygame.init()
        pygame.display.set_caption("ESP32-C5 Lichess Handheld Simulator · 480×320")
        self.screen = pygame.display.set_mode((W, H))
        self.clock = pygame.time.Clock()

        self.font_piece = pygame.font.SysFont("Arial", 27, bold=True)
        self.font_big = pygame.font.SysFont("Arial", 22, bold=True)
        self.font = pygame.font.SysFont("Arial", 15)
        self.font_small = pygame.font.SysFont("Arial", 12)

        self.client = client
        self.board = chess.Board()
        self.selected = None
        self.last_move = None
        self.my_color = chess.WHITE
        self.account_id = ""
        self.opponent = "—"
        self.status = "Starting..."
        self.wtime = None
        self.btime = None
        self.game_status = "idle"
        self.game_id = None

        self.piece_letters = {
            chess.PAWN: "P",
            chess.KNIGHT: "N",
            chess.BISHOP: "B",
            chess.ROOK: "R",
            chess.QUEEN: "Q",
            chess.KING: "K",
        }

    def set_status(self, msg):
        self.status = str(msg)[:70]

    def new_ai_game(self):
        def worker():
            event_q.put(("status", "Creating Lichess AI game..."))
            try:
                payload = self.client.create_ai_game(level=1, minutes=5, increment=3)
                gid = payload["id"]
                event_q.put(("new_game", gid))
            except Exception as e:
                event_q.put(("status", f"Create game failed: {e}"))

        threading.Thread(target=worker, daemon=True).start()

    def send_move(self, uci):
        def worker():
            ok, msg = self.client.post_move(uci)
            event_q.put(("status", msg if ok else f"Move failed: {msg}"))

        threading.Thread(target=worker, daemon=True).start()

    def resign(self):
        def worker():
            ok, msg = self.client.resign()
            event_q.put(("status", "Resigned" if ok else f"Resign failed: {msg}"))

        threading.Thread(target=worker, daemon=True).start()

    def apply_moves(self, moves_text):
        b = chess.Board()
        last = None
        for token in moves_text.split():
            try:
                mv = chess.Move.from_uci(token)
                if mv not in b.legal_moves:
                    break
                b.push(mv)
                last = mv
            except Exception:
                break
        self.board = b
        self.last_move = last
        if self.selected is not None:
            piece = self.board.piece_at(self.selected)
            if piece is None or piece.color != self.my_color:
                self.selected = None

    def process_game_event(self, data):
        typ = data.get("type")
        if typ == "gameFull":
            self.game_id = data.get("id", self.game_id)
            white = data.get("white", {})
            black = data.get("black", {})
            white_id = (white.get("id") or "").lower()
            black_id = (black.get("id") or "").lower()

            if self.account_id == white_id:
                self.my_color = chess.WHITE
                self.opponent = black.get("name") or black.get("id") or "Black"
            elif self.account_id == black_id:
                self.my_color = chess.BLACK
                self.opponent = white.get("name") or white.get("id") or "White"

            state = data.get("state", {})
            self.apply_moves(state.get("moves", ""))
            self.wtime = state.get("wtime")
            self.btime = state.get("btime")
            self.game_status = state.get("status", "started")
            self.set_status(f"Connected · game {self.game_id}")

        elif typ == "gameState":
            self.apply_moves(data.get("moves", ""))
            self.wtime = data.get("wtime")
            self.btime = data.get("btime")
            self.game_status = data.get("status", self.game_status)
            if self.game_status != "started":
                self.set_status(f"Game ended: {self.game_status}")

    def display_square_to_chess(self, file_idx, rank_idx):
        if self.my_color == chess.WHITE:
            file_ = file_idx
            rank_ = 7 - rank_idx
        else:
            file_ = 7 - file_idx
            rank_ = rank_idx
        return chess.square(file_, rank_)

    def chess_square_to_display(self, square):
        file_ = chess.square_file(square)
        rank_ = chess.square_rank(square)
        if self.my_color == chess.WHITE:
            return file_, 7 - rank_
        else:
            return 7 - file_, rank_

    def handle_board_click(self, x, y):
        if not self.game_id or self.game_status not in ("started", "created"):
            return
        fx, ry = x // SQ, y // SQ
        sq = self.display_square_to_chess(fx, ry)

        if self.selected is None:
            piece = self.board.piece_at(sq)
            if piece and piece.color == self.my_color and self.board.turn == self.my_color:
                self.selected = sq
            return

        if sq == self.selected:
            self.selected = None
            return

        piece = self.board.piece_at(self.selected)
        promotion = None
        if piece and piece.piece_type == chess.PAWN:
            target_rank = chess.square_rank(sq)
            if target_rank in (0, 7):
                promotion = chess.QUEEN

        mv = chess.Move(self.selected, sq, promotion=promotion)
        if mv in self.board.legal_moves:
            uci = mv.uci()
            self.selected = None
            self.set_status(f"Sending {uci}...")
            self.send_move(uci)
        else:
            # Allow selecting another own piece directly
            p2 = self.board.piece_at(sq)
            if p2 and p2.color == self.my_color:
                self.selected = sq
            else:
                self.set_status("Illegal move")
                self.selected = None

    def draw_board(self):
        light = (235, 236, 208)
        dark = (115, 149, 82)
        selected_c = (246, 246, 105)
        last_c = (205, 210, 106)
        legal_c = (50, 50, 50)

        for dy in range(8):
            for dx in range(8):
                sq = self.display_square_to_chess(dx, dy)
                rect = pygame.Rect(dx * SQ, dy * SQ, SQ, SQ)
                base = light if (dx + dy) % 2 == 0 else dark
                color = base

                if self.last_move and sq in (self.last_move.from_square, self.last_move.to_square):
                    color = last_c
                if sq == self.selected:
                    color = selected_c

                pygame.draw.rect(self.screen, color, rect)

                piece = self.board.piece_at(sq)
                if piece:
                    letter = self.piece_letters[piece.piece_type]
                    fg = (245, 245, 245) if piece.color == chess.WHITE else (30, 30, 30)
                    outline = (25, 25, 25) if piece.color == chess.WHITE else (230, 230, 230)

                    # simple "disc + letter" style; reliable on macOS without chess-glyph fonts
                    center = rect.center
                    pygame.draw.circle(self.screen, outline, center, 14)
                    pygame.draw.circle(self.screen, fg, center, 12)
                    text_color = (30, 30, 30) if piece.color == chess.WHITE else (245, 245, 245)
                    txt = self.font_piece.render(letter, True, text_color)
                    self.screen.blit(txt, txt.get_rect(center=center))

        if self.selected is not None:
            for mv in self.board.legal_moves:
                if mv.from_square != self.selected:
                    continue
                dx, dy = self.chess_square_to_display(mv.to_square)
                cx = dx * SQ + SQ // 2
                cy = dy * SQ + SQ // 2
                pygame.draw.circle(self.screen, legal_c, (cx, cy), 5)

    @staticmethod
    def fmt_ms(ms):
        if ms is None:
            return "--:--"
        total = max(0, int(ms) // 1000)
        return f"{total // 60:02d}:{total % 60:02d}"

    def draw_button(self, rect, label):
        pygame.draw.rect(self.screen, (65, 65, 65), rect, border_radius=6)
        txt = self.font.render(label, True, (245, 245, 245))
        self.screen.blit(txt, txt.get_rect(center=rect.center))

    def draw_sidebar(self):
        pygame.draw.rect(self.screen, (38, 38, 38), (SIDEBAR_X, 0, 160, 320))
        x = SIDEBAR_X + 10

        title = self.font_big.render("Lichess", True, (245, 245, 245))
        self.screen.blit(title, (x, 9))

        my_name = (self.client.account or {}).get("username", "You")
        top_name = self.opponent if self.my_color == chess.WHITE else my_name
        bot_name = my_name if self.my_color == chess.WHITE else self.opponent

        top_time = self.btime if self.my_color == chess.WHITE else self.wtime
        bot_time = self.wtime if self.my_color == chess.WHITE else self.btime

        self.screen.blit(self.font.render(top_name[:18], True, (220, 220, 220)), (x, 48))
        self.screen.blit(self.font_big.render(self.fmt_ms(top_time), True, (255, 255, 255)), (x, 69))

        pygame.draw.line(self.screen, (80, 80, 80), (x, 112), (470, 112), 1)

        self.screen.blit(self.font.render(bot_name[:18], True, (220, 220, 220)), (x, 126))
        self.screen.blit(self.font_big.render(self.fmt_ms(bot_time), True, (255, 255, 255)), (x, 147))

        turn_text = "Your turn" if self.board.turn == self.my_color else "Opponent"
        self.screen.blit(self.font.render(turn_text, True, (190, 190, 190)), (x, 182))

        new_rect = pygame.Rect(330, 215, 140, 34)
        resign_rect = pygame.Rect(330, 257, 140, 34)
        self.draw_button(new_rect, "New AI game")
        self.draw_button(resign_rect, "Resign")

        # Status strip at very bottom, clipped to sidebar
        status = self.font_small.render(self.status[:23], True, (180, 180, 180))
        self.screen.blit(status, (x, 302))

    def handle_sidebar_click(self, x, y):
        if pygame.Rect(330, 215, 140, 34).collidepoint(x, y):
            self.new_ai_game()
        elif pygame.Rect(330, 257, 140, 34).collidepoint(x, y):
            self.resign()

    def run(self):
        running = True
        while running:
            while True:
                try:
                    kind, data = event_q.get_nowait()
                except queue.Empty:
                    break
                if kind == "status":
                    self.set_status(data)
                elif kind == "new_game":
                    self.game_id = data
                    self.client.game_id = data
                    self.board = chess.Board()
                    self.selected = None
                    self.client.start_game_stream(data)
                    self.set_status(f"Opening {data}...")
                elif kind == "game_event":
                    self.process_game_event(data)

            for ev in pygame.event.get():
                if ev.type == pygame.QUIT:
                    running = False
                elif ev.type == pygame.KEYDOWN:
                    if ev.key == pygame.K_ESCAPE:
                        running = False
                    elif ev.key == pygame.K_n:
                        self.new_ai_game()
                    elif ev.key == pygame.K_r:
                        self.resign()
                elif ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                    x, y = ev.pos
                    if x < BOARD_PX:
                        self.handle_board_click(x, y)
                    else:
                        self.handle_sidebar_click(x, y)

            self.screen.fill((0, 0, 0))
            self.draw_board()
            self.draw_sidebar()
            pygame.display.flip()
            self.clock.tick(30)

        self.client.stop_stream()
        pygame.quit()


def main():
    if not TOKEN:
        print("ERROR: LICHESS_TOKEN is not set.")
        print("Run: export LICHESS_TOKEN='your_token_here'")
        print("Do not paste your token into source code.")
        sys.exit(1)

    client = LichessClient()
    try:
        account = client.check_account()
        print(f"Connected to Lichess as: {account.get('username')}")
        print("Controls: mouse = touch, N = new AI game, R = resign, Esc = quit")
    except Exception as e:
        print(f"Could not connect to Lichess: {e}")
        sys.exit(2)

    app = App(client)
    app.account_id = (account.get("id") or account.get("username") or "").lower()
    app.set_status(f"Logged in: {account.get('username')}")
    app.run()


if __name__ == "__main__":
    main()
