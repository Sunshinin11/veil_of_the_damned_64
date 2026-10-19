"""Importa o chefe do Norte (mesmo formato do Malakor/Sylvara):
  Sprites/Norte_Boss/k1..k6_jarl_*.png    -> assets/sprites/spr_jarl_<acao>_d*_f*.rgba16.png (144x144)
  Sprites/Norte_Boss/d1..d6_draugr_*.png  -> assets/sprites/spr_draugr_<acao>_d*_f*.rgba16.png
Enquanto nao existem, o jogo usa o Malakor/Varkhul pintados de gelo.  Uso: python tools/import_north_boss.py
"""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_altar as ia

ROOT = Path(__file__).resolve().parents[1]
ia.SRC = ROOT / "Sprites" / "Norte_Boss"
ia.BOSSES = {
    "jarl": {"idle": "k1_jarl_parado", "walk": "k2_jarl_andando", "attack": "k3_jarl_ataque",
             "hurt": "k4_jarl_dano", "death": "k5_jarl_morte", "enrage": "k6_jarl_furia"},
    "draugr": {"idle": "d1_draugr_parado", "walk": "d2_draugr_andando", "attack": "d3_draugr_ataque",
               "hurt": "d4_draugr_dano", "death": "d5_draugr_morte", "enrage": "d6_draugr_furia"},
}

if __name__ == "__main__":
    missing = [f"{fn}.png" for acts in ia.BOSSES.values() for fn in acts.values() if not (ia.SRC / f"{fn}.png").exists()]
    if missing:
        raise SystemExit("faltando em Sprites/Norte_Boss: " + ", ".join(missing))
    ia.bosses()
