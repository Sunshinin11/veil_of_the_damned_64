"""Prepare four distinct painted gait phases in each cardinal direction."""

from pathlib import Path
from PIL import Image
import numpy as np


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Sprites" / "Pranchetas" / "Druida caminhada nova fontes"
OUTPUT = ROOT / "Sprites" / "Pranchetas" / "Druida caminhada nova quadros"


def foreground(image):
    image = image.convert("RGBA")
    alpha = image.getchannel("A").point(lambda value: 255 if value >= 80 else 0)
    image.putalpha(alpha)
    box = alpha.getbbox()
    if box is None:
        raise ValueError("Empty generated pose")
    return image.crop(box)


def strip_poses(filename):
    image = Image.open(SOURCE / filename).convert("RGBA")
    occupied = (np.asarray(image.getchannel("A")) >= 80).sum(axis=0)
    cuts = [0]
    for quarter in (1, 2, 3):
        center = round(image.width * quarter / 4)
        start, end = max(0, center - 85), min(image.width, center + 85)
        cuts.append(start + int(np.argmin(occupied[start:end])))
    cuts.append(image.width)
    poses = []
    for left, right in zip(cuts, cuts[1:]):
        poses.append(foreground(image.crop((left, 0, right, image.height))))
    return poses


def single_pose(filename):
    return foreground(Image.open(SOURCE / filename))


def game_frame(pose):
    scale = min(58 / pose.width, 54 / pose.height)
    width, height = min(44, round(pose.width * scale)), round(pose.height * scale)
    resized = pose.resize((width, height), Image.Resampling.LANCZOS)
    resized.putalpha(resized.getchannel("A").point(lambda value: 255 if value >= 100 else 0))
    canvas = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    canvas.alpha_composite(resized, ((64 - width) // 2, 62 - height))
    return canvas


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    up = strip_poses("cima_transicoes.png")
    left = strip_poses("esquerda_transicoes.png")
    right = strip_poses("direita_transicoes.png")
    cycles = {
        "cima": [single_pose("cima_passo_esquerdo.png"), up[1],
                 single_pose("cima_passo_direito.png"), up[3]],
        "baixo": [single_pose("baixo_passo_esquerdo.png"),
                  single_pose("baixo_joelho_direito.png"),
                  single_pose("baixo_passo_direito.png"),
                  single_pose("baixo_joelho_esquerdo.png")],
        "esquerda": [left[0], left[1], single_pose("esquerda_passo_moderado.png"), left[3]],
        "direita": [right[0], right[1], single_pose("direita_passo_moderado.png"), right[3]],
    }
    sheet = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    all_frames = {}
    for row, (direction, poses) in enumerate(cycles.items()):
        frames = [game_frame(pose) for pose in poses]
        all_frames[direction] = frames
        for phase, frame in enumerate(frames):
            frame.save(OUTPUT / f"druida_andar_{direction}_{phase}.png", optimize=True)
            sheet.alpha_composite(frame, (phase * 64, row * 64))
    sheet.save(OUTPUT / "Druida caminhada ciclo 4 direcoes.png", optimize=True)

    preview = []
    for phase in range(4):
        strip = Image.new("RGBA", (256, 64), (0, 0, 0, 0))
        for column, direction in enumerate(cycles):
            strip.alpha_composite(all_frames[direction][phase], (column * 64, 0))
        preview.append(strip.resize((1024, 256), Image.Resampling.NEAREST))
    preview[0].save(
        OUTPUT / "Druida caminhada ciclo 4 direcoes.gif",
        save_all=True,
        append_images=preview[1:],
        duration=170,
        loop=0,
        disposal=2,
        transparency=0,
    )
    print(OUTPUT)


if __name__ == "__main__":
    main()
