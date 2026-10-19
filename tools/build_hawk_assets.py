"""Exporta um gavião por quadro das cartelas de voo da Caçadora."""

from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter

from inspect_sprite_components import components

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Sprites" / "DLC"
OUTPUT = ROOT / "assets" / "sprites"
# As duas vistas laterais usadas pelo jogo apontam para a direita;
# o renderer espelha para a esquerda quando necessário.
VIEWS = ("Frente", "Diagonal Cima Direita", "Direita", "Costas")


def extract(view):
    path = SOURCE / f"Poder Gaviao Voo {view} 4 Quadros.png"
    sheet = Image.open(path).convert("RGBA")
    alpha = np.asarray(sheet.getchannel("A"))
    birds = sorted(
        (part for part in components(alpha >= 180, with_runs=True) if part[4] > 3000),
        key=lambda part: (part[0] + part[2]) / 2,
    )
    if len(birds) != 4:
        raise ValueError(f"{path.name}: esperados 4 gaviões, encontrados {len(birds)}")
    frames = []
    for left, top, right, bottom, _, runs in birds:
        margin = 2
        box = (max(0, left - margin), max(0, top - margin),
               min(sheet.width, right + margin), min(sheet.height, bottom + margin))
        bird = sheet.crop(box)
        mask = Image.new("L", bird.size)
        draw = ImageDraw.Draw(mask)
        for x0, x1, y in runs:
            draw.line((x0 - box[0], y - box[1], x1 - box[0] - 1, y - box[1]), fill=255)
        mask = mask.filter(ImageFilter.MaxFilter(3))
        bird.putalpha(ImageChops.multiply(bird.getchannel("A"), mask))
        frames.append(bird)
    return frames


def build():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sheets = [extract(view) for view in VIEWS]
    width = max(bird.width for row in sheets for bird in row)
    height = max(bird.height for row in sheets for bird in row)
    scale = min(50.0 / width, 42.0 / height)
    for direction, row in enumerate(sheets):
        for frame, bird in enumerate(row):
            size = (max(1, round(bird.width * scale)), max(1, round(bird.height * scale)))
            small = bird.resize(size, Image.Resampling.LANCZOS)
            small.putalpha(small.getchannel("A").point(lambda value: 255 if value >= 100 else 0))
            canvas = Image.new("RGBA", (56, 48))
            canvas.alpha_composite(small, ((56 - size[0]) // 2, (48 - size[1]) // 2))
            canvas.save(OUTPUT / f"spr_hawk_d{direction}_f{frame}.rgba16.png", optimize=True)
    print("Gavião: 4 direções x 4 quadros, uma ave por quadro")


if __name__ == "__main__":
    build()
