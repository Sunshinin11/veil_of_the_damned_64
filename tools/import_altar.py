"""Importa as pranchetas do Altar Negro (mapa 3) e dos chefes:
  Sprites/Altar_Negro/a*.png, t*.png -> assets/scenery/scene_hell_*.rgba16.png (+ chao/parede/topo/lava)
  Sprites/Altar_Negro/m*.png (Malakor) e v*.png (Varkhul), grades 4x4 (linhas = frente, diagonal,
  lado esquerdo, costas; colunas = 4 quadros) -> assets/sprites/spr_malakor_hd_*/spr_varkhul_* (144x144).
Os chefes usam UMA escala por chefe (mesmo tamanho em todas as acoes), pes encostados embaixo e o
tronco centralizado. Uso:  python tools/import_altar.py
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter
import cv2
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_cripta_caverna import figure_box, cells, finish

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "Altar_Negro"
SCN = ROOT / "assets" / "scenery"
SPR = ROOT / "assets" / "sprites"

SHEETS = [
    ("a1_selos.png", (2, 1), [("hell_seal_on", 0, (56, 112)), ("hell_seal_off", 1, (56, 112))], "base"),
    ("a2_rochas.png", (2, 2), [("hell_rocks_a", 0, (64, 40)), ("hell_rocks_b", 1, (64, 40)),
                               ("hell_rocks_c", 2, (64, 40)), ("hell_rocks_d", 3, (64, 40))], "base"),
    ("a2b_espinhos.png", (3, 1), [("hell_spikes_a", 0, (56, 104)), ("hell_spikes_b", 1, (56, 104)),
                                  ("hell_spikes_c", 2, (56, 104))], "base"),
    ("a2c_fogo.png", (3, 1), [("hell_firevent_f0", 0, (64, 96)), ("hell_firevent_f1", 1, (64, 96)),
                              ("hell_firevent_f2", 2, (64, 96))], "anim"),
    ("a3_braseiro.png", (3, 1), [("hell_brazier_f0", 0, (64, 80)), ("hell_brazier_f1", 1, (64, 80)),
                                 ("hell_brazier_f2", 2, (64, 80))], "anim"),
    ("a4_decoracao.png", (2, 2), [("hell_hanged", 0, (48, 104)), ("hell_skulls", 1, (40, 104)),
                                  ("hell_cage", 2, (56, 96)), ("hell_statue", 3, (64, 96))], "base"),
    ("a5_trono_frente.png", (1, 1), [("hell_throne", 0, (112, 120))], "base"),
    ("a6_decals.png", (3, 2), [("hell_decal_lava", 0, (64, 32)), ("hell_decal_crack", 1, (64, 32)),
                               ("hell_decal_ritual", 2, (64, 32)), ("hell_decal_bones", 3, (64, 32)),
                               ("hell_decal_ash", 4, (64, 32)), ("hell_decal_blood", 5, (64, 32))], "decal"),
    ("a7_portal.png", (3, 1), [("hell_portal_f0", 0, (96, 96)), ("hell_portal_f1", 1, (96, 96)),
                               ("hell_portal_f2", 2, (96, 96))], "anim"),
]
TEXTURES = [("t1_chao_altar.png", "floor_hell", (32, 32)),
            ("t2_parede_altar.png", "wall_hell", (64, 32)),
            ("t3_topo_altar.png", "wall_hell_top", (64, 32)),
            ("t4_lava.png", "lava_hell", (32, 32))]
BOSSES = {
    "malakor_hd": {"idle": "m1_malakor_parado", "walk": "m2_malakor_andando", "attack": "m3_malakor_ataque",
                   "hurt": "m4_malakor_dano", "death": "m5_malakor_morte", "enrage": "m6_malakor_furia"},
    "varkhul": {"idle": "v1_varkhul_parado", "walk": "v2_varkhul_andando", "attack": "v3_varkhul_ataque_foice",
                "hurt": "v4_varkhul_dano", "death": "v5_varkhul_morte", "enrage": "v6_varkhul_furia"},
}
BOSS_CANVAS = 144


def scenery():
    for fname, grid, items, mode in SHEETS:
        img = Image.open(SRC / fname).convert("RGBA")
        cs = cells(img, grid)
        boxes = {name: figure_box(cs[idx]) for name, idx, size in items}
        if mode == "anim":
            xs = [b[0] for b in boxes.values() if b]
            union = (min(b[0] for b in xs), min(b[1] for b in xs), max(b[2] for b in xs), max(b[3] for b in xs))
        for name, idx, size in items:
            box, keep = boxes[name]
            a = np.asarray(cs[idx]).copy()
            a[~keep, 3] = 0
            fig = Image.fromarray(a, "RGBA").crop(union if mode == "anim" else box)
            finish(fig, size, mode).save(SCN / f"scene_{name}.rgba16.png", optimize=True)
        print(f"{fname}: {len(items)}")
    for fname, name, size in TEXTURES:
        img = Image.open(SRC / fname).convert("RGB")
        if name == "floor_hell":
            # Chao NEUTRO (pedido): basalto cinza-escuro sem os veios laranja; o vermelho vem do filtro de tela.
            g = np.asarray(img.convert("L")).astype(np.float32)
            g = np.clip((g - 20) * 0.75 + 34, 0, 255)
            img = Image.fromarray(np.dstack([g * 1.02, g * 0.97, g * 0.94]).clip(0, 255).astype(np.uint8), "RGB")
        small = img.resize(size, Image.Resampling.LANCZOS).filter(ImageFilter.UnsharpMask(0.8, 60, 1))
        Image.fromarray(np.dstack([np.asarray(small), np.full(size[::-1], 255, np.uint8)]), "RGBA") \
            .save(SCN / f"scene_{name}.rgba16.png", optimize=True)
    # Lago de lava: a textura de lava recortada num formato irregular (decal grande, 96x48)
    lava = Image.open(SRC / "t4_lava.png").convert("RGB").resize((96, 48), Image.Resampling.LANCZOS)
    yy, xx = np.mgrid[0:48, 0:96]
    rng = np.random.default_rng(7)
    ang = np.arctan2((yy - 24) / 24.0, (xx - 48) / 48.0)
    r = np.hypot((xx - 48) / 46.0, (yy - 24) / 22.0)
    edge = 0.82 + 0.10 * np.sin(ang * 3 + 1.0) + 0.06 * np.sin(ang * 7 + 2.0)
    mask = r < edge
    rgb = np.asarray(lava).copy()
    rim = (r > edge - 0.12) & mask
    rgb[rim] = (rgb[rim] * 0.35).astype(np.uint8)                       # crosta escura na borda
    out = np.dstack([rgb, np.where(mask, 255, 0).astype(np.uint8)])
    out[~mask, :3] = 0
    Image.fromarray(out, "RGBA").save(SCN / "scene_hell_lava_pool.rgba16.png", optimize=True)
    print("texturas + lago de lava ok")


def boss_cells(path):
    img = Image.open(path).convert("RGBA")
    cs = cells(img, (4, 4))
    out = []
    for i, c in enumerate(cs):
        fb = figure_box(c)
        if not fb:
            out.append(None); continue
        box, keep = fb
        a = np.asarray(c).copy()
        a[~keep, 3] = 0
        fig = Image.fromarray(a, "RGBA").crop(box)
        al = np.asarray(fig.getchannel("A")) >= 100
        h = al.shape[0]
        band = al[int(.30 * h):int(.60 * h)]
        cx = float(np.nonzero(band)[1].mean()) if band.any() else fig.width / 2
        out.append((fig, cx))
    return out


def bosses():
    for boss, acts in BOSSES.items():
        sheets = {act: boss_cells(SRC / f"{fn}.png") for act, fn in acts.items()}
        # escala unica: a maior figura (altura e meia-largura em torno do tronco) cabe no quadro
        lim = BOSS_CANVAS - 6
        s = 10.0
        for figs in sheets.values():
            for f in figs:
                if not f: continue
                fig, cx = f
                half = max(cx, fig.width - cx)
                s = min(s, lim / fig.height, (BOSS_CANVAS / 2 - 3) / half)
        for act, figs in sheets.items():
            for i, f in enumerate(figs):
                d, fr = i // 4, i % 4
                canvas = Image.new("RGBA", (BOSS_CANVAS, BOSS_CANVAS))
                if f:
                    fig, cx = f
                    w, h = max(1, round(fig.width * s)), max(1, round(fig.height * s))
                    small = finish(fig, (w, h), "base")
                    canvas.alpha_composite(small, (round(BOSS_CANVAS / 2 - cx * s), BOSS_CANVAS - 3 - h))
                canvas.save(SPR / f"spr_{boss}_{act}_d{d}_f{fr}.rgba16.png", optimize=True)
        print(f"{boss}: escala {s:.3f}")


if __name__ == "__main__":
    scenery()
    bosses()
