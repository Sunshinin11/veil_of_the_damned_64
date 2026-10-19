"""Moldura padrao dos icones do cinto: a mesma moldura de ferro/madeira dos icones do Druida.
frame_icon(arte) -> icone 30x30 com a arte (22x22) por baixo da moldura extraida de spr_hud_icon_0..5."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageEnhance

SPR = Path(__file__).resolve().parents[1] / "assets" / "sprites"
_FRAME = None


def _frame():
    global _FRAME
    if _FRAME is None:
        a = np.stack([np.asarray(Image.open(SPR / f"spr_hud_icon_{i}.rgba16.png").convert("RGBA")).astype(int)
                      for i in range(6)])
        same = (np.abs(a - a[0]).max(axis=(0, 3)) <= 24) & (a[0, ..., 3] > 0)
        f = a[0].copy().astype(np.uint8)
        f[..., 3] = np.where(same, 255, 0)
        outer = (a[0, ..., 3] == 0)
        _FRAME = (Image.fromarray(f, "RGBA"), outer)
    return _FRAME


def frame_icon(art, fill=(14, 12, 10, 255), zoom=1.0):
    """art: RGBA de qualquer tamanho (corta as bordas transparentes)."""
    frame, outer = _frame()
    art = art.convert("RGBA")
    box = art.getchannel("A").point(lambda v: 255 if v > 40 else 0).getbbox()
    if box:
        art = art.crop(box)
    if zoom > 1.0:   # aproxima o centro (ilustracoes cheias de detalhe nas bordas)
        w, h = art.size
        cw, ch = int(w / zoom), int(h / zoom)
        art = art.crop(((w - cw) // 2, (h - ch) // 2, (w + cw) // 2, (h + ch) // 2))
    s = 22 / max(art.width, art.height)
    art = art.resize((max(1, round(art.width * s)), max(1, round(art.height * s))), Image.Resampling.LANCZOS)
    art = ImageEnhance.Contrast(art).enhance(1.15)
    out = Image.new("RGBA", (30, 30), fill)
    out.alpha_composite(art, ((30 - art.width) // 2, (30 - art.height) // 2))
    out.alpha_composite(frame)
    a = np.asarray(out).copy()
    a[outer] = (0, 0, 0, 0)          # cantos transparentes iguais aos do Druida
    return Image.fromarray(a, "RGBA")
