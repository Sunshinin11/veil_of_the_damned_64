"""Importa as cartas: Sprites/Cartas/c1..c3_cartas.png (grades 4x2 de ilustracoes, ordem = id da carta)
e c4_molduras.png (4x1: comum, rara, lendaria, amaldicoada) -> assets/card_XX.rgba16.png (64x80) e
assets/card_frame_<raridade>.rgba16.png (72x96, centro transparente). Uso: python tools/import_cards.py
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "Cartas_Druida"
if not SRC.exists():
    SRC = ROOT / "Sprites" / "Cartas"
OUT = ROOT / "assets"


def grid(img, cols, rows):
    cw, ch = img.width // cols, img.height // rows
    return [img.crop((c * cw, r * ch, (c + 1) * cw, (r + 1) * ch)) for r in range(rows) for c in range(cols)]


def main():
    cid = 0
    for sheet in ("c1_cartas.png", "c2_cartas.png", "c3_cartas.png"):
        p = SRC / sheet
        if not p.exists():
            print("falta", sheet); cid += 8; continue
        for cell in grid(Image.open(p).convert("RGB"), 4, 2):
            w, h = cell.size                    # corta 4% de borda (evita sobra da celula vizinha)
            cell = cell.crop((int(w * .04), int(h * .04), int(w * .96), int(h * .96)))
            art = cell.resize((64, 80), Image.Resampling.LANCZOS).filter(ImageFilter.UnsharpMask(0.8, 70, 2))
            art.convert("RGBA").save(OUT / f"card_{cid:02d}.rgba16.png", optimize=True)
            cid += 1
    p = SRC / "c4_molduras.png"
    if p.exists():
        names = ["common", "rare", "legendary", "cursed"]
        for name, cell in zip(names, grid(Image.open(p).convert("RGBA"), 4, 1)):
            a = np.asarray(cell.getchannel("A")) >= 100
            ys, xs = np.nonzero(a)
            cell = cell.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
            fr = cell.resize((72, 96), Image.Resampling.LANCZOS)
            arr = np.asarray(fr).copy()
            arr[..., 3] = np.where(arr[..., 3] >= 128, 255, 0)
            arr[arr[..., 3] == 0, :3] = 0
            Image.fromarray(arr, "RGBA").save(OUT / f"card_frame_{name}.rgba16.png", optimize=True)
    # Cacadora: Sprites/Cartas_Cacadora/h1..h3_cartas.png (mesmas grades 4x2) -> card_24..47
    hsrc = ROOT / "Sprites" / "Cartas_Cacadora"
    hid = 24
    for sheet in ("h1_cartas.png", "h2_cartas.png", "h3_cartas.png"):
        p = hsrc / sheet
        if not p.exists():
            print("falta", sheet, "(fica o provisorio)"); hid += 8; continue
        for cell in grid(Image.open(p).convert("RGB"), 4, 2):
            w, h = cell.size
            cell = cell.crop((int(w * .04), int(h * .04), int(w * .96), int(h * .96)))
            art = cell.resize((64, 80), Image.Resampling.LANCZOS).filter(ImageFilter.UnsharpMask(0.8, 70, 2))
            art.convert("RGBA").save(OUT / f"card_{hid:02d}.rgba16.png", optimize=True)
            mark = OUT / f".card_{hid:02d}.placeholder"
            if mark.exists():
                mark.write_text("")   # arte real: o importador da cacadora nao sobrescreve mais
                mark.rename(OUT / f".card_{hid:02d}.real")
            hid += 1
    # Barbaro: Sprites/Cartas_Barbaro/b1..b3_cartas.png -> card_48..71
    bsrc = ROOT / "Sprites" / "Cartas_Barbaro"
    bid = 48
    for sheet in ("b1_cartas.png", "b2_cartas.png", "b3_cartas.png"):
        p = bsrc / sheet
        if not p.exists():
            print("falta", sheet, "(fica o provisorio)"); bid += 8; continue
        for cell in grid(Image.open(p).convert("RGB"), 4, 2):
            w, h = cell.size
            cell = cell.crop((int(w * .04), int(h * .04), int(w * .96), int(h * .96)))
            art = cell.resize((64, 80), Image.Resampling.LANCZOS).filter(ImageFilter.UnsharpMask(0.8, 70, 2))
            art.convert("RGBA").save(OUT / f"card_{bid:02d}.rgba16.png", optimize=True)
            mark = OUT / f".card_{bid:02d}.placeholder"
            if mark.exists():
                mark.rename(OUT / f".card_{bid:02d}.real")
            bid += 1
    print("cartas:", cid, "cacadora ate", hid - 1, "barbaro ate", bid - 1)


if __name__ == "__main__":
    main()
