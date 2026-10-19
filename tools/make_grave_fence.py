"""Gera o sprite da cerca de ferro do cemiterio, em vista FRONTAL (reta).
Pilastra de pedra nas duas pontas + grade de lanças; lado a lado forma uma cerca continua.
Execute: python tools/make_grave_fence.py
"""
from pathlib import Path
import numpy as np
from PIL import Image

OUT = Path(__file__).resolve().parents[1] / "assets" / "scenery" / "scene_grave_fence.rgba16.png"
W, H = 108, 64
rng = np.random.default_rng(5)
img = np.zeros((H, W, 4), np.uint8)

def put(x, y, c):
    if 0 <= x < W and 0 <= y < H:
        img[y, x] = (c[0], c[1], c[2], 255)

def shade(c, k, n=6):
    j = int(rng.integers(-n, n + 1))
    return tuple(int(np.clip(v * k + j, 0, 255)) for v in c)

STONE = (118, 108, 90)
MOSS = (74, 86, 52)
IRON = (58, 44, 34)
RUST = (118, 74, 42)

def pillar(x0):
    pw = 14
    # corpo
    for y in range(13, H):
        for x in range(x0, x0 + pw):
            t = (x - x0) / (pw - 1)
            k = 1.12 - 0.5 * t                      # luz vem da esquerda
            c = shade(STONE, k)
            if y > H - 10 and rng.random() < 0.35: c = shade(MOSS, k)   # musgo na base
            if rng.random() < 0.05: c = shade(MOSS, k)
            put(x, y, c)
    # frisos horizontais
    for x in range(x0, x0 + pw):
        put(x, 22, shade(STONE, 0.6, 3)); put(x, 40, shade(STONE, 0.6, 3))
    # painel com relevo
    for y in range(27, 37):
        put(x0 + 3, y, shade(STONE, 0.65, 3)); put(x0 + pw - 4, y, shade(STONE, 0.5, 3))
    # capitel (piramide)
    for i in range(13):
        half = 1 + int(i * (pw / 2 + 1.5) / 12)
        cx = x0 + pw / 2
        for x in range(int(cx - half), int(cx + half) + 1):
            t = (x - (cx - half)) / max(1, 2 * half)
            put(x, i, shade(STONE, 1.2 - 0.6 * t))
    # ponta
    put(x0 + pw // 2, 0, shade(STONE, 1.25)); 

def bar(cx):
    # haste com ponta de lanca
    for y in range(18, H - 2):
        for dx, k in ((-1, 1.25), (0, 1.0), (1, 0.62)):
            c = shade(IRON, k, 5)
            if rng.random() < 0.10: c = shade(RUST, k)
            put(cx + dx, y, c)
    # lanca
    tip = [(0, 8), (0, 9), (-1, 10), (1, 10), (-1, 11), (0, 11), (1, 11), (-2, 12), (-1, 12), (0, 12), (1, 12), (2, 12),
           (-1, 13), (0, 13), (1, 13), (0, 14), (0, 15), (0, 16), (0, 17)]
    for dx, y in tip:
        k = 1.3 if dx < 0 else (1.0 if dx == 0 else 0.6)
        put(cx + dx, y, shade(IRON, k, 4))
    put(cx, 8, shade(RUST, 1.2))

def rail(y):
    for x in range(0, W):
        put(x, y, shade(IRON, 1.25, 4)); put(x, y + 1, shade(IRON, 0.95, 4)); put(x, y + 2, shade(IRON, 0.55, 4))
        if rng.random() < 0.08: put(x, y + 1, shade(RUST, 1.0))

rail(28); rail(50)
for i in range(9):
    bar(10 + i * 11)
pillar(-7); pillar(W - 7)   # meia pilastra em cada ponta: lado a lado formam uma pilastra inteira
# sapata de pedra continua (contato com o chao)
for y in range(H - 4, H):
    for x in range(0, W):
        put(x, y, shade(STONE, 0.7 - 0.05 * (y - (H - 4)), 5))

# contorno escuro de 1px
a = img[..., 3] > 0
edge = np.zeros_like(a)
edge[1:, :] |= a[:-1, :]; edge[:-1, :] |= a[1:, :]; edge[:, 1:] |= a[:, :-1]; edge[:, :-1] |= a[:, 1:]
edge &= ~a
img[edge] = (18, 14, 12, 255)
Image.fromarray(img, "RGBA").save(OUT, optimize=True)
Image.fromarray(img, "RGBA").resize((W * 4, H * 4), Image.NEAREST).save(OUT.parents[2] / "capturas" / "analise_mapa" / "cerca_nova.png")
print("cerca gerada:", OUT)
