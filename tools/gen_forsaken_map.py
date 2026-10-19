"""Gera include/map_forsaken.h: mapa do Caminho dos Condenados (area 1).

O mapa e desenhado aqui como level design (zonas, trilha, cercas, marcos) e exportado como
grade de caracteres + poligonos de chao (trilha organica, terra, lajes, grama). Rode de novo
depois de mexer:   python tools/gen_forsaken_map.py

A camera do jogo mostra so ~2,8 tiles de largura, entao tudo que importa fica a ate 2 tiles da
trilha; o mapa e estreito e denso de proposito.

Legenda (cada tile = 72 unidades; linha 0 = norte):
  #  muro de pedra            .  chao livre            E  entrada (spawn)
  D  descida p/ a cripta (saida unica)                 N  mausoleu selado (decoracao)
  T  lapide   X  cruz   U  estatua   G  arvore gigante   R  arvore seca   K  arbusto
  J  cerca de ferro (corre leste-oeste)    g  arco do portao (vao de 1 tile)   p  santuario do corvo
  F  fogueira   ~  corpo   Y  pilha de ossos   C  vaga de bau (so algumas abrem por partida)
  b v k o  decalques no chao (ossos, sangue, rachaduras, cova aberta)
  s S Z q  grupos de inimigos (pequeno, medio, misto, elite+escolta)   a  emboscada
"""
from pathlib import Path
import math
import random

COLS, ROWS = 18, 34
TILE = 72.0
rng = random.Random(11)
half_w, half_h = COLS * TILE / 2, ROWS * TILE / 2
grid = [['.' for _ in range(COLS)] for _ in range(ROWS)]


def put(c, r, ch, force=True):
    if 0 <= c < COLS and 0 <= r < ROWS and (force or grid[r][c] == '.'):
        grid[r][c] = ch


def wxz(c, r):
    """centro do tile (c, r) em unidades de mundo (c, r podem ser fracionarios)."""
    return ((c + 0.5) * TILE - half_w, half_h - (r + 0.5) * TILE)


# ------------------------------------------------------------------ muros e portoes
for c in range(COLS):
    put(c, 0, '#'); put(c, ROWS - 1, '#')
for r in range(ROWS):
    put(0, r, '#'); put(COLS - 1, r, '#')
GATE_C = 9
# portao sul (linha 29): muro com vao de 1 tile fechado por arco + estatuas
for c in range(1, 8): put(c, 29, '#')
for c in range(11, COLS - 1): put(c, 29, '#')
put(GATE_C, 29, 'g'); put(GATE_C - 1, 29, '#'); put(GATE_C + 1, 29, '#')
put(GATE_C - 1, 28, 'U'); put(GATE_C + 1, 28, 'U')   # estatuas na frente do muro, ladeando o portao
# patio do spawn (linhas 30..32): 6 tiles de largura
for r in range(30, 33):
    for c in range(1, 6): put(c, r, '#')
    for c in range(12, COLS - 1): put(c, r, '#')
put(GATE_C, 31, 'E')
put(7, 31, 'F'); put(11, 31, 'F'); put(8, 32, '~'); put(10, 30, 'b'); put(6, 30, 'Y')

# portao do meio (linha 11): cerca de parede a parede + arco + estatuas
for c in range(1, 8): put(c, 11, 'J')
for c in range(11, COLS - 1): put(c, 11, 'J')
put(GATE_C, 11, 'g'); put(GATE_C - 1, 11, 'u'); put(GATE_C + 1, 11, 'u')

# ------------------------------------------------------------------ mausoleu (saida)
put(GATE_C, 2, '.')
put(5, 2, 'U'); put(13, 2, 'U')
put(GATE_C, 17, 'P')   # portal de saida: aparece aqui quando os 3 guardioes caem
put(7, 3, 'F'); put(11, 3, 'F')
put(8, 1, 'T'); put(10, 1, 'T'); put(6, 4, 'K'); put(12, 4, 'K')

