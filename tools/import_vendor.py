"""Importa o mascate demoniaco e a essencia (moeda):
  Sprites/Mercador_Demonio/v1_mascate.png   (3 quadros lado a lado, grade 3x1) -> assets/scenery/scene_vendor_f0..f2.rgba16.png (64x80)
  Sprites/Mercador_Demonio/v3_selos.png     (2 icones, grade 2x1: intacto, destruido) -> assets/scenery/hud_seal_on/off.rgba16.png (16x16)
  Sprites/Mercador_Demonio/v2_essencia.png  (4 quadros lado a lado, grade 4x1) -> assets/sprites/spr_essence_d0_f0..f3.rgba16.png (32x32)
Uso:  python tools/import_vendor.py   (depois e so compilar)
"""
from pathlib import Path
import sys
import numpy as np
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_cripta_caverna import figure_box, cells, finish

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "Mercador_Demonio"
JOBS = [
    ("mercador_demonio_idle.png", (3, 1), ROOT / "assets" / "scenery", "scene_vendor_f{}", (64, 80)),
    ("essencia_demoniaca.png", (4, 1), ROOT / "assets" / "sprites", "spr_essence_d0_f{}", (32, 32)),
]


def run():
    for fname, grid, out_dir, pattern, size in JOBS:
        path = SRC / fname
        if not path.exists():
            print(f"(faltando {path.name} - mantendo o placeholder)")
            continue
        cs = cells(Image.open(path).convert("RGBA"), grid)
        boxes = [figure_box(c) for c in cs]
        valid = [b[0] for b in boxes if b and b[0]]
        # mesma caixa para todos os quadros: a animacao nao "pula"
        union = (min(b[0] for b in valid), min(b[1] for b in valid), max(b[2] for b in valid), max(b[3] for b in valid))
        for i, c in enumerate(cs):
            box, keep = boxes[i]
            a = np.asarray(c).copy()
            a[~keep, 3] = 0
            fig = Image.fromarray(a, "RGBA").crop(union)
            name = pattern[i] if isinstance(pattern, list) else pattern.format(i)
            finish(fig, size, "anim").save(out_dir / f"{name}.rgba16.png", optimize=True)
        print(f"{fname}: {len(cs)} quadros")


def seals():
    # icones ja vem em pixel art 16x16 (2 lado a lado): corta direto, sem reamostrar
    path = SRC / "selos_demonicos_16x16.png"
    if not path.exists():
        print("(faltando selos_demonicos_16x16.png - mantendo o placeholder)")
        return
    img = Image.open(path).convert("RGBA")
    for i, name in enumerate(["hud_seal_on", "hud_seal_off"]):
        img.crop((i * 16, 0, i * 16 + 16, 16)).save(ROOT / "assets" / "scenery" / f"{name}.rgba16.png", optimize=True)
    print("selos: 2 icones")


if __name__ == "__main__":
    run()
    seals()
