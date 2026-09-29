#!/usr/bin/env python3
"""
Convert 12 locally supplied chess-piece PNG files into RGB565 + 1-bit masks.

Expected files in assets_user/neo/:
    wp.png wn.png wb.png wr.png wq.png wk.png
    bp.png bn.png bb.png br.png bq.png bk.png

This tool deliberately does not download third-party artwork. Put files you are
allowed to use into the folder yourself, then run:

    python3 -m pip install pillow
    python3 tools/encode_pieces.py

It writes include/pieces_user.h.
"""
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
ASSET=ROOT/"assets_user"/"neo"
OUT=ROOT/"include"/"pieces_user.h"
NAMES=["wp","wn","wb","wr","wq","wk","bp","bn","bb","br","bq","bk"]
W=H=36

missing=[n for n in NAMES if not (ASSET/f"{n}.png").exists()]
if missing:
    raise SystemExit("Missing: "+", ".join(f"{n}.png" for n in missing))

def rgb565(r,g,b):
    return ((r & 0xF8)<<8)|((g & 0xFC)<<3)|(b>>3)

lines=[
    "#pragma once",
    "#include <Arduino.h>",
    "#define USER_PIECES_AVAILABLE 1",
    f"static constexpr int USER_PIECE_W={W};",
    f"static constexpr int USER_PIECE_H={H};",
]
for n in NAMES:
    im=Image.open(ASSET/f"{n}.png").convert("RGBA")
    im.thumbnail((W,H),Image.Resampling.LANCZOS)
    canvas=Image.new("RGBA",(W,H),(0,0,0,0))
    canvas.alpha_composite(im,((W-im.width)//2,(H-im.height)//2))
    pix=list(canvas.getdata())
    colors=[rgb565(r,g,b) for r,g,b,a in pix]
    mask=[0]*((W*H+7)//8)
    for i,(_,_,_,a) in enumerate(pix):
        if a>=32: mask[i>>3] |= 1<<(i&7)

    lines.append(f"static const uint16_t {n}_pix[{W*H}] PROGMEM={{")
    for i in range(0,len(colors),18):
        lines.append(",".join(f"0x{x:04X}" for x in colors[i:i+18])+",")
    lines.append("};")
    lines.append(f"static const uint8_t {n}_mask[{len(mask)}] PROGMEM={{")
    for i in range(0,len(mask),24):
        lines.append(",".join(f"0x{x:02X}" for x in mask[i:i+24])+",")
    lines.append("};")

lines += [
    "inline const uint16_t* pixelsFor(char p){",
    "  switch(p){",
]
for n in NAMES:
    ch = n[1].upper() if n[0]=="w" else n[1].lower()
    lines.append(f"    case '{ch}': return {n}_pix;")
lines += ["  } return nullptr; }","inline const uint8_t* maskFor(char p){","  switch(p){"]
for n in NAMES:
    ch = n[1].upper() if n[0]=="w" else n[1].lower()
    lines.append(f"    case '{ch}': return {n}_mask;")
lines += ["  } return nullptr; }"]

OUT.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Wrote",OUT)
