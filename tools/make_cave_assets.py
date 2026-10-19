"""Gera as texturas e os adereços de CAVERNA da Cripta Vazia (mapa 2), em pixel art procedural.

  assets/scenery/scene_floor_cave.rgba16.png      chao 32x32 (repete nos dois eixos)
  assets/scenery/scene_wall_cave.rgba16.png       face do muro 64x32 (repete)
  assets/scenery/scene_wall_cave_top.rgba16.png   topo do muro 64x32 (repete)
  assets/scenery/scene_stalagmite.rgba16.png      coluna natural (estalagmite)
  assets/scenery/scene_cave_rocks.rgba16.png      pedras soltas
  assets/scenery/scene_crystal.rgba16.png         aglomerado de cristais brilhantes
Rode:  python tools/make_cave_assets.py   (o Makefile converte os PNGs em .sprite)
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parents[1] / "assets" / "scenery"
rng = np.random.default_rng(7)


def periodic_noise(w, h, gx, gy, seed):
    """Ruido de valor suave que repete nas duas direcoes (grade gx x gy)."""
    r = np.random.default_rng(seed)
    grid = r.random((gy, gx))
    ys, xs = np.mgrid[0:h, 0:w]
    fx, fy = xs / w * gx, ys / h * gy
    x0, y0 = np.floor(fx).astype(int), np.floor(fy).astype(int)
    tx, ty = fx - x0, fy - y0
    tx, ty = tx * tx * (3 - 2 * tx), ty * ty * (3 - 2 * ty)
    g = lambda yy, xx: grid[yy % gy, xx % gx]
    top = g(y0, x0) * (1 - tx) + g(y0, x0 + 1) * tx
    bot = g(y0 + 1, x0) * (1 - tx) + g(y0 + 1, x0 + 1) * tx
    return top * (1 - ty) + bot * ty


def height_field(w, h, octaves, seed, aniso=1.0):
    total = np.zeros((h, w))
    amp, norm = 1.0, 0.0
    for i, (gx, gy) in enumerate(octaves):
        total += amp * periodic_noise(w, h, gx, max(1, int(gy * aniso)), seed + i * 11)
        norm += amp
        amp *= 0.55
    return total / norm


def shade(height, palette, emboss=0.0):
    """Mapeia altura -> paleta (poucos tons = cara de pixel art) com relevo iluminado por cima."""
    hgt = height.copy()
    dy = np.roll(hgt, 1, axis=0) - hgt            # luz vinda de cima
    dx = np.roll(hgt, 1, axis=1) - hgt
    lit = hgt + emboss * (dy * 1.0 + dx * 0.5)
    lit = (lit - lit.min()) / (lit.max() - lit.min() + 1e-6)
    idx = np.clip((lit * len(palette)).astype(int), 0, len(palette) - 1)
    pal = np.array(palette, dtype=np.uint8)
    rgb = pal[idx]
    return np.dstack([rgb, np.full(idx.shape, 255, np.uint8)])


def cracks(img, count, seed, length=(6, 16), color=(22, 20, 24)):
    r = np.random.default_rng(seed)
    h, w = img.shape[:2]
    for _ in range(count):
        x, y = int(r.integers(0, w)), int(r.integers(0, h))
        dx, dy = r.choice([-1, 0, 1]), r.choice([-1, 0, 1])
        if dx == 0 and dy == 0:
            dx = 1
        for _ in range(int(r.integers(*length))):
            img[y % h, x % w, :3] = color
            x += dx + int(r.integers(-1, 2) == 1 and dy == 0) * 0
            y += dy
            if r.random() < 0.35:
                dx = int(np.clip(dx + r.integers(-1, 2), -1, 1))
                dy = int(np.clip(dy + r.integers(-1, 2), -1, 1))
            if dx == 0 and dy == 0:
                dx = 1
    return img


FLOOR_PAL = [(38, 35, 38), (49, 45, 47), (60, 55, 56), (72, 66, 64), (86, 79, 74), (100, 92, 84)]
WALL_PAL = [(30, 28, 33), (42, 39, 44), (55, 51, 55), (68, 63, 66), (82, 76, 76), (96, 89, 86)]
TOP_PAL = [(54, 50, 52), (68, 63, 63), (82, 76, 73), (97, 90, 84), (112, 104, 95), (126, 117, 106)]


def floor_tex():
    h = height_field(32, 32, [(3, 3), (6, 6), (12, 12)], 100)
    img = shade(h, FLOOR_PAL, emboss=2.2)
    img = cracks(img, 2, 3, (4, 8), color=(40,36,38))
    r = np.random.default_rng(5)
    for _ in range(9):   # pedrinhas claras com sombra embaixo
        x, y = int(r.integers(0, 32)), int(r.integers(0, 32))
        img[(y + 1) % 32, x % 32, :3] = (34, 31, 34)
        img[y % 32, x % 32, :3] = (112, 104, 94)
        img[y % 32, (x + 1) % 32, :3] = (92, 85, 78)
    for _ in range(3):   # manchas de musgo azulado
        x, y = int(r.integers(0, 32)), int(r.integers(0, 32))
        img[y % 32, x % 32, :3] = (52, 78, 74)
    return img


def wall_tex():
    h = height_field(64, 32, [(4, 4), (8, 12), (16, 20)], 200, aniso=1.0)
    stripes = (np.sin(np.mgrid[0:32, 0:64][0] / 32 * np.pi * 4 + 1.7 * periodic_noise(64, 32, 4, 2, 9)) + 1) / 2
    img = shade(0.65 * h + 0.35 * stripes, WALL_PAL, emboss=3.0)
    img = cracks(img, 3, 11, (5, 10))
    return img


def top_tex():
    h = height_field(64, 32, [(4, 2), (8, 4), (16, 8), (32, 16)], 300)
    img = shade(h, TOP_PAL, emboss=2.0)
    return cracks(img, 5, 21, (4, 10), color=(40, 37, 40))


def save(arr, name):
    Image.fromarray(arr, "RGBA").save(OUT / f"scene_{name}.rgba16.png", optimize=True)


def rock_shade(canvas, mask, cx, hw_fn, light=(-0.6, 0.7), base=(120, 110, 100), noise_seed=1):
    """Pinta os pixels de 'mask' como solido cilindrico iluminado pela esquerda + ruido."""
    h, w = mask.shape
    nz = periodic_noise(w, h, 6, 8, noise_seed)
    nz2 = periodic_noise(w, h, 12, 16, noise_seed + 3)
    for y in range(h):
        for x in range(w):
            if not mask[y, x]:
                continue
            hw = max(1.0, hw_fn(y))
            u = max(-1.0, min(1.0, (x - cx) / hw))
            zz = (1 - u * u) ** 0.5
            lam = max(0.0, light[0] * u + light[1] * zz)
            v = 0.30 + 0.80 * lam + 0.22 * (nz[y, x] - 0.5) + 0.12 * (nz2[y, x] - 0.5)
            if y % 9 in (0, 1):
                v *= 0.86   # estratos horizontais
            v = np.clip(v, 0.12, 1.2)
            canvas[y, x] = (int(min(255, base[0] * v)), int(min(255, base[1] * v)), int(min(255, base[2] * v)), 255)


def outline(canvas):
    a = canvas[..., 3] > 0
    h, w = a.shape
    edge = a & ~(np.roll(a, 1, 0) & np.roll(a, -1, 0) & np.roll(a, 1, 1) & np.roll(a, -1, 1))
    canvas[edge, :3] = (canvas[edge, :3] * 0.45).astype(np.uint8)


def stalagmite():
    w, h = 40, 96
    canvas = np.zeros((h, w, 4), np.uint8)
    cx = w / 2
    r = np.random.default_rng(4)
    wob = [float(r.normal(0, 0.9)) for _ in range(h)]
    for y in range(h):
        t = y / (h - 1)                         # 0 = ponta, 1 = base
        half = 1.6 + (cx - 2.5) * (t ** 0.85) + wob[y] * 0.4
        # alarga um pouco a base (raizes)
        if t > 0.9:
            half += (t - 0.9) * 28
        x0, x1 = int(cx - half), int(cx + half)
        for x in range(max(0, x0), min(w, x1 + 1)):
            canvas[y, x, 3] = 255
    mask = canvas[..., 3] > 0
    rock_shade(canvas, mask, cx, lambda y: 1.6 + (cx - 2.5) * ((y / (h - 1)) ** 0.85) + (28 * max(0, y / (h - 1) - 0.9)),
               base=(118, 108, 100), noise_seed=31)
    outline(canvas)
    return canvas


def rocks():
    w, h = 56, 36
    canvas = np.zeros((h, w, 4), np.uint8)
    blobs = [(18, 30, 15, 17), (38, 31, 13, 12), (28, 33, 10, 8), (8, 33, 7, 7), (46, 34, 8, 6)]
    for i, (cx, base_y, rw, rh) in enumerate(blobs):
        layer = np.zeros((h, w, 4), np.uint8)
        m = np.zeros((h, w), bool)
        for y in range(h):
            for x in range(w):
                dx, dy = (x - cx) / rw, (y - base_y) / rh
                if dy <= 0.05 and dx * dx + dy * dy <= 1.0 + 0.08 * ((x * 7 + y * 3 + i) % 5 - 2):
                    m[y, x] = True
        rock_shade(layer, m, cx, lambda y, rw=rw: rw, base=(112 - i * 4, 104 - i * 4, 98 - i * 3), noise_seed=40 + i)
        outline(layer)
        mask = layer[..., 3] > 0
        canvas[mask] = layer[mask]
    return canvas


def crystal():
    w, h = 48, 64
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # base de rocha
    d.polygon([(2, 63), (6, 52), (16, 47), (30, 48), (42, 53), (46, 63)], fill=(70, 64, 66, 255), outline=(30, 28, 32, 255))
    d.polygon([(8, 62), (12, 54), (22, 51), (30, 53), (36, 62)], fill=(88, 80, 78, 255))
    prisms = [  # (x base centro, largura, altura, inclinacao)
        (24, 12, 46, 0), (13, 9, 32, -5), (35, 9, 30, 5), (19, 7, 22, -9), (30, 6, 20, 9)]
    for cx, pw, ph, tilt in prisms:
        by = 56
        tip = (cx + tilt, by - ph)
        left, right = (cx - pw // 2, by), (cx + pw // 2, by)
        mid = (cx + tilt * 0.5, by - ph * 0.72)
        d.polygon([left, (cx, by), tip], fill=(96, 224, 226, 255), outline=(20, 70, 90, 255))        # faceta clara
        d.polygon([(cx, by), right, tip], fill=(40, 150, 190, 255), outline=(20, 70, 90, 255))     # faceta escura
        d.line([tip, (cx, by)], fill=(205, 255, 255, 255), width=1)                                    # aresta brilhante
        d.polygon([tip, (tip[0] - pw // 4, tip[1] + ph // 6), (tip[0] + pw // 4, tip[1] + ph // 6)], fill=(210, 255, 255, 255))
    arr = np.array(img)
    arr[..., 3] = np.where(arr[..., 3] > 100, 255, 0)
    return arr


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    save(floor_tex(), "floor_cave")
    save(wall_tex(), "wall_cave")
    save(top_tex(), "wall_cave_top")
    save(stalagmite(), "stalagmite")
    save(rocks(), "cave_rocks")
    save(crystal(), "crystal")
    print("ok")


if __name__ == "__main__":
    main()
