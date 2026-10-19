"""Gera as texturas extras do chao do Caminho dos Condenados e os sprites de sombra/brilho.

Parte do chao ja existente (assets/scenery/scene_floor_path.rgba16.png) e cria variantes
repetiveis (trilha de terra batida, grama morta, terra revirada) por transformacao de cor e
ruido periodico, para que as emendas continuem invisiveis. Tambem cria:
  scene_shadow.rgba32.png      sombra suave (32x16) sob personagens e objetos
  scene_glow_fire.rgba32.png   poca de luz laranja (64x32) sob fogueiras e braseiros
  scene_glow_gold.rgba32.png   brilho dourado (64x32) sob baus fechados
Execute: python tools/prepare_cemetery_floor.py
"""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "scenery"
SIZE = 32


def periodic_noise(rng, scale):
    """Ruido de valor periodico (repete nas bordas) em SIZE x SIZE, 0..1."""
    cells = max(2, SIZE // scale)
    grid = rng.random((cells, cells)).astype(np.float32)
    ys = np.linspace(0, cells, SIZE, endpoint=False)
    xs = np.linspace(0, cells, SIZE, endpoint=False)
    out = np.zeros((SIZE, SIZE), np.float32)
    for yi, y in enumerate(ys):
        y0 = int(y) % cells
        y1 = (y0 + 1) % cells
        fy = y - int(y)
        fy = fy * fy * (3 - 2 * fy)
        for xi, x in enumerate(xs):
            x0 = int(x) % cells
            x1 = (x0 + 1) % cells
            fx = x - int(x)
            fx = fx * fx * (3 - 2 * fx)
            top = grid[y0, x0] * (1 - fx) + grid[y0, x1] * fx
            bot = grid[y1, x0] * (1 - fx) + grid[y1, x1] * fx
            out[yi, xi] = top * (1 - fy) + bot * fy
    return out


def speckles(rng, count, radius_choices=(0, 0, 1)):
    """Pedrinhas/folhas: mascara 0..1 com manchas pequenas que dao a volta na borda."""
    mask = np.zeros((SIZE, SIZE), np.float32)
    for _ in range(count):
        cx, cy = rng.integers(0, SIZE, 2)
        r = int(rng.choice(radius_choices))
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dx * dx + dy * dy <= r * r + 0.5:
                    mask[(cy + dy) % SIZE, (cx + dx) % SIZE] = 1.0
    return mask


def save_rgb(name, arr):
    img = Image.fromarray(np.uint8(np.clip(arr, 0, 255)), "RGB")
    img.save(OUT / f"scene_{name}.rgba16.png", optimize=True)


def luminance_norm(base):
    lum = base.mean(axis=2)
    lum = (lum - lum.mean()) / (lum.std() + 1e-3)
    return lum


def make_trail(base, rng):
    """Calcamento de pedra irregular (paralelepipedos): celulas de Voronoi periodicas + argamassa escura."""
    n = 9
    seeds = []
    for gy in range(3):
        for gx in range(3):
            seeds.append(((gx + 0.5 + rng.uniform(-0.28, 0.28)) * SIZE / 3, (gy + 0.5 + rng.uniform(-0.28, 0.28)) * SIZE / 3))
    cell_tone = [0.80 + 0.34 * rng.random() for _ in range(n)]
    cell_warm = [rng.uniform(-6, 6) for _ in range(n)]
    arr = np.zeros((SIZE, SIZE, 3), np.float32)
    stone = np.array([98, 91, 80], np.float32)
    mortar = np.array([34, 30, 26], np.float32)
    moss = np.array([58, 74, 38], np.float32)
    for y in range(SIZE):
        for x in range(SIZE):
            ds = []
            for i, (sx, sy) in enumerate(seeds):
                dx = abs(x + 0.5 - sx); dy = abs(y + 0.5 - sy)
                dx = min(dx, SIZE - dx); dy = min(dy, SIZE - dy)
                ds.append((dx * dx + dy * dy) ** 0.5)
            order = np.argsort(ds)
            i1, i2 = order[0], order[1]
            gap = ds[i2] - ds[i1]
            if gap < 1.5:
                c = mortar * (0.8 + 0.4 * rng.random())
                if rng.random() < 0.35: c = c * 0.5 + moss * 0.5
            else:
                sx, sy = seeds[i1]
                ox = (x + 0.5 - sx); oy = (y + 0.5 - sy)
                bevel = np.clip((-ox - oy) / 9.0, -1, 1) * 9.0           # luz vem de cima/esquerda
                edge = np.clip(1.0 - (gap - 1.5) / 2.2, 0, 1) * 18.0     # borda da pedra mais escura
                c = stone * cell_tone[i1] + cell_warm[i1] + bevel - edge
                c = c + rng.uniform(-5, 5)
                if rng.random() < 0.05: c = c * 0.6 + moss * 0.4
            arr[y, x] = c
    return arr


def make_grass(base, rng):
    lum = luminance_norm(base)
    tone = np.array([52, 70, 36], np.float32)  # grama: verde vivo, mais claro que o musgo da base
    arr = np.ones((SIZE, SIZE, 3), np.float32) * tone
    n = periodic_noise(rng, 6)
    arr *= (0.8 + 0.4 * n)[..., None]
    arr += lum[..., None] * 8.0
    blades = speckles(rng, 70, (0,))
    arr = arr * (1 - blades[..., None] * 0.4) + blades[..., None] * np.array([92, 112, 52]) * 0.4
    dry = speckles(rng, 34, (0,))
    arr = arr * (1 - dry[..., None] * 0.5) + dry[..., None] * np.array([110, 88, 48]) * 0.5
    return arr


def make_moss(base, rng):
    """Musgo/terra esverdeada: chao base da area (domina o mapa, margeando a trilha de pedra)."""
    lum = luminance_norm(base)
    tone = np.array([40, 52, 33], np.float32)
    arr = np.ones((SIZE, SIZE, 3), np.float32) * tone
    n = periodic_noise(rng, 8)
    arr *= (0.78 + 0.44 * n)[..., None]
    arr += lum[..., None] * 6.0
    blades = speckles(rng, 80, (0,))
    arr = arr * (1 - blades[..., None] * 0.4) + blades[..., None] * np.array([82, 104, 52]) * 0.4
    dirt = speckles(rng, 26, (0, 1))
    arr = arr * (1 - dirt[..., None] * 0.5) + dirt[..., None] * np.array([84, 66, 44]) * 0.5
    dark = speckles(rng, 18, (0,))
    arr *= (1 - dark[..., None] * 0.3)
    return arr


def make_soil(base, rng):
    lum = luminance_norm(base)
    tone = np.array([46, 31, 25], np.float32)  # terra revirada, escura e avermelhada
    arr = np.ones((SIZE, SIZE, 3), np.float32) * tone
    n = periodic_noise(rng, 5)
    arr *= (0.75 + 0.5 * n)[..., None]
    arr += lum[..., None] * 6.0
    clods = speckles(rng, 22, (0, 1))
    arr *= (1 - clods[..., None] * 0.3)
    bone = speckles(rng, 2, (0,))
    arr = arr * (1 - bone[..., None] * 0.7) + bone[..., None] * np.array([170, 160, 135]) * 0.7
    return arr


def make_shadow():
    w, h = 32, 16
    img = np.zeros((h, w, 4), np.uint8)
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt(((xx - (w - 1) / 2) / (w / 2)) ** 2 + ((yy - (h - 1) / 2) / (h / 2)) ** 2)
    alpha = np.clip(1.0 - d, 0, 1) ** 0.75 * 165
    img[..., 3] = np.uint8(alpha)
    Image.fromarray(img, "RGBA").save(OUT / "scene_shadow.rgba32.png", optimize=True)


def make_glow(name, rgb, peak):
    w, h = 64, 32
    img = np.zeros((h, w, 4), np.uint8)
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt(((xx - (w - 1) / 2) / (w / 2)) ** 2 + ((yy - (h - 1) / 2) / (h / 2)) ** 2)
    alpha = np.clip(1.0 - d, 0, 1) ** 1.6 * peak
    img[..., 0], img[..., 1], img[..., 2] = rgb
    img[..., 3] = np.uint8(alpha)
    Image.fromarray(img, "RGBA").save(OUT / f"scene_{name}.rgba32.png", optimize=True)


def make_vortex():
    """Vortice azul (32x32 RGBA32): bracos em espiral, nucleo claro e borda suave. Gira por UV no jogo."""
    n = 32
    img = np.zeros((n, n, 4), np.uint8)
    for y in range(n):
        for x in range(n):
            dx = (x + 0.5 - n / 2) / (n / 2); dy = (y + 0.5 - n / 2) / (n / 2)
            r = (dx * dx + dy * dy) ** 0.5
            th = np.arctan2(dy, dx)
            if r >= 0.98:
                continue
            rim = np.clip((0.98 - r) / 0.40, 0, 1) ** 1.2
            arms = 0.5 + 0.5 * np.sin(3 * th + 9.0 * r)
            core = np.exp(-(r / 0.38) ** 2)
            a = rim * (0.40 + 0.60 * arms) * 0.85 + core * 0.55
            a = float(np.clip(a, 0, 1))
            mix = np.clip(core + 0.55 * arms * (1 - r), 0, 1)
            c0 = np.array([36, 120, 230], np.float32); c1 = np.array([210, 245, 255], np.float32)
            c = c0 * (1 - mix) + c1 * mix
            img[y, x] = (int(c[0]), int(c[1]), int(c[2]), int(a * 235))
    Image.fromarray(img, "RGBA").save(OUT / "scene_portal_vortex.rgba32.png", optimize=True)


def make_vision():
    """Escuridao em volta do jogador (32x32 RGBA32, escalada ~17x no jogo): centro claro, borda quase opaca."""
    n = 32
    img = np.zeros((n, n, 4), np.uint8)
    for y in range(n):
        for x in range(n):
            r = (((x + 0.5 - n / 2) / (n / 2)) ** 2 + ((y + 0.5 - n / 2) / (n / 2)) ** 2) ** 0.5
            # 0 ate r=0.30 (raio de visao livre), sobe suave ate ~0.96 em r=0.86
            t = np.clip((r - 0.30) / 0.56, 0, 1)
            a = (t * t * (3 - 2 * t)) ** 0.9 * 0.97
            img[y, x] = (14, 18, 20, int(a * 255))
    Image.fromarray(img, "RGBA").save(OUT / "scene_vision.rgba32.png", optimize=True)


def make_fog_puff():
    """Mancha de nevoa suave (32x32 RGBA32) para as camadas que derivam na tela."""
    n = 32
    rng = np.random.default_rng(71)
    noise = periodic_noise(rng, 8)
    img = np.zeros((n, n, 4), np.uint8)
    for y in range(n):
        for x in range(n):
            r = (((x + 0.5 - n / 2) / (n / 2)) ** 2 + ((y + 0.5 - n / 2) / (n / 2)) ** 2) ** 0.5
            a = np.clip(1.0 - r, 0, 1) ** 1.5 * (0.55 + 0.45 * noise[y, x]) * 0.55
            img[y, x] = (150, 158, 160, int(a * 255))
    Image.fromarray(img, "RGBA").save(OUT / "scene_fog_puff.rgba32.png", optimize=True)


def main():
    base = np.asarray(Image.open(OUT / "scene_floor_path.rgba16.png").convert("RGB"), np.float32)
    save_rgb("floor_trail", make_trail(base, np.random.default_rng(11)))
    save_rgb("floor_grass", make_grass(base, np.random.default_rng(23)))
    save_rgb("floor_soil", make_soil(base, np.random.default_rng(37)))
    save_rgb("floor_moss", make_moss(base, np.random.default_rng(53)))
    make_shadow()
    make_glow("glow_fire", (255, 128, 36), 120)
    make_glow("glow_gold", (255, 205, 70), 105)
    make_glow("glow_blue", (70, 170, 255), 125)
    make_vortex()
    make_vision()
    make_fog_puff()
    print("Texturas de chao, sombra e brilhos gerados em", OUT)


if __name__ == "__main__":
    main()