# ------------------------------------------------------------------ clareira profanada (linhas 4..9)
G = (7, 6)
put(*G, 'G')
for c, r, ch in [(5, 6, 'T'), (6, 4, 'X'), (8, 4, 'T'), (10, 5, 'U'), (10, 8, 'X'), (8, 9, 'T'), (6, 8, 'U')]:
    put(c, r, ch)
put(6, 6, 'o'); put(8, 7, 'o'); put(7, 7, 'k'); put(6, 5, 'b')
put(7, 7 + 0, 'k')
put(13, 7, 'Z'); put(3, 7, 'Z')
put(3, 9, 'q'); put(13, 5, 'q'); put(9, 3, 'q')
put(8, 6, 'a'); put(12, 4, 'a')
put(2, 5, 'C'); put(15, 8, 'C')
put(3, 5, 'X'); put(15, 6, 'T'); put(14, 9, 'T'); put(2, 8, 'R'); put(15, 4, 'R'); put(1, 3, 'R')

# ------------------------------------------------------------------ cemiterio murado (jazigos, linhas 13..18)
for c in range(1, 6): put(c, 13, 'J'); put(c, 18, 'J')
for c in range(12, 17): put(c, 13, 'J'); put(c, 18, 'J')
for c, r, ch in [(2, 14, 'T'), (4, 14, 'X'), (2, 16, 'X'), (4, 16, 'T'), (13, 14, 'X'), (15, 14, 'T'), (13, 16, 'T'), (15, 16, 'X')]:
    put(c, r, ch)
put(1, 14, 'C'); put(16, 14, 'C'); put(3, 15, 'a'); put(14, 15, 'a')
put(3, 17, 'S'); put(14, 17, 'S'); put(2, 15, 'o'); put(4, 15, 'o'); put(13, 15, 'o'); put(15, 15, 'o')
put(6, 15, 'U'); put(11, 15, 'U'); put(6, 17, 'T'); put(11, 17, 'X')
put(9, 13, 's'); put(7, 12, 'K'); put(11, 12, 'K')

# ------------------------------------------------------------------ encruzilhada e campo baixo (linhas 19..28)
put(9, 22, 'p')                                   # santuario do corvo no meio da trilha
put(7, 22, 'K'); put(11, 22, 'K'); put(8, 21, 'b')
put(2, 24, 'C'); put(2, 23, 'X'); put(3, 25, 'T'); put(1, 24, 'R'); put(2, 26, 'Y'); put(3, 23, 's')   # nicho oeste
put(15, 21, 'F'); put(14, 22, '~'); put(16, 22, 'Y'); put(16, 20, 'C'); put(14, 20, 'Z')                # acampamento leste
put(2, 14, '.'); put(15, 14, '.'); put(15, 20, '.'); put(1, 15, '.')  # garante acesso aos baus de canto
put(13, 21, 'v'); put(14, 21, 'b')
put(13, 26, 's'); put(4, 27, 's'); put(10, 19, 's')
for c, r, ch in [(4, 26, 'T'), (5, 27, 'X'), (6, 27, 'T'), (12, 27, 'X'), (14, 27, 'T'), (13, 25, 'K'), (5, 28, 'K')]:
    put(c, r, ch, False)

# alameda de arvores e lapides acompanhando a trilha (so no que ainda esta livre)
for r in list(range(19, 29, 2)) + list(range(2, 11, 2)):
    for cs in ((6, 7), (11, 12)):
        put(rng.choice(cs), r + rng.choice((0, 1)), rng.choice('TXKRTR'), False)
# floresta nas bordas
for r in range(1, ROWS - 1):
    for c in (1, 16):
        if grid[r][c] == '.' and rng.random() < 0.62:
            put(c, r, 'R' if rng.random() < 0.8 else 'K')
    for c in (2, 15):
        if grid[r][c] == '.' and rng.random() < 0.20:
            put(c, r, 'R' if rng.random() < 0.55 else 'K')
