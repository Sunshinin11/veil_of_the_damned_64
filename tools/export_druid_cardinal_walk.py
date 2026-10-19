"""Collect the existing painted Druid walk frames into a four-direction sheet."""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "sprites"
OUTPUT = ROOT / "Sprites" / "Pranchetas"

# Visual facing direction, from top row to bottom row.
DIRECTIONS = (
    ("cima", "druid_walk_alt", 0),
    ("baixo", "druid_walk", 0),
    ("esquerda", "druid_walk_alt", 2),
    ("direita", "druid_walk", 2),
)


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sheet = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    frames = []
    for direction, (name, prefix, source_row) in enumerate(DIRECTIONS):
        for phase in range(4):
            path = SOURCE / f"spr_{prefix}_d{source_row}_f{phase}.rgba16.png"
            frame = Image.open(path).convert("RGBA")
            if frame.size != (64, 64):
                raise ValueError(f"Unexpected frame size in {path}: {frame.size}")
            sheet.alpha_composite(frame, (phase * 64, direction * 64))
            frames.append(frame)

    sheet_path = OUTPUT / "Druida caminhada 4 direcoes pronto.png"
    sheet.save(sheet_path, optimize=True)

    # An animated comparison shows the four directions at the same phase.
    preview = []
    for phase in range(4):
        strip = Image.new("RGBA", (256, 64), (0, 0, 0, 0))
        for direction in range(4):
            strip.alpha_composite(frames[direction * 4 + phase], (direction * 64, 0))
        preview.append(strip.resize((1024, 256), Image.Resampling.NEAREST))
    preview_path = OUTPUT / "Druida caminhada 4 direcoes previa.gif"
    preview[0].save(
        preview_path,
        save_all=True,
        append_images=preview[1:],
        duration=180,
        loop=0,
        disposal=2,
        transparency=0,
    )
    print(sheet_path)
    print(preview_path)


if __name__ == "__main__":
    main()
