"""Importa o cenario da campanha O Norte Esquecido (Barbaro):
  Sprites/Norte_Passo/n1..n6, t1..t3     -> scene_snow_*, scene_ice_portal_f*, texturas da neve
  Sprites/Norte_Caverna/g1..g5, t4..t6   -> scene_ice_*, texturas do gelo
  Sprites/Trono_Jarl/j1..j7, t7..t10     -> scene_frost_*, texturas do salao e a poca de agua gelada
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
NP = ROOT / "Sprites" / "Norte_Passo"
NC = ROOT / "Sprites" / "Norte_Caverna"
TJ = ROOT / "Sprites" / "Trono_Jarl"

ICE_C, SNOW_C, STEEL, PALE, WARM = (0.78, 0.92, 1.18), (1.15, 1.18, 1.25), (0.70, 0.80, 0.98), (0.95, 1.0, 1.10), (1.0, 0.95, 0.9)
def anim3(prefix, src, tint):
    return {f"{prefix}_f{k}": (f"{src}_f{k}", tint) for k in range(3)}
SHEETS = [
    (NP, "n1_pinheiros.png", (3, 1), [("snow_pine_a", 0, (64, 112)), ("snow_pine_b", 1, (64, 112)), ("snow_pine_c", 2, (64, 112))], "base",
     {"snow_pine_a": ("scene_tree_dead", SNOW_C), "snow_pine_b": ("scene_tree_dead", PALE), "snow_pine_c": ("scene_great_tree", SNOW_C)}),
    (NP, "n2_rochas.png", (2, 2), [("snow_rock_a", 0, (64, 56)), ("snow_rock_b", 1, (56, 40)), ("snow_runestone", 2, (48, 80)), ("snow_cart", 3, (80, 56))], "base",
     {"snow_rock_a": ("scene_cave_rocks_a", SNOW_C), "snow_rock_b": ("scene_cave_rocks_b", SNOW_C), "snow_runestone": ("scene_tomb_statue", STEEL), "snow_cart": ("scene_cave_sarco_a", WARM)}),
    (NP, "n3_decoracao.png", (2, 2), [("snow_corpse", 0, (64, 40)), ("snow_banner", 1, (40, 96)), ("snow_palisade", 2, (64, 64)), ("snow_bones", 3, (64, 48))], "base",
     {"snow_corpse": ("scene_traveler_body", ICE_C), "snow_banner": ("scene_path_shrine", WARM), "snow_palisade": ("scene_grave_fence", WARM), "snow_bones": ("scene_bone_pile", PALE)}),
    (NP, "n4_fogueira.png", (3, 1), [("snow_fire_f0", 0, (48, 56)), ("snow_fire_f1", 1, (48, 56)), ("snow_fire_f2", 2, (48, 56))], "anim",
     anim3("snow_fire", "scene_campfire", WARM)),
    (NP, "n5_decals.png", (3, 2), [("snow_decal_ice", 0, (96, 48)), ("snow_decal_drift", 1, (64, 32)), ("snow_decal_blood", 2, (64, 32)),
                                    ("snow_decal_tracks", 3, (64, 32)), ("snow_decal_frozen", 4, (64, 32)), ("snow_decal_bones", 5, (64, 32))], "decal",
     {"snow_decal_ice": ("scene_cave_decal_water", ICE_C), "snow_decal_drift": ("scene_decal_cracks", SNOW_C), "snow_decal_blood": ("scene_decal_blood", WARM),
      "snow_decal_tracks": ("scene_decal_cracks", PALE), "snow_decal_frozen": ("scene_cave_decal_water", PALE), "snow_decal_bones": ("scene_decal_bones", PALE)}),
    (NP, "n6_portal.png", (3, 1), [("ice_portal_f0", 0, (96, 96)), ("ice_portal_f1", 1, (96, 96)), ("ice_portal_f2", 2, (96, 96))], "anim",
     anim3("ice_portal", "scene_cave_portal", ICE_C)),
    (NC, "g1_estalagmites.png", (3, 1), [("ice_stalag_a", 0, (56, 104)), ("ice_stalag_b", 1, (56, 104)), ("ice_stalag_c", 2, (56, 104))], "base",
     {"ice_stalag_a": ("scene_cave_stalag_a", ICE_C), "ice_stalag_b": ("scene_cave_stalag_b", ICE_C), "ice_stalag_c": ("scene_cave_stalag_c", ICE_C)}),
    (NC, "g2_cristais.png", (3, 1), [("ice_crystal_a", 0, (48, 56)), ("ice_crystal_b", 1, (48, 56)), ("ice_crystal_c", 2, (48, 56))], "base",
     {"ice_crystal_a": ("scene_cave_crystal_a", PALE), "ice_crystal_b": ("scene_cave_crystal_b", PALE), "ice_crystal_c": ("scene_cave_crystal_c", PALE)}),
    (NC, "g3_decoracao.png", (2, 2), [("ice_frozen_warrior", 0, (48, 96)), ("ice_frozen_wolf", 1, (72, 64)), ("ice_mammoth", 2, (96, 72)), ("ice_bones", 3, (48, 32))], "base",
     {"ice_frozen_warrior": ("scene_cave_statue_a", ICE_C), "ice_frozen_wolf": ("scene_cave_statue_b", ICE_C), "ice_mammoth": ("scene_cave_ribcage", PALE), "ice_bones": ("scene_cave_bones_a", PALE)}),
    (NC, "g4_braseiro.png", (3, 1), [("ice_brazier_f0", 0, (40, 72)), ("ice_brazier_f1", 1, (40, 72)), ("ice_brazier_f2", 2, (40, 72))], "anim",
     anim3("ice_brazier", "scene_cave_brazier", (0.55, 0.85, 1.35))),
    (NC, "g5_decals.png", (3, 2), [("ice_decal_crack", 0, (64, 32)), ("ice_decal_frozen", 1, (64, 32)), ("ice_decal_glow", 2, (64, 32)),
                                    ("ice_decal_bones", 3, (64, 32)), ("ice_decal_snow", 4, (64, 32)), ("ice_decal_blood", 5, (64, 32))], "decal",
     {"ice_decal_crack": ("scene_cave_decal_cracks", ICE_C), "ice_decal_frozen": ("scene_cave_decal_water", ICE_C), "ice_decal_glow": ("scene_cave_decal_water", PALE),
      "ice_decal_bones": ("scene_cave_decal_bones", PALE), "ice_decal_snow": ("scene_cave_decal_grave", SNOW_C), "ice_decal_blood": ("scene_cave_decal_blood", STEEL)}),
    (TJ, "j1_pedras_runicas.png", (2, 1), [("frost_seal_on", 0, (56, 112)), ("frost_seal_off", 1, (56, 112))], "base",
     {"frost_seal_on": ("scene_hell_seal_on", (0.45, 0.80, 1.40)), "frost_seal_off": ("scene_hell_seal_off", STEEL)}),
    (TJ, "j2_gelo_rochas.png", (3, 2), [("frost_spikes_a", 0, (56, 80)), ("frost_spikes_b", 1, (56, 80)), ("frost_spikes_c", 2, (56, 80)),
                                         ("frost_rocks_a", 3, (64, 48)), ("frost_rocks_b", 4, (64, 48)), ("frost_rocks_c", 5, (64, 48))], "base",
     {"frost_spikes_a": ("scene_hell_spikes_a", ICE_C), "frost_spikes_b": ("scene_hell_spikes_b", ICE_C), "frost_spikes_c": ("scene_hell_spikes_c", ICE_C),
      "frost_rocks_a": ("scene_hell_rocks_a", STEEL), "frost_rocks_b": ("scene_hell_rocks_b", STEEL), "frost_rocks_c": ("scene_hell_rocks_c", STEEL)}),
    (TJ, "j3_geiser.png", (3, 1), [("frost_vent_f0", 0, (64, 96)), ("frost_vent_f1", 1, (64, 96)), ("frost_vent_f2", 2, (64, 96))], "anim",
     anim3("frost_vent", "scene_hell_firevent", (0.50, 0.85, 1.50))),
    (TJ, "j4_decoracao.png", (2, 2), [("frost_hanged", 0, (48, 104)), ("frost_skulls", 1, (40, 104)), ("frost_statue", 2, (64, 96)), ("frost_cage", 3, (56, 96))], "base",
     {"frost_hanged": ("scene_hell_hanged", ICE_C), "frost_skulls": ("scene_hell_skulls", PALE), "frost_statue": ("scene_hell_statue", STEEL), "frost_cage": ("scene_hell_cage", ICE_C)}),
    (TJ, "j5_trono.png", (1, 1), [("frost_throne", 0, (112, 120))], "base", {"frost_throne": ("scene_hell_throne", STEEL)}),
    (TJ, "j6_decals.png", (3, 2), [("frost_decal_ice", 0, (64, 32)), ("frost_decal_crack", 1, (64, 32)), ("frost_decal_circle", 2, (64, 32)),
                                    ("frost_decal_bones", 3, (64, 32)), ("frost_decal_snow", 4, (64, 32)), ("frost_decal_blood", 5, (64, 32))], "decal",
     {"frost_decal_ice": ("scene_hell_decal_lava", (0.30, 0.60, 1.40)), "frost_decal_crack": ("scene_hell_decal_crack", STEEL), "frost_decal_circle": ("scene_hell_decal_ritual", (0.35, 0.70, 1.40)),
      "frost_decal_bones": ("scene_hell_decal_bones", PALE), "frost_decal_snow": ("scene_hell_decal_ash", SNOW_C), "frost_decal_blood": ("scene_hell_decal_blood", STEEL)}),
    (TJ, "j7_braseiro.png", (3, 1), [("frost_brazier_f0", 0, (40, 80)), ("frost_brazier_f1", 1, (40, 80)), ("frost_brazier_f2", 2, (40, 80))], "anim",
     anim3("frost_brazier", "scene_hell_brazier", (0.45, 0.80, 1.50))),
]
TEXTURES = [
    (NP, "t1_chao_neve.png", "floor_snow", (32, 32), ("scene_floor_moss", SNOW_C)),
    (NP, "t2_parede_penhasco.png", "wall_snow", (64, 32), ("scene_wall_path", SNOW_C)),
    (NP, "t3_topo_penhasco.png", "wall_snow_top", (64, 32), ("scene_wall_path_top", SNOW_C)),
    (NC, "t4_chao_gelo.png", "floor_ice", (32, 32), ("scene_floor_cave", ICE_C)),
    (NC, "t5_parede_gelo.png", "wall_ice", (64, 32), ("scene_wall_cave", ICE_C)),
    (NC, "t6_topo_gelo.png", "wall_ice_top", (64, 32), ("scene_wall_cave_top", ICE_C)),
    (TJ, "t7_chao_salao.png", "floor_frost", (32, 32), ("scene_floor_hell", STEEL)),
    (TJ, "t8_parede_fortaleza.png", "wall_frost", (64, 32), ("scene_wall_hell", STEEL)),
    (TJ, "t9_topo_fortaleza.png", "wall_frost_top", (64, 32), ("scene_wall_hell_top", STEEL)),
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
    # Poca de agua gelada (decal grande, 96x48)
    t10 = TJ / "t10_agua_gelada.png"
    if t10.exists():
        rgb = np.asarray(Image.open(t10).convert("RGB").resize((96, 48), Image.Resampling.LANCZOS)).copy()
    else:
        lava = np.asarray(Image.open(SCN / "scene_hell_lava_pool.rgba16.png").convert("RGBA").resize((96, 48))).astype(np.float32)
        lum = lava[..., :3].mean(-1)
        rgb = np.dstack([lum * 0.25, lum * 0.55, lum * 0.95 + 30]).clip(0, 255).astype(np.uint8)
    yy, xx = np.mgrid[0:48, 0:96]
    ang = np.arctan2((yy - 24) / 24.0, (xx - 48) / 48.0)
    r = np.hypot((xx - 48) / 46.0, (yy - 24) / 22.0)
    edge = 0.82 + 0.10 * np.sin(ang * 3 + 0.5) + 0.06 * np.sin(ang * 7 + 3.0)
    mask = r < edge
    rim = (r > edge - 0.14) & mask
    rgb = rgb.copy(); rgb[rim] = np.clip(rgb[rim].astype(np.float32) * 0.5 + 110, 0, 255).astype(np.uint8)   # borda de gelo clara
    out = np.dstack([rgb, np.where(mask, 255, 0).astype(np.uint8)]); out[~mask, :3] = 0
    Image.fromarray(out, "RGBA").save(SCN / "scene_frost_pool.rgba16.png", optimize=True)
    print(f"norte: {real} da arte nova, {fake} provisorios")


if __name__ == "__main__":
    scenery()
