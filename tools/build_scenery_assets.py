"""Prepare painted scenery sources for the N64 without changing the originals."""
from pathlib import Path
from PIL import Image
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Sprites" / "Cenario"
OUT = ROOT / "assets" / "scenery"

# Source path, N64 dimensions, repeating axes.
TEXTURES = {
    "floor_path": ("Caminho dos Condenados/Piso terra musgo.png", (32, 32), "xy"),
    "wall_path": ("Caminho dos Condenados/Muro cemiterio.png", (64, 32), "x"),
    "floor_crypt": ("Cripta Vazia/Piso lajes.png", (32, 32), "xy"),
    "wall_crypt": ("Cripta Vazia/Parede pedra.png", (64, 32), "x"),
    "floor_altar": ("Altar Negro/Piso obsidiana.png", (32, 32), "xy"),
    "wall_altar": ("Altar Negro/Parede obsidiana.png", (64, 32), "x"),
}

# Source path, transparent canvas dimensions. Every prop keeps its own alpha.
PROPS = {
    "tree_dead": ("Caminho dos Condenados/Arvore seca.png", (96, 96)),
    "gravestone": ("Caminho dos Condenados/Lapide gotica.png", (64, 64)),
    "chapel_arch": ("Caminho dos Condenados/Arco capela.png", (96, 96)),
    "traveler_body": ("Caminho dos Condenados/Corpo viajante.png", (96, 48)),
    "cultist_body": ("Cripta Vazia/Corpo cultista.png", (96, 48)),
    "stained_glass": ("Cripta Vazia/Vitral gotico.png", (64, 96)),
    "black_altar": ("Altar Negro/Altar sacrificial.png", (96, 96)),
    "malakor_throne": ("Altar Negro/Trono Malakor frontal.png", (96, 96)),
    "pentagram": ("Altar Negro/Pentagrama.png", (64, 64)),
    "monolith": ("Altar Negro/Monolito.png", (64, 96)),
    "crypt_chains": ("Cripta Vazia/Correntes penduradas.png", (64, 96)),
    "grave_fence": ("Caminho dos Condenados/Grade cemiterio frontal.png", (108, 64)),
    "path_shrine": ("Caminho dos Condenados/Santuario corvo.png", (96, 64)),
    "crypt_pillar": ("Cripta Vazia/Pilar ossario.png", (64, 96)),
    "sarcophagus": ("Cripta Vazia/Sarcófago cavaleiro.png", (96, 64)),
    "crypt_altar": ("Cripta Vazia/Altar profanado.png", (96, 64)),
}

ANIMATED_PROPS = {
    "campfire": ("Caminho dos Condenados/Fogueira", (64, 64)),
    "crypt_brazier": ("Cripta Vazia/Braseiro ferro", (64, 64)),
    "infernal_brazier": ("Altar Negro/Braseiro ritual", (64, 64)),
    "portal_path": ("Caminho dos Condenados/Portal", (96, 96)),
    "portal_crypt": ("Cripta Vazia/Portal", (96, 96)),
    "portal_altar": ("Altar Negro/Portal", (96, 96)),
}


def soften_tile_seam(image: Image.Image, axes: str) -> Image.Image:
    """Match opposite edge pixels after shrinking; keep the painted center."""
    pixels = np.asarray(image, dtype=np.float32).copy()
    for axis in axes:
        if axis == "x":
            left = pixels[:, 0].copy()
            right = pixels[:, -1].copy()
            edge = (left + right) * .5
            pixels[:, 0] = edge
            pixels[:, -1] = edge
            pixels[:, 1] = pixels[:, 1] * .7 + edge * .3
            pixels[:, -2] = pixels[:, -2] * .7 + edge * .3
        else:
            top = pixels[0].copy()
            bottom = pixels[-1].copy()
            edge = (top + bottom) * .5
            pixels[0] = edge
            pixels[-1] = edge
            pixels[1] = pixels[1] * .7 + edge * .3
            pixels[-2] = pixels[-2] * .7 + edge * .3
    return Image.fromarray(np.uint8(np.clip(pixels, 0, 255)), "RGB")


