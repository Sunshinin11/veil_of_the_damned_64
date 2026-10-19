"""Export approved four-frame movement strips at the game's sprite size."""
import argparse
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from inspect_sprite_components import components

ROOT = Path(__file__).resolve().parents[1]


# Altura final (px) do personagem, igual aos quadros antigos de idle/andar, e a linha dos pes
# (y=60 na tela 64x64, como todos os outros quadros) para nao dar "pulo" ao trocar de animacao.
TARGET_HEIGHT = {("druid", "left"): 50, ("druid", "down"): 52,
                 ("druid", "front"): 52, ("druid", "back"): 52,
                 ("werewolf", "left"): 51}
FOOT_ROW = 60
# Altura da CABECA (capuz/cabeca ate os pes, sem o cajado) do quadro mais ereto de cada folha.
# Assim o personagem tem o mesmo tamanho andando de lado, de frente ou de costas (a silhueta de
# lado e mais estreita e parecia menor quando so a altura total era igualada).
HEAD_TARGET = {("druid", "left"): 49, ("druid", "down"): 49, ("druid", "front"): 49,
               ("druid", "back"): 49, ("werewolf", "left"): 50}


def head_height(frame):
    rows = np.asarray(frame.getchannel("A")) > 0
    counts = rows.sum(axis=1)
    top = next((y for y in range(64) if counts[y] >= 7), 0)
    return FOOT_ROW - top


def main(direction="left", character="druid", action="walk", variant=""):
    suffix = f" {variant.title()}" if variant else ""
    source = Image.open(ROOT / f"Sprites/{character.title()} {action.title()} {direction.title()} 4 Frames{suffix}.png").convert("RGBA")
    parts = sorted(
        (part for part in components(np.asarray(source.getchannel("A")) >= 100,
                                    with_runs=True) if part[4] > 3000),
        key=lambda part: part[0],
    )
    if len(parts) != 4:
        raise ValueError(f"Expected four complete figures, found {len(parts)}")
    scale = min(TARGET_HEIGHT[(character, direction)] / max(p[3] - p[1] for p in parts),
                60 / max(p[2] - p[0] for p in parts))
    output = ROOT / "assets/sprites"

    def pose_image(part, scale):
        x0, y0, x1, y1 = part[:4]
        pose = source.crop((x0, y0, x1, y1))
        mask = Image.new("L", pose.size)
        draw = ImageDraw.Draw(mask)
        for left, right, y in part[5]:
            draw.line((left - x0, y - y0, right - x0 - 1, y - y0), fill=255)
        pose.putalpha(mask)
        width, height = round(pose.width * scale), round(pose.height * scale)
        pose = pose.resize((width, height), Image.Resampling.LANCZOS)
        pose.putalpha(pose.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
        frame = Image.new("RGBA", (64, 64))
        frame.alpha_composite(pose, ((64 - width) // 2, FOOT_ROW - height))
        return frame

    # Cada quadro e escalado para ter a MESMA altura de cabeca (o quadro agachado da corrida
    # ficava ~18% menor que o ereto e o personagem "pulsava" de tamanho enquanto andava).
    target = HEAD_TARGET.get((character, direction))
    scales = []
    for part in parts:
        s_i = scale
        if target:
            s_i = scale * max(0.9, min(1.25, target / max(1, head_height(pose_image(part, scale)))))
            s_i = min(s_i, 62 / (part[2] - part[0]), 58 / (part[3] - part[1]))
        scales.append(s_i)
    for index, part in enumerate(parts):
        scale = scales[index]
        x0, y0, x1, y1 = part[:4]
        pose = source.crop((x0, y0, x1, y1))
        mask = Image.new("L", pose.size)
        draw = ImageDraw.Draw(mask)
        for left, right, y in part[5]:
            draw.line((left - x0, y - y0, right - x0 - 1, y - y0), fill=255)
        pose.putalpha(mask)
        width, height = round(pose.width * scale), round(pose.height * scale)
        pose = pose.resize((width, height), Image.Resampling.LANCZOS)
        pose.putalpha(pose.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
        frame = Image.new("RGBA", (64, 64))
        frame.alpha_composite(pose, ((64 - width) // 2, FOOT_ROW - height))
        variant_tag = f"_{variant}" if variant else ""
        path = output / f"spr_{character}_{action}_{direction}{variant_tag}_d0_f{index}.rgba16.png"
        frame.save(path, optimize=True)
        print(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--direction", choices=("left", "down", "front", "back"), default="left")
    parser.add_argument("--character", choices=("druid", "werewolf"), default="druid")
    parser.add_argument("--action", choices=("walk", "run"), default="walk")
    parser.add_argument("--variant", choices=("alt",), default="")
    args = parser.parse_args()
    main(args.direction, args.character, args.action, args.variant)
