"""Finalize the requested N64 scenery PNGs at their exact game dimensions."""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "scenery"
MANIFEST = json.loads((OUT / "scene_generation_manifest.json").read_text(encoding="utf-8"))
PREVIEW = ROOT / "Sprites" / "Pranchetas" / "cenario_gotico_revisao.png"


def rgba16_preview(image: Image.Image) -> Image.Image:
    arr = np.asarray(image.convert("RGBA"), dtype=np.uint8).copy()
    rgb5 = np.rint(arr[:, :, :3].astype(np.float32) * (31.0 / 255.0))
    arr[:, :, :3] = np.rint(rgb5 * (255.0 / 31.0)).astype(np.uint8)
    arr[:, :, 3] = np.where(arr[:, :, 3] >= 96, 255, 0).astype(np.uint8)
    arr[arr[:, :, 3] == 0, :3] = 0
    return Image.fromarray(arr, "RGBA")


def tile(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    image = image.convert("RGB")
    # Keep the central painted region; any framing at a generated edge is discarded.
    w, h = image.size
    cw, ch = int(w * .76), int(h * .76)
    image = image.crop(((w-cw)//2, (h-ch)//2, (w+cw)//2, (h+ch)//2))
    image = image.resize(size, Image.Resampling.LANCZOS)
    pixels = np.asarray(image, dtype=np.float32).copy()
    for axis in (1, 0):
        length = pixels.shape[axis]
        band = max(3, length // 6)
        for i in range(band):
            j = length - 1 - i
            weight = (1 - i / band) ** 2
            if axis == 1:
                a, b = pixels[:, i].copy(), pixels[:, j].copy()
                avg = (a + b) / 2
                pixels[:, i] = a * (1-weight) + avg * weight
                pixels[:, j] = b * (1-weight) + avg * weight
            else:
                a, b = pixels[i].copy(), pixels[j].copy()
                avg = (a + b) / 2
                pixels[i] = a * (1-weight) + avg * weight
                pixels[j] = b * (1-weight) + avg * weight
    opaque = Image.fromarray(np.uint8(np.clip(pixels, 0, 255)), "RGB").convert("RGBA")
    return rgba16_preview(opaque)


def isolated(image: Image.Image, size: tuple[int, int], kind: str, name: str) -> Image.Image:
    image = image.convert("RGBA")
    alpha = image.getchannel("A")
    initial_threshold = 32 if name == "scene_cobweb.rgba16.png" else 64
    box = alpha.point(lambda a: 255 if a >= initial_threshold else 0).getbbox()
    if box is None:
        raise ValueError(f"No visible source pixels: {name}")
    image = image.crop(box)
    width, height = size
    margin = 1 if kind == "decal" else 2
    scale = min((width - 2*margin) / image.width, (height - margin) / image.height)
    resized = image.resize((max(1, round(image.width*scale)), max(1, round(image.height*scale))), Image.Resampling.LANCZOS)
    # Downsampling can dilute wires and thin web strands. Their alpha is still binary.
    threshold = 56 if name in {"scene_cobweb.rgba16.png", "scene_grave_fence.rgba16.png", "scene_cell_bars.rgba16.png", "scene_wall_chains.rgba16.png", "scene_decal_cracks.rgba16.png"} else 96
    arr = np.asarray(resized, dtype=np.uint8).copy()
    arr[:, :, 3] = np.where(arr[:, :, 3] >= threshold, 255, 0).astype(np.uint8)
    resized = Image.fromarray(arr, "RGBA")
    # Trim empty rows introduced by resize, then anchor standing props to the bottom.
    box = resized.getchannel("A").getbbox()
    if box is None:
        raise ValueError(f"Sprite disappeared on downsampling: {name}")
    resized = resized.crop(box)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    x = (width - resized.width) // 2
    y = (height - resized.height) if kind == "prop" else (height - resized.height) // 2
    canvas.alpha_composite(resized, (x, y))
    return rgba16_preview(canvas)


def contact_sheet(results: dict[str, Image.Image]) -> None:
    font = ImageFont.load_default()
    columns, cell_w, cell_h = 4, 300, 340
    rows = (len(results) + columns - 1) // columns
    sheet = Image.new("RGB", (columns*cell_w, rows*cell_h), (34, 30, 38))
    draw = ImageDraw.Draw(sheet)
    for i, (name, image) in enumerate(results.items()):
        x, y = (i % columns)*cell_w, (i // columns)*cell_h
        checker = Image.new("RGB", (280, 288), (48, 43, 53))
        dc = ImageDraw.Draw(checker)
        for cy in range(0, 288, 16):
            for cx in range(0, 280, 16):
                if (cx//16 + cy//16) % 2:
                    dc.rectangle((cx, cy, cx+15, cy+15), fill=(60, 54, 64))
        magnification = min(3, 270//image.width, 274//image.height)
        magnification = max(1, magnification)
        shown = image.resize((image.width*magnification, image.height*magnification), Image.Resampling.NEAREST)
        checker.paste(shown, ((280-shown.width)//2, (288-shown.height)//2), shown)
        sheet.paste(checker, (x+10, y+30))
        draw.text((x+10, y+8), name.replace(".rgba16.png", ""), font=font, fill=(235, 230, 220))
        draw.text((x+10, y+323), f"{image.width} x {image.height}", font=font, fill=(185, 181, 178))
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(PREVIEW)


def main() -> None:
    results = {}
    for name, entry in MANIFEST.items():
        source = Image.open(entry["source"])
        size = tuple(entry["size"])
        image = tile(source, size) if entry["kind"] == "tile" else isolated(source, size, entry["kind"], name)
        image.save(OUT / name, optimize=True)
        results[name] = image
        arr = np.asarray(image)
        alpha_values = np.unique(arr[:, :, 3]).tolist()
        assert image.size == size and set(alpha_values) <= {0, 255}, name
        if entry["kind"] == "tile":
            assert alpha_values == [255], name
            assert np.array_equal(arr[:, 0], arr[:, -1]), f"Horizontal seam: {name}"
            assert np.array_equal(arr[0], arr[-1]), f"Vertical seam: {name}"
        elif entry["kind"] == "prop":
            assert np.any(arr[-1, :, 3] == 255), f"Floating base: {name}"
        print(f"{name}: {image.size}, alpha={alpha_values}")
    contact_sheet(results)
    print(f"Preview: {PREVIEW}")


if __name__ == "__main__":
    main()