for c in range(2, COLS - 2):
    if grid[1][c] == '.' and rng.random() < 0.5:
        put(c, 1, 'R' if rng.random() < 0.6 else 'K')

# ------------------------------------------------------------------ chao: poligonos
quads = []   # (kind, [(x,z) x6])


def add_quad(kind, a, b, c, d):
    quads.append((kind, [a, b, c, a, c, d]))


def add_fan(kind, center, ring):
    tris = [(center, ring[i], ring[(i + 1) % len(ring)]) for i in range(len(ring))]
    if len(tris) % 2:
        tris.append((tris[0][0], tris[0][0], tris[0][0]))   # triangulo degenerado: completa o par
    for i in range(0, len(tris), 2):
        quads.append((kind, list(tris[i]) + list(tris[i + 1])))


def smooth(points, step=12.0):
    """Catmull-Rom em unidades de mundo, devolve pontos a cada ~step."""
    pts = [points[0]] + points + [points[-1]]
    out = []
    for i in range(1, len(pts) - 2):
        p0, p1, p2, p3 = pts[i - 1], pts[i], pts[i + 1], pts[i + 2]
        n = max(2, int(math.dist(p1, p2) / step))
        for k in range(n):
            t = k / n
            out.append(tuple(0.5 * ((2 * p1[j]) + (-p0[j] + p2[j]) * t + (2 * p0[j] - 5 * p1[j] + 4 * p2[j] - p3[j]) * t * t +
                                    (-p0[j] + 3 * p1[j] - 3 * p2[j] + p3[j]) * t ** 3) for j in range(2)))
    out.append(points[-1])
    return out


def strip(kind, tile_pts, half_width, taper_start=0.0, taper_end=0.0, seed=0):
    path = smooth([wxz(c, r) for c, r in tile_pts])
    total = sum(math.dist(path[i], path[i + 1]) for i in range(len(path) - 1))
    left, right, s = [], [], 0.0
    for i, p in enumerate(path):
        q = path[min(i + 1, len(path) - 1)]
        o = path[max(i - 1, 0)]
        dx, dz = q[0] - o[0], q[1] - o[1]
        ln = math.hypot(dx, dz) or 1.0
        nx, nz = -dz / ln, dx / ln
        if i: s += math.dist(path[i - 1], p)
        def w(side):
            wob = 1 + 0.20 * math.sin(s * 0.019 + seed + side * 2.1) + 0.13 * math.sin(s * 0.051 + seed * 1.7 + side * 4.0)
            f = 1.0
            if taper_start and s < taper_start: f = min(f, 0.25 + 0.75 * s / taper_start)
            if taper_end and total - s < taper_end: f = min(f, 0.15 + 0.85 * (total - s) / taper_end)
            return half_width * wob * f
        left.append((p[0] + nx * w(0), p[1] + nz * w(0)))
        right.append((p[0] - nx * w(1), p[1] - nz * w(1)))
    for i in range(len(path) - 1):
        add_quad(kind, left[i], right[i], right[i + 1], left[i + 1])


def blob(kind, cx, cz, radius, seed, n=10, squash=1.0):
    ring = []
    for i in range(n):
        a = i / n * 2 * math.pi
        rr = radius * (0.72 + 0.5 * (math.sin(a * 2 + seed) * 0.5 + 0.5) * (0.6 + 0.4 * math.sin(a * 3 + seed * 1.3)))
        ring.append((cx + math.cos(a) * rr, cz + math.sin(a) * rr * squash))
    add_fan(kind, (cx, cz), ring)


