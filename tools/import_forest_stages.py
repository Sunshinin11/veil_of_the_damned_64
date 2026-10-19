"""Importa o cenario das fases 2 (Pantano Podre) e 3 (Coracao Seco) da campanha da Floresta Morta.
  Sprites/Pantano/p1..p6, t1..t3       -> assets/scenery/scene_swamp_*.rgba16.png, scene_forest_portal_f*, texturas
  Sprites/Coracao_Seco/c1..c7, t4..t7  -> assets/scenery/scene_heart_*.rgba16.png, texturas e poca de seiva
Enquanto uma prancheta nao existe, gera um PROVISORIO a partir da arte que ja temos (pintada com outra cor),
assim a ROM nunca fica sem arquivo. Uso:  python tools/import_forest_stages.py
"""
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageFilter
sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_cripta_caverna import figure_box, cells, finish

ROOT = Path(__file__).resolve().parents[1]
SCN = ROOT / "assets" / "scenery"
SW = ROOT / "Sprites" / "Pantano"
HE = ROOT / "Sprites" / "Coracao_Seco"

# (pasta, arquivo, grade, [(nome, indice, tamanho)], modo, provisorios {nome: (arquivo existente, tinta rgb)})
TEAL, MUD, AMBER, ROOT_C, SICK = (0.62, 0.86, 0.80), (0.80, 0.78, 0.66), (1.10, 0.86, 0.52), (0.86, 0.70, 0.52), (0.90, 1.05, 0.45)
SHEETS = [
    (SW, "p1_arvores.png", (3, 1), [("swamp_tree_a", 0, (64, 112)), ("swamp_tree_b", 1, (64, 112)), ("swamp_tree_c", 2, (64, 112))], "base",
     {"swamp_tree_a": ("scene_tree_dead", TEAL), "swamp_tree_b": ("scene_tree_dead", TEAL), "swamp_tree_c": ("scene_tree_dead", TEAL)}),
    (SW, "p2_plantas.png", (2, 2), [("swamp_reeds_a", 0, (48, 40)), ("swamp_reeds_b", 1, (48, 40)), ("swamp_shroom", 2, (40, 32)), ("swamp_log", 3, (64, 40))], "base",
     {"swamp_reeds_a": ("scene_dead_bush", TEAL), "swamp_reeds_b": ("scene_dead_bush", MUD), "swamp_shroom": ("scene_cave_mush_a", TEAL), "swamp_log": ("scene_cave_rocks_a", MUD)}),
    (SW, "p3_decoracao.png", (2, 2), [("swamp_boat", 0, (80, 56)), ("swamp_totem", 1, (48, 96)), ("swamp_hut", 2, (96, 96)), ("swamp_bones", 3, (48, 32))], "base",
     {"swamp_boat": ("scene_cave_sarco_a", MUD), "swamp_totem": ("scene_tomb_statue", TEAL), "swamp_hut": ("scene_crypt_entrance", MUD), "swamp_bones": ("scene_bone_pile", MUD)}),
    (SW, "p4_lanterna.png", (3, 1), [("swamp_lantern_f0", 0, (40, 80)), ("swamp_lantern_f1", 1, (40, 80)), ("swamp_lantern_f2", 2, (40, 80))], "anim",
     {"swamp_lantern_f0": ("scene_cave_brazier_f0", TEAL), "swamp_lantern_f1": ("scene_cave_brazier_f1", TEAL), "swamp_lantern_f2": ("scene_cave_brazier_f2", TEAL)}),
    (SW, "p5_decals.png", (3, 2), [("swamp_decal_pool", 0, (96, 48)), ("swamp_decal_pool_s", 1, (64, 32)), ("swamp_decal_mud", 2, (64, 32)),
                                    ("swamp_decal_lily", 3, (64, 32)), ("swamp_decal_algae", 4, (64, 32)), ("swamp_decal_bones", 5, (64, 32))], "decal",
     {"swamp_decal_pool": ("scene_cave_decal_water", TEAL), "swamp_decal_pool_s": ("scene_cave_decal_water", TEAL), "swamp_decal_mud": ("scene_decal_cracks", MUD),
      "swamp_decal_lily": ("scene_cave_decal_web", TEAL), "swamp_decal_algae": ("scene_cave_decal_blood", (0.35, 0.85, 0.45)), "swamp_decal_bones": ("scene_decal_bones", MUD)}),
    (SW, "p6_portal.png", (3, 1), [("forest_portal_f0", 0, (96, 96)), ("forest_portal_f1", 1, (96, 96)), ("forest_portal_f2", 2, (96, 96))], "anim",
     {"forest_portal_f0": ("scene_cave_portal_f0", (0.80, 1.0, 0.75)), "forest_portal_f1": ("scene_cave_portal_f1", (0.80, 1.0, 0.75)), "forest_portal_f2": ("scene_cave_portal_f2", (0.80, 1.0, 0.75))}),
    (HE, "c1_totens.png", (2, 1), [("heart_totem_on", 0, (56, 112)), ("heart_totem_off", 1, (56, 112))], "base",
     {"heart_totem_on": ("scene_hell_seal_on", SICK), "heart_totem_off": ("scene_hell_seal_off", ROOT_C)}),
    (HE, "c2_raizes.png", (3, 2), [("heart_roots_a", 0, (80, 72)), ("heart_roots_b", 1, (80, 72)), ("heart_roots_c", 2, (80, 72)),
                                    ("heart_thorns_a", 3, (56, 72)), ("heart_thorns_b", 4, (56, 72)), ("heart_thorns_c", 5, (56, 72))], "base",
     {"heart_roots_a": ("scene_hell_rocks_a", ROOT_C), "heart_roots_b": ("scene_hell_rocks_b", ROOT_C), "heart_roots_c": ("scene_hell_rocks_c", ROOT_C),
      "heart_thorns_a": ("scene_hell_spikes_a", ROOT_C), "heart_thorns_b": ("scene_hell_spikes_b", ROOT_C), "heart_thorns_c": ("scene_hell_spikes_c", ROOT_C)}),
    (HE, "c3_fungos.png", (3, 1), [("heart_fungus_f0", 0, (64, 64)), ("heart_fungus_f1", 1, (64, 64)), ("heart_fungus_f2", 2, (64, 64))], "anim",
     {"heart_fungus_f0": ("scene_hell_brazier_f0", SICK), "heart_fungus_f1": ("scene_hell_brazier_f1", SICK), "heart_fungus_f2": ("scene_hell_brazier_f2", SICK)}),
    (HE, "c4_decoracao.png", (2, 2), [("heart_cocoon", 0, (48, 104)), ("heart_skulls", 1, (48, 96)), ("heart_stone", 2, (48, 96)), ("heart_carcass", 3, (64, 72))], "base",
     {"heart_cocoon": ("scene_hell_hanged", ROOT_C), "heart_skulls": ("scene_hell_skulls", ROOT_C), "heart_stone": ("scene_hell_statue", MUD), "heart_carcass": ("scene_cave_ribcage", ROOT_C)}),
    (HE, "c5_trono.png", (1, 1), [("heart_throne", 0, (112, 120))], "base", {"heart_throne": ("scene_hell_throne", ROOT_C)}),
    (HE, "c6_esporos.png", (3, 1), [("heart_spore_f0", 0, (64, 96)), ("heart_spore_f1", 1, (64, 96)), ("heart_spore_f2", 2, (64, 96))], "anim",
     {"heart_spore_f0": ("scene_hell_firevent_f0", SICK), "heart_spore_f1": ("scene_hell_firevent_f1", SICK), "heart_spore_f2": ("scene_hell_firevent_f2", SICK)}),
    (HE, "c7_decals.png", (3, 2), [("heart_decal_poison", 0, (64, 32)), ("heart_decal_crack", 1, (64, 32)), ("heart_decal_circle", 2, (64, 32)),
                                    ("heart_decal_leaves", 3, (64, 32)), ("heart_decal_bones", 4, (64, 32)), ("heart_decal_sap", 5, (64, 32))], "decal",
     {"heart_decal_poison": ("scene_hell_decal_lava", SICK), "heart_decal_crack": ("scene_hell_decal_crack", AMBER), "heart_decal_circle": ("scene_hell_decal_ritual", SICK),
      "heart_decal_leaves": ("scene_hell_decal_ash", AMBER), "heart_decal_bones": ("scene_hell_decal_bones", AMBER), "heart_decal_sap": ("scene_hell_decal_blood", (0.45, 0.55, 0.25))}),
]
TEXTURES = [  # (pasta, arquivo, nome, tamanho, provisorio)
    (SW, "t1_chao_pantano.png", "floor_swamp", (32, 32), ("scene_floor_cave", TEAL)),
    (SW, "t2_parede_pantano.png", "wall_swamp", (64, 32), ("scene_wall_cave", TEAL)),
    (SW, "t3_topo_pantano.png", "wall_swamp_top", (64, 32), ("scene_wall_cave_top", TEAL)),
    (HE, "t4_chao_coracao.png", "floor_heart", (32, 32), ("scene_floor_hell", AMBER)),
    (HE, "t5_parede_coracao.png", "wall_heart", (64, 32), ("scene_wall_hell", ROOT_C)),
    (HE, "t6_topo_coracao.png", "wall_heart_top", (64, 32), ("scene_wall_hell_top", ROOT_C)),
]


