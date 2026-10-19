"""Importa os extras do Barbaro (a figura dele sai do build_sprite_assets.py, no formato do druida):
  Sprites/Barbaro/fx_grito.png (4x1)        -> assets/sprites/spr_barb_fx_shout_d0_f0..3.rgba16.png (64x64)
  Sprites/Barbaro/fx_impacto.png (4x1)      -> assets/sprites/spr_barb_fx_impact_d0_f0..3.rgba16.png (64x48)
  Sprites/Barbaro/icones_barbaro.png (3x2)  -> assets/sprites/spr_hud_bicon_0..4.rgba16.png (30x30)
  + cartas provisorias card_48..71 (as de verdade vem do tools/import_cards.py)
Sem a prancheta, gera um provisorio para a ROM nunca ficar sem arquivo.  Uso: python tools/import_barbarian.py
"""
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageDraw
sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_cripta_caverna import figure_box, cells, finish
from hud_icon_frame import frame_icon

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "Barbaro"
SPR = ROOT / "assets" / "sprites"
ASSETS = ROOT / "assets"


def tint(im, t):
    a = np.asarray(im.convert("RGBA")).astype(np.float32)
    for k in range(3): a[..., k] *= t[k]
    return Image.fromarray(a.clip(0, 255).astype(np.uint8), "RGBA")


def fx(fname, name, size, ring):
    p = SRC / fname
    if p.exists():
        cs = cells(Image.open(p).convert("RGBA"), (4, 1))
        boxes = [figure_box(c) for c in cs]
        xs = [b[0] for b in boxes if b]
        union = (min(b[0] for b in xs), min(b[1] for b in xs), max(b[2] for b in xs), max(b[3] for b in xs))
        for i, c in enumerate(cs):
            a = np.asarray(c).copy()
            if boxes[i]: a[~boxes[i][1], 3] = 0
            finish(Image.fromarray(a, "RGBA").crop(union), size, "decal" if ring else "anim").save(SPR / f"spr_{name}_d0_f{i}.rgba16.png", optimize=True)
        return True
    for i in range(4):   # provisorio: anel / poeira desenhados
        im = Image.new("RGBA", size, (0, 0, 0, 0)); d = ImageDraw.Draw(im)
        W, H = size; k = (i + 1) / 4
        if ring:
            rx, ry = W * 0.48 * k, H * 0.30 * k
            d.ellipse((W / 2 - rx, H / 2 - ry, W / 2 + rx, H / 2 + ry), outline=(230, 220, 200, 255), width=2)
        else:
            for j in range(9):
                ang = j * 0.7; r = 6 + 16 * k
                x, y = W / 2 + np.cos(ang) * r, H - 8 - abs(np.sin(ang)) * r * 0.6
                d.ellipse((x - 3, y - 3, x + 3, y + 3), fill=(120, 105, 90, 255))
        im.save(SPR / f"spr_{name}_d0_f{i}.rgba16.png", optimize=True)
    return False


def icons():
    p = SRC / "icones_barbaro.png"
    if p.exists():
        cs = cells(Image.open(p).convert("RGBA"), (3, 2))
        for i in range(5):
            frame_icon(cs[i], zoom=1.12).save(SPR / f"spr_hud_bicon_{i}.rgba16.png", optimize=True)
        return True
    for i in range(5):   # provisorio: icones do druida em tom de fogo
        tint(Image.open(SPR / f"spr_hud_icon_{i}.rgba16.png"), (1.25, 0.70, 0.45)).save(SPR / f"spr_hud_bicon_{i}.rgba16.png", optimize=True)
    return False


def cards():
    n = 0
    for k in range(24):
        out = ASSETS / f"card_{48 + k:02d}.rgba16.png"
        mark = ASSETS / f".card_{48 + k:02d}.placeholder"
        if out.exists() and not mark.exists():
            continue                                  # ja tem a arte de verdade
        src = ASSETS / f"card_{k:02d}.rgba16.png"
        if not src.exists():
            continue
        tint(Image.open(src), (1.15, 0.78, 0.62)).save(out, optimize=True)
        mark.write_text("provisorio\n")
        n += 1
    return n


if __name__ == "__main__":
    r = [fx("fx_grito.png", "barb_fx_shout", (64, 64), True), fx("fx_impacto.png", "barb_fx_impact", (64, 48), False), icons()]
    print("barbaro: efeitos/icones", ["provisorio", "arte nova"][all(r)], "| cartas provisorias:", cards())