main = [(9, 31), (9, 29), (9.4, 27), (8.2, 24.6), (9, 22), (9.2, 20), (9, 17), (9, 14), (9, 11), (10, 9), (10.8, 7), (10.6, 5), (9.6, 3.2), (9, 2.2)]
west = [(8.2, 24.2), (6.2, 24.0), (4.0, 24.3), (2.4, 24.1)]
east = [(9.6, 22.2), (11.6, 21.8), (13.6, 21.2), (15.0, 21.2)]

# grama morta: manchas longe da trilha e das areas especiais
centerline = smooth([wxz(c, r) for c, r in main], 18.0) + smooth([wxz(c, r) for c, r in west], 18.0) + smooth([wxz(c, r) for c, r in east], 18.0)
grass_n, tries = 0, 0
while grass_n < 34 and tries < 600:
    tries += 1
    x = rng.uniform(-half_w + 90, half_w - 90); z = rng.uniform(-half_h + 90, half_h - 90)
    if min(math.dist((x, z), p) for p in centerline) < 80: continue
    c, r = int((x + half_w) // TILE), int((half_h - z) // TILE)
    if grid[r][c] == '#' or (13 <= r <= 18 and (c <= 5 or c >= 12)) or math.dist((x, z), wxz(*G)) < 190: continue
    if r >= 29: continue
    blob(3, x, z, rng.uniform(46, 96), rng.uniform(0, 6), 10, 0.85); grass_n += 1

# jazigos: terra revirada (retangulos) + manchas sob covas
for c0, c1 in ((1, 5), (12, 16)):
    x0 = c0 * TILE - half_w + 6; x1 = (c1 + 1) * TILE - half_w - 6
    z1 = half_h - 14 * TILE - 4; z0 = half_h - 18 * TILE + 4
    add_quad(1, (x0, z0), (x1, z0), (x1, z1), (x0, z1))
for r in range(ROWS):
    for c in range(COLS):
        if grid[r][c] in 'oa':
            cx, cz = wxz(c, r); blob(1, cx, cz, 34, c * 1.7 + r, 8, 0.8)

# trilha
strip(0, main, 46.0, 0.0, 0.0, 1.0)
strip(0, west, 36.0, 40.0, 70.0, 2.3)
strip(0, east, 36.0, 40.0, 70.0, 4.1)
# patio do spawn
px0, px1 = 6 * TILE - half_w, 12 * TILE - half_w
pz1, pz0 = half_h - 30 * TILE, half_h - 33 * TILE
add_quad(0, (px0, pz0), (px1, pz0), (px1, pz1), (px0, pz1))
# lajes antigas ao redor da arvore gigante
gx, gz = wxz(*G)
ring = []
for i in range(22):
    a = i / 22 * 2 * math.pi
    rr = 142 + 14 * math.sin(a * 3 + 1) + 8 * math.sin(a * 7)
    ring.append((gx + math.cos(a) * rr, gz + math.sin(a) * rr * 0.95))
add_fan(2, (gx, gz), ring)

order = {3: 0, 1: 1, 0: 2, 2: 3}
quads.sort(key=lambda q: order[q[0]])
print('entradas de chao:', len(quads), {k: sum(1 for q in quads if q[0] == k) for k in range(4)})

# ------------------------------------------------------------------ validacao (conectividade)
start = next((c, r) for r in range(ROWS) for c in range(COLS) if grid[r][c] == 'E')
blocking = set('#JTXUuGRNpgF')
seen, stack = {start}, [start]
while stack:
    c, r = stack.pop()
    for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        n = (c + dc, r + dr)
        if n in seen or not (0 <= n[0] < COLS and 0 <= n[1] < ROWS): continue
        ch = grid[n[1]][n[0]]
        if ch in blocking and ch != 'g': continue
        if ch == 'g' or ch not in blocking:
            seen.add(n); stack.append(n)
problems = [(grid[r][c], c, r) for r in range(ROWS) for c in range(COLS) if grid[r][c] in 'PCsSZqa~' and (c, r) not in seen]
print('inacessiveis (andando em 4 direcoes):', problems)
counts = {}
for row in grid:
    for ch in row: counts[ch] = counts.get(ch, 0) + 1
print({k: v for k, v in sorted(counts.items())})

# ------------------------------------------------------------------ exporta
root = Path(__file__).resolve().parents[1]
lines = ['// GERADO por tools/gen_forsaken_map.py - nao edite a mao, rode o script de novo.',
         '#ifndef MAP_FORSAKEN_H', '#define MAP_FORSAKEN_H', '', '#include <stdint.h>', '',
         f'#define MAP_CEM_COLS {COLS}', f'#define MAP_CEM_ROWS {ROWS}', 'static const char *const MAP_CEM[] = {']
for r in range(ROWS):
    lines.append('    "' + ''.join(grid[r]) + '"' + (',' if r < ROWS - 1 else ''))
lines += ['};', '',
          '// Chao: cada entrada = 2 triangulos (6 vertices). kind: 0 trilha, 1 terra revirada, 2 lajes antigas, 3 grama morta.',
          'typedef struct { int16_t x[6], z[6]; uint8_t kind; } FloorQuad;',
          f'#define MAP_CEM_FLOOR_COUNT {len(quads)}',
          'static const FloorQuad MAP_CEM_FLOOR[MAP_CEM_FLOOR_COUNT] = {']
for i, (k, pts) in enumerate(quads):
    xs = ','.join(str(int(round(p[0]))) for p in pts); zs = ','.join(str(int(round(p[1]))) for p in pts)
    lines.append(f'    {{{{{xs}}},{{{zs}}},{k}}}' + (',' if i < len(quads) - 1 else ''))
lines += ['};', '', '#endif // MAP_FORSAKEN_H', '']
(root / 'include' / 'map_forsaken.h').write_text('\n'.join(lines), encoding='utf-8')
print('escrito include/map_forsaken.h')

# ------------------------------------------------------------------ previa PNG
try:
    from PIL import Image, ImageDraw
    S = 24
    img = Image.new('RGB', (COLS * S, ROWS * S), (36, 34, 26))
    d = ImageDraw.Draw(img)
    palette = {0: (120, 100, 74), 1: (70, 44, 36), 2: (70, 78, 84), 3: (62, 74, 40)}
    for k, pts in quads:
        poly = [((x + half_w) / TILE * S, (half_h - z) / TILE * S) for x, z in pts]
        d.polygon(poly[:3], fill=palette[k]); d.polygon(poly[3:], fill=palette[k])
    col = {'#': (80, 80, 92), 'E': (60, 220, 60), 'P': (255, 255, 90), 'C': (255, 190, 40), 's': (230, 60, 60),
           'S': (230, 60, 60), 'Z': (255, 100, 60), 'q': (255, 0, 120), 'a': (200, 120, 255), 'G': (30, 160, 60),
           'R': (50, 110, 50), 'K': (90, 120, 60), 'T': (170, 170, 190), 'X': (170, 170, 190), 'U': (210, 210, 235), 'u': (210, 210, 235),
           'J': (170, 120, 70), 'g': (220, 220, 140), 'p': (140, 200, 220), 'F': (255, 130, 0), 'N': (200, 200, 240),
           '~': (150, 50, 50), 'Y': (220, 215, 190), 'b': (200, 200, 170), 'v': (160, 20, 20), 'k': (20, 20, 20), 'o': (10, 10, 10)}
    for r in range(ROWS):
        for c in range(COLS):
            ch = grid[r][c]
            if ch == '.': continue
            d.rectangle([c * S + 2, r * S + 2, c * S + S - 3, r * S + S - 3], fill=col.get(ch, (255, 0, 255)))
            d.text((c * S + 8, r * S + 6), ch, fill=(0, 0, 0))
    img.save(root / 'capturas' / 'analise_mapa' / 'mapa_caminho_novo.png')
except Exception as e:
    print('sem previa:', e)
