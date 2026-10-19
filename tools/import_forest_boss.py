"""Importa o chefe da Floresta Morta (mesmo formato das pranchetas do Malakor/Varkhul):
  Sprites/Floresta_Morta/s1..s6_sylvara_*.png    -> assets/sprites/spr_sylvara_<acao>_d*_f*.rgba16.png (144x144)
  Sprites/Floresta_Morta/r1..r6_maeraiz_*.png    -> assets/sprites/spr_rootmother_<acao>_d*_f*.rgba16.png
Enquanto as pranchetas nao existem o jogo usa o Malakor/Varkhul pintados de musgo.
Uso:  python tools/import_forest_boss.py
"""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_altar as ia

ROOT = Path(__file__).resolve().parents[1]
ia.SRC = ROOT / "Sprites" / "Floresta_Morta"
ia.BOSSES = {
    "sylvara": {"idle": "s1_sylvara_parada", "walk": "s2_sylvara_andando", "attack": "s3_sylvara_ataque",
                "hurt": "s4_sylvara_dano", "death": "s5_sylvara_morte", "enrage": "s6_sylvara_furia"},
    "rootmother": {"idle": "r1_maeraiz_parada", "walk": "r2_maeraiz_andando", "attack": "r3_maeraiz_ataque",
                   "hurt": "r4_maeraiz_dano", "death": "r5_maeraiz_morte", "enrage": "r6_maeraiz_furia"},
}

if __name__ == "__main__":
    missing = [f"{fn}.png" for acts in ia.BOSSES.values() for fn in acts.values() if not (ia.SRC / f"{fn}.png").exists()]
    if missing:
        raise SystemExit("faltando em Sprites/Floresta_Morta: " + ", ".join(missing))
    ia.bosses()