def build_texture(name: str, source: str, dimensions: tuple[int, int], axes: str) -> None:
    image = Image.open(SOURCE / source).convert("RGB")
    image = image.resize(dimensions, Image.Resampling.LANCZOS)
    image = soften_tile_seam(image, axes)
    image.save(OUT / f"scene_{name}.rgba16.png", optimize=True)


def build_prop(name: str, source: str, dimensions: tuple[int, int]) -> None:
    image = Image.open(SOURCE / source).convert("RGBA")
    if name == "grave_fence":
        # Crop through the center of each end pillar so neighboring straight
        # sections join into one pillar, as in the original 108x64 fence tile.
        image = image.crop((110, 54, 1519, 901)).resize(dimensions, Image.Resampling.LANCZOS)
        image.putalpha(image.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
        image.save(OUT / f"scene_{name}.rgba16.png", optimize=True)
        return
    box = image.getchannel("A").point(lambda a: 255 if a >= 32 else 0).getbbox()
    if not box:
        raise ValueError(f"Empty scenery source: {source}")
    image = image.crop(box)
    width, height = dimensions
    factor = min((width - 4) / image.width, (height - 4) / image.height)
    image = image.resize((max(1, round(image.width * factor)),
                          max(1, round(image.height * factor))), Image.Resampling.LANCZOS)
    # RGBA16 has a one-bit alpha channel. Preserve fine painted outlines but
    # remove low-opacity halos that would become harsh opaque pixels.
    image.putalpha(image.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
    canvas = Image.new("RGBA", dimensions)
    x = (width - image.width) // 2
    y = (height - image.height) // 2 if name in {"pentagram", "traveler_body", "cultist_body"} else height - 2 - image.height
    canvas.alpha_composite(image, (x, y))
    canvas.save(OUT / f"scene_{name}.rgba16.png", optimize=True)


def build_animated_prop(name: str, stem: str, dimensions: tuple[int, int]) -> None:
    """Use one scale and bottom anchor for all three frames of a prop."""
    sources = [(SOURCE / f"{stem}{'' if frame == 0 and not name.startswith('portal_') else f' {frame}'}.png")
               for frame in range(3)]
    images = [Image.open(path).convert("RGBA") for path in sources]
    boxes = [im.getchannel("A").point(lambda a: 255 if a >= 32 else 0).getbbox()
             for im in images]
    if any(box is None for box in boxes):
        raise ValueError(f"Empty animated scenery source: {stem}")
    # Frame edits have almost identical source canvases. Their common crop keeps
    # the stone/iron body fixed while only fire or magical energy changes.
    if max(im.width for im in images) - min(im.width for im in images) <= 2 and \
       max(im.height for im in images) - min(im.height for im in images) <= 2:
        left = min(box[0] for box in boxes)
        top = min(box[1] for box in boxes)
        right = max(box[2] for box in boxes)
        bottom = max(box[3] for box in boxes)
        crops = [im.crop((left, top, right, bottom)) for im in images]
    else:
        crops = [im.crop(box) for im, box in zip(images, boxes)]
    width, height = dimensions
    factor = min((width - 4) / max(im.width for im in crops),
                 (height - 4) / max(im.height for im in crops))
    for frame, image in enumerate(crops):
        image = image.resize((max(1, round(image.width * factor)),
                              max(1, round(image.height * factor))), Image.Resampling.LANCZOS)
        image.putalpha(image.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
        canvas = Image.new("RGBA", dimensions)
        canvas.alpha_composite(image, ((width - image.width) // 2, height - 2 - image.height))
        canvas.save(OUT / f"scene_{name}_f{frame}.rgba16.png", optimize=True)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for name, args in TEXTURES.items():
        build_texture(name, *args)
    for name, args in PROPS.items():
        build_prop(name, *args)
    for name, args in ANIMATED_PROPS.items():
        build_animated_prop(name, *args)
    print(f"Prepared {len(TEXTURES)} textures, {len(PROPS)} props and {len(ANIMATED_PROPS)} animations in {OUT}")


if __name__ == "__main__":
    main()
