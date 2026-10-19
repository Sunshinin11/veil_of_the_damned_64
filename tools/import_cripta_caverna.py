"""Importa as pranchetas do ChatGPT para a Cripta/Caverna (mapa 2):
  Sprites/Cripta_Caverna/p*.png, t*.png  ->  assets/scenery/scene_cave_*.rgba16.png (+ chao/parede/topo)
Cada prancheta e uma grade de celulas iguais. Em cada celula pega a figura (e os pedacinhos soltos
dela, como pedrinhas), corta, reduz para o tamanho do N64 com nitidez, alfa de 1 bit, e encosta a base
no fundo do quadro (decals ficam centralizados). Quadros de animacao usam o MESMO recorte (sem tremer).
Uso:  python tools/import_cripta_caverna.py
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter
import cv2

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "Cripta_Caverna"
OUT = ROOT / "assets" / "scenery"

# arquivo, (colunas, linhas), [(nome de saida, celula, (largura max, altura max))], modo
SHEETS = [
    ("p1_estalagmites.png", (3, 1), [("cave_stalag_a", 0, (56, 112)), ("cave_stalag_b", 1, (56, 112)),
                                     ("cave_stalag_c", 2, (56, 112))], "base"),
    ("p2_pedras.png", (2, 2), [("cave_rocks_a", 0, (64, 40)), ("cave_rocks_b", 1, (64, 40)),
                               ("cave_rocks_c", 2, (64, 40)), ("cave_rocks_d", 3, (64, 40))], "base"),
    ("p3_cristais.png", (3, 1), [("cave_crystal_a", 0, (56, 64)), ("cave_crystal_b", 1, (56, 64)),
                                 ("cave_crystal_c", 2, (56, 64))], "base"),
    ("p4_cogumelos.png", (3, 1), [("cave_mush_a", 0, (48, 40)), ("cave_mush_b", 1, (48, 40)),
                                  ("cave_mush_c", 2, (48, 40))], "base"),
    ("p5_sarcofagos.png", (2, 1), [("cave_sarco_a", 0, (112, 80)), ("cave_sarco_b", 1, (112, 80))], "base"),
    ("p6_altar_sangue.png", (1, 1), [("cave_altar", 0, (104, 72))], "base"),
    ("p7_braseiro.png", (3, 1), [("cave_brazier_f0", 0, (64, 72)), ("cave_brazier_f1", 1, (64, 72)),
                                 ("cave_brazier_f2", 2, (64, 72))], "anim"),
    ("p8_estatuas.png", (2, 1), [("cave_statue_a", 0, (56, 112)), ("cave_statue_b", 1, (56, 112))], "base"),
    ("p9_ossos_corpos.png", (2, 2), [("cave_bones_a", 0, (80, 56)), ("cave_bones_b", 1, (80, 56)),
                                     ("cave_corpse", 2, (96, 56)), ("cave_ribcage", 3, (80, 56))], "base"),
    ("p10_decals.png", (3, 2), [("cave_decal_blood", 0, (64, 32)), ("cave_decal_bones", 1, (64, 32)),
                                ("cave_decal_cracks", 2, (64, 32)), ("cave_decal_grave", 3, (64, 32)),
                                ("cave_decal_water", 4, (64, 32)), ("cave_decal_web", 5, (64, 32))], "decal"),
    ("p11_portal.png", (3, 1), [("cave_portal_f0", 0, (96, 96)), ("cave_portal_f1", 1, (96, 96)),
                                ("cave_portal_f2", 2, (96, 96))], "anim"),
    ("p12_entrada.png", (1, 1), [("cave_entrance", 0, (128, 120))], "base"),
    ("p13_trono_ossos.png", (1, 1), [("cave_throne", 0, (88, 104))], "base"),
]
TEXTURES = [("t1_chao_caverna.png", "floor_cave", (32, 32)),
            ("t2_parede_caverna.png", "wall_cave", (64, 32)),
            ("t3_topo_parede.png", "wall_cave_top", (64, 32))]


def figure_box(cell):
    """Caixa da figura na celula: maior componente + pedacos soltos proximos (ignora lascas na borda)."""
    a = np.asarray(cell.getchannel("A")) >= 100
    n, lab, st, _ = cv2.connectedComponentsWithStats(a.astype(np.uint8), 8)
    if n <= 1:
        return None
    H, W = a.shape
    bigi = max(range(1, n), key=lambda i: st[i, cv2.CC_STAT_AREA])
    big = st[bigi, cv2.CC_STAT_AREA]
    x0, y0, x1, y1 = W, H, 0, 0
    keep = np.zeros_like(a)
    for i in range(1, n):
        x, y, w, h, area = st[i]
        edge = x <= 1 or y <= 1 or x + w >= W - 1 or y + h >= H - 1
        if area < big * 0.002 or (edge and i != bigi):   # lasca da celula vizinha
            continue
        keep |= lab == i
        x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x + w), max(y1, y + h)
    return (x0, y0, x1, y1), keep


def cells(img, grid):
    cw, ch = img.width // grid[0], img.height // grid[1]
    return [img.crop((c * cw, r * ch, (c + 1) * cw, (r + 1) * ch)) for r in range(grid[1]) for c in range(grid[0])]


def finish(fig, size, mode):
    W, H = size
    s = min(W / fig.width, H / fig.height)
    w, h = max(1, round(fig.width * s)), max(1, round(fig.height * s))
    # premultiplica antes de reduzir (sem halo escuro/claro na borda)
    arr = np.asarray(fig).astype(np.float32)
    arr[..., :3] *= arr[..., 3:] / 255
    small = Image.fromarray(arr.astype(np.uint8), "RGBA").resize((w, h), Image.Resampling.LANCZOS)
    sa = np.asarray(small).astype(np.float32)
    al = sa[..., 3:] / 255
    sa[..., :3] = np.where(al > 0, sa[..., :3] / np.maximum(al, 1e-3), 0)
    rgb = Image.fromarray(np.clip(sa[..., :3], 0, 255).astype(np.uint8), "RGB")
    rgb = rgb.filter(ImageFilter.UnsharpMask(radius=1.0, percent=70, threshold=2))
    out = np.dstack([np.asarray(rgb), np.where(sa[..., 3] >= 128, 255, 0).astype(np.uint8)])
    out[out[..., 3] == 0, :3] = 0
    fig2 = Image.fromarray(out, "RGBA")
    canvas = Image.new("RGBA", (W, H))
    y = (H - h) // 2 if mode == "decal" else H - h
    canvas.alpha_composite(fig2, ((W - w) // 2, y))
    return canvas


def main():
    for fname, grid, items, mode in SHEETS:
        img = Image.open(SRC / fname).convert("RGBA")
        cs = cells(img, grid)
        boxes = {}
        for name, idx, size in items:
            boxes[name] = figure_box(cs[idx])
        if mode == "anim":   # um recorte so (uniao) para todos os quadros
            xs = [b[0] for b in boxes.values() if b]
            union = (min(b[0] for b in xs), min(b[1] for b in xs), max(b[2] for b in xs), max(b[3] for b in xs))
        for name, idx, size in items:
            if not boxes[name]:
                print("  vazio:", fname, idx); continue
            box, keep = boxes[name]
            cell = cs[idx].copy()
            a = np.asarray(cell).copy()
            a[~keep, 3] = 0
            cell = Image.fromarray(a, "RGBA")
            fig = cell.crop(union if mode == "anim" else box)
            finish(fig, size, mode).save(OUT / f"scene_{name}.rgba16.png", optimize=True)
        print(f"{fname}: {len(items)} sprites")
    for fname, name, size in TEXTURES:
        img = Image.open(SRC / fname).convert("RGB")
        small = img.resize(size, Image.Resampling.LANCZOS)        # imagem inteira: continua sem emenda
        small = small.filter(ImageFilter.UnsharpMask(radius=0.8, percent=60, threshold=1))
        Image.fromarray(np.dstack([np.asarray(small), np.full(size[::-1], 255, np.uint8)]), "RGBA") \
            .save(OUT / f"scene_{name}.rgba16.png", optimize=True)
        print(f"{fname}: textura {size}")


if __name__ == "__main__":
    main()
