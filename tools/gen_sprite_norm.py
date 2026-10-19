"""Gera include/sprite_norm.h: fator de escala por (animacao, direcao) para o druida e o lobisomem
terem o MESMO tamanho aparente em todas as animacoes e direcoes.

Mede a altura da cabeca (do topo do capuz/cabeca ate os pes, sem o cajado) do quadro mais ereto de
cada animacao/direcao e calcula alvo/medido. Os quadros de corrida novos ja saem normalizados do
export_druid_walk_left.py (fator 1). Ataque e morte ficam de fora: tem poses deitadas/estendidas.
Rode depois de tools/build_sprite_assets.py:   python tools/gen_sprite_norm.py
"""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SPR = ROOT / "assets" / "sprites"
TARGET = {"druid": 49, "werewolf": 50, "hunt": 51}
ANIMS = ["druid_run", "druid_run_alt", "druid_idle", "druid_cast", "druid_hurt", "druid_transform", "druid_escape",
         "werewolf_idle", "werewolf_walk", "werewolf_run", "werewolf_cast",
         "werewolf_hurt", "werewolf_escape",
         "hunt_idle", "hunt_run", "hunt_attack", "hunt_hurt", "hunt_death", "hunt_roll"]


def head(path):
    im = Image.open(path).convert("RGBA")
    a = np.asarray(im.getchannel("A")) > 0
    rows = a.sum(axis=1)
    ys = [y for y in range(a.shape[0]) if rows[y] >= 7]
    if not ys:
        return None
    return im.getchannel("A").getbbox()[3] - ys[0]


def hunt_body_height(path):
    """Mede capuz ate bota no centro, ignorando a ponta alta do arco."""
    im = np.asarray(Image.open(path).convert("RGBA"))
    width = im.shape[1]
    middle = im[:, int(width * .28):int(width * .72), 3] >= 128
    ys = np.flatnonzero(middle.sum(axis=1) >= 3)
    return int(ys[-1] - ys[0] + 1) if len(ys) else None


lines = ["// GERADO por tools/gen_sprite_norm.py - nao edite.",
         "#ifndef SPRITE_NORM_H", "#define SPRITE_NORM_H",
         "typedef struct { const char *name; float s[4]; } SpriteNorm;",
         "static const SpriteNorm SPRITE_NORM[] = {"]
hunt_idle_scale = None
for name in ANIMS:
    target = TARGET[name.split("_")[0]]
    vals = []
    for d in range(4):
        if name == "hunt_roll" and hunt_idle_scale is not None:
            vals.append(hunt_idle_scale[d])  # agachada por natureza, sem ampliar a pose
            continue
        frames = range(4) if name == "hunt_run" else range(1)
        measure = hunt_body_height if name.startswith("hunt_") else head
        heights = [measure(SPR / f"spr_{name}_d{d}_f{f}.rgba16.png") for f in frames]
        heights = [h for h in heights if h]
        observed = float(np.median(heights)) if name.startswith("hunt_") and heights else max(heights) if heights else 0
        vals.append(max(0.8, min(1.2, target / observed)) if observed else 1.0)
    if name == "hunt_idle":
        hunt_idle_scale = vals
    lines.append('    {"%s", {%s}},' % (name, ", ".join(f"{v:.3f}f" for v in vals)))
lines += ["};", "#endif", ""]
(ROOT / "include" / "sprite_norm.h").write_text("\n".join(lines), encoding="utf-8")
print("\n".join(lines))