def tinted(src, tint):
    im = Image.open(SCN / f"{src}.rgba16.png").convert("RGBA")
    a = np.asarray(im).astype(np.float32)
    for k in range(3): a[..., k] *= tint[k]
    return Image.fromarray(a.clip(0, 255).astype(np.uint8), "RGBA")


def placeholder(name, src, tint, size=None):
    im = tinted(src, tint)
    if size and im.size != size:
        im = im.resize(size, Image.Resampling.NEAREST)
    im.save(SCN / f"scene_{name}.rgba16.png", optimize=True)


def scenery():
    real = fake = 0
    for folder, fname, grid, items, mode, fallback in SHEETS:
        p = folder / fname
        if not p.exists():
            for name, idx, size in items:
                src, tint = fallback[name]
                placeholder(name, src, tint, size if mode == "decal" else None)
                fake += 1
            continue
        cs = cells(Image.open(p).convert("RGBA"), grid)
        boxes = {name: figure_box(cs[idx]) for name, idx, size in items}
        union = None
        if mode == "anim":
            xs = [b[0] for b in boxes.values() if b]
            union = (min(b[0] for b in xs), min(b[1] for b in xs), max(b[2] for b in xs), max(b[3] for b in xs))
        for name, idx, size in items:
            if not boxes[name]:
                continue
            box, keep = boxes[name]
            a = np.asarray(cs[idx]).copy()
            a[~keep, 3] = 0
            fig = Image.fromarray(a, "RGBA").crop(union if mode == "anim" else box)
            finish(fig, size, mode).save(SCN / f"scene_{name}.rgba16.png", optimize=True)
            real += 1
    for folder, fname, name, size, (src, tint) in TEXTURES:
        p = folder / fname
        if p.exists():
            img = Image.open(p).convert("RGB")
            small = img.resize(size, Image.Resampling.LANCZOS).filter(ImageFilter.UnsharpMask(0.8, 60, 1))
            Image.fromarray(np.dstack([np.asarray(small), np.full(size[::-1], 255, np.uint8)]), "RGBA") \
                .save(SCN / f"scene_{name}.rgba16.png", optimize=True)
            real += 1
        else:
            placeholder(name, src, tint)
            fake += 1
    # Poca de seiva venenosa (decal grande, 96x48): textura de seiva recortada num formato irregular
    t7 = HE / "t7_seiva.png"
    if t7.exists():
        sap = Image.open(t7).convert("RGB").resize((96, 48), Image.Resampling.LANCZOS)
        rgb = np.asarray(sap).copy()
    else:
        lava = np.asarray(Image.open(SCN / "scene_hell_lava_pool.rgba16.png").convert("RGBA")).astype(np.float32)
        rgb = np.dstack([lava[..., 1] * 0.75, lava[..., 0] * 0.85, lava[..., 2] * 0.3]).clip(0, 255).astype(np.uint8)
        if rgb.shape[:2] != (48, 96):
            rgb = np.asarray(Image.fromarray(rgb).resize((96, 48)))
    yy, xx = np.mgrid[0:48, 0:96]
    ang = np.arctan2((yy - 24) / 24.0, (xx - 48) / 48.0)
    r = np.hypot((xx - 48) / 46.0, (yy - 24) / 22.0)
    edge = 0.82 + 0.10 * np.sin(ang * 3 + 2.0) + 0.06 * np.sin(ang * 7 + 1.0)
    mask = r < edge
    rim = (r > edge - 0.12) & mask
    rgb = rgb.copy(); rgb[rim] = (rgb[rim] * 0.35).astype(np.uint8)
    out = np.dstack([rgb, np.where(mask, 255, 0).astype(np.uint8)]); out[~mask, :3] = 0
    Image.fromarray(out, "RGBA").save(SCN / "scene_heart_poison_pool.rgba16.png", optimize=True)
    print(f"floresta fases 2/3: {real} da arte nova, {fake} provisorios")


if __name__ == "__main__":
    scenery()
