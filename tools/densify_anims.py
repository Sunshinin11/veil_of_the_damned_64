"""Gera versoes SUAVES (12 quadros) das animacoes do druida e do lobisomem.

Le os 4 quadros originais  assets/sprites/spr_<anim>_d<dir>_f0..f3.rgba16.png  (os que o
build_sprite_assets.py gera a cada build) e grava  spr_<anim>_s_d<dir>_f0..f11.rgba16.png  ao lado
(sufixo "_s"). Os quadros originais NAO sao tocados; por isso o build pode regenerar os de 4 quadros
sem estragar os de 12. As poses entre os quadros sao calculadas por fluxo optico (ver export_druid_run).

  - animacoes em LOOP (parado, andar, correr): o ciclo e circular (quadro 4 -> quadro 1);
  - animacoes de UMA VEZ (ataque, magia, dano, morte, esquiva, transformacao): do 1o ao ultimo, sem voltar.

Uso:  python tools/densify_anims.py          (todas)
      python tools/densify_anims.py druid_idle werewolf_attack   (so as citadas)
O jogo usa NF_PLAYER (include/anim_frames.h) como numero de quadros: mantenha igual a TARGET.
"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parent))
from export_druid_run import between, _prep

ROOT = Path(__file__).resolve().parents[1]
SPR = ROOT / "assets" / "sprites"
TARGET = 12
LOOP = ["druid_idle", "druid_walk", "druid_walk_alt", "druid_run", "druid_run_alt",
        "werewolf_idle", "werewolf_walk", "werewolf_run",
        "hunt_idle", "hunt_run", "barb_idle", "barb_run", "barb_spin"]
ONCE = ["druid_attack", "druid_cast", "druid_escape", "druid_hurt", "druid_death", "druid_transform",
        "werewolf_attack", "werewolf_cast", "werewolf_escape", "werewolf_hurt", "werewolf_death",
        "hunt_attack", "hunt_hurt", "hunt_death", "hunt_roll",
        "barb_attack", "barb_attack2", "barb_shout", "barb_hurt", "barb_death", "barb_roll"]
HOLD = {"barb_run": (1, 2)}                   # direcoes sem interpolacao (poses seguras)
SINGLE_DIR = {"werewolf_run_left": 1}          # so a direcao 0
LOOP_SINGLE = ["werewolf_run_left"]


def densify(name, d, loop):
    # Corrida e idle de costas: a primeira pose das cartelas tem dois arcos.
    source_frames = (1, 2, 3) if name in ("hunt_run", "hunt_idle") and d == 3 else range(4)
    src = [SPR / f"spr_{name}_d{d}_f{i}.rgba16.png" for i in source_frames]
    if not all(p.exists() for p in src):
        print("  (falta)", name, d); return False
    frames = [Image.open(p).convert("RGBA") for p in src]
    big = [_prep(f) for f in frames]
    n = len(frames)
    if name in HOLD and d in HOLD[name]:
        # passada larga de lado/diagonal: o fluxo optico borra as pernas; segura cada pose 3 quadros
        for j in range(TARGET):
            frames[(j * n) // TARGET].save(SPR / f"spr_{name}_s_d{d}_f{j}.rgba16.png", optimize=True)
        return True
    for j in range(TARGET):
        u = j * n / TARGET if loop else j * (n - 1) / (TARGET - 1)
        i = int(u + 1e-6); t = u - i
        if i >= n - 1 and not loop: i, t = n - 2, 1.0
        if t < 1e-3:
            out = frames[i % n]
        elif t > 1 - 1e-3:
            out = frames[(i + 1) % n]
        else:
            out = between(big[i % n], big[(i + 1) % n], t)
        out.save(SPR / f"spr_{name}_s_d{d}_f{j}.rgba16.png", optimize=True)
    return True


def main():
    only = set(sys.argv[1:])
    jobs = [(a, True, 4) for a in LOOP] + [(a, False, 4) for a in ONCE] + [(a, True, 1) for a in LOOP_SINGLE]
    for name, loop, ndir in jobs:
        if only and name not in only: continue
        ok = sum(densify(name, d, loop) for d in range(ndir))
        print(f"{name}: {ok} direcoes -> {TARGET} quadros ({'loop' if loop else 'uma vez'})", flush=True)


if __name__ == "__main__":
    main()
