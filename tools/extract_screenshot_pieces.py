#!/usr/bin/env python3
"""Extract the 12 user-selected Neo pieces from their supplied board screenshot."""
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets_user" / "source_board.png"
OUT = ROOT / "assets_user" / "neo"

# name -> (file, displayed row). Avoid edge pieces except rooks; connected-
# component filtering removes the rank/file labels from those squares.
SQUARES = {
    "br": (7, 0), "bn": (6, 0), "bb": (5, 0), "bq": (3, 0),
    "bk": (4, 0), "bp": (4, 1),
    "wr": (7, 7), "wn": (6, 7), "wb": (5, 7), "wq": (3, 7),
    "wk": (4, 7), "wp": (4, 6),
}

def main():
    im = Image.open(SOURCE).convert("RGBA")
    w, h = im.size
    OUT.mkdir(parents=True, exist_ok=True)
    for name, (file_, row) in SQUARES.items():
        x0, x1 = round(file_ * w / 8), round((file_ + 1) * w / 8)
        y0, y1 = round(row * h / 8), round((row + 1) * h / 8)
        tile = im.crop((x0, y0, x1, y1)).convert("RGBA")
        tw, th = tile.size
        samples = [tile.getpixel((3, 3)), tile.getpixel((tw-4, 3)),
                   tile.getpixel((3, th-4)), tile.getpixel((tw-4, th-4))]
        bg = min(samples, key=lambda c: sum(abs(c[i]-samples[0][i]) for i in range(3)))
        raw = tile.load()
        mask = [[False] * tw for _ in range(th)]
        for y in range(th):
            for x in range(tw):
                p = raw[x, y]
                dist = sum(abs(int(p[i])-int(bg[i])) for i in range(3))
                mask[y][x] = dist > 34

        # Chess.com prints file letters in the bottom-right corner of rank 1.
        # They can touch a piece's antialiased shadow (notably the knight on
        # g1), so remove that coordinate-only corner before component tracing.
        if row == 7:
            for y in range(int(th * 0.76), th):
                for x in range(int(tw * 0.84), tw):
                    mask[y][x] = False

        seen = [[False] * tw for _ in range(th)]
        comps = []
        for y in range(th):
            for x in range(tw):
                if not mask[y][x] or seen[y][x]:
                    continue
                q = deque([(x, y)]); seen[y][x] = True; pts = []
                while q:
                    px, py = q.popleft(); pts.append((px, py))
                    for nx, ny in ((px-1,py),(px+1,py),(px,py-1),(px,py+1)):
                        if 0 <= nx < tw and 0 <= ny < th and mask[ny][nx] and not seen[ny][nx]:
                            seen[ny][nx] = True; q.append((nx, ny))
                comps.append(pts)
        # The piece is by far the largest component and is centered; coordinate
        # labels are small components near an edge.
        comps.sort(key=len, reverse=True)
        keep = set(comps[0])
        xs = [p[0] for p in keep]; ys = [p[1] for p in keep]
        bx0, bx1 = max(0,min(xs)-4), min(tw,max(xs)+5)
        by0, by1 = max(0,min(ys)-4), min(th,max(ys)+5)
        rgba = Image.new("RGBA", (tw, th), (0,0,0,0))
        outp = rgba.load()
        for x, y in keep:
            outp[x,y] = raw[x,y]
        piece = rgba.crop((bx0,by0,bx1,by1))
        piece.thumbnail((36,36), Image.Resampling.LANCZOS)
        canvas = Image.new("RGBA", (36,36), (0,0,0,0))
        canvas.alpha_composite(piece, ((36-piece.width)//2,(36-piece.height)//2))
        canvas.save(OUT / f"{name}.png")
    print("Extracted", len(SQUARES), "piece sprites from", SOURCE.name)

if __name__ == "__main__":
    main()
