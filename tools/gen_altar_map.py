"""Gera include/map_altar.h: O Altar Negro (mapa 3).
Entrada ao sul -> patio de sangue -> alas oeste/leste e camara noroeste com os 3 SELOS ('*').
Ao norte, o santuario do pentagrama: depois dos 3 selos, Malakor surge no centro ('B').
Legenda: # muro  . chao  E entrada  * selo  B ponto do chefe  P pentagrama  H trono  M espinhos de rocha
         F braseiro  L lago de lava (machuca)  f fenda de fogo (machuca)  l fissura de lava  r rochas
         h enforcado  x estaca de cranios  g jaula  u estatua demoniaca
         s/S/Z hordas  a emboscada  C bau  Y ossos  b/v/k/o manchas
"""
from collections import deque
from pathlib import Path

COLS, ROWS = 26, 36
g = [["#"] * COLS for _ in range(ROWS)]


def carve(c0, r0, c1, r1, ch="."):
    for r in range(r0, r1 + 1):
        for c in range(c0, c1 + 1):
            g[r][c] = ch


def put(c, r, ch):
    assert g[r][c] != "#", (c, r, ch)
    g[r][c] = ch


# --- salas ---
carve(10, 31, 15, 34)            # camara de entrada
carve(11, 27, 14, 30)            # corredor sul
carve(4, 19, 21, 26)             # patio de sangue
carve(1, 10, 6, 18)              # ala oeste (selo 1)
carve(2, 18, 5, 21)              # ligacao ala oeste <-> patio
carve(19, 10, 24, 18)            # ala leste (selo 2)
carve(20, 18, 23, 21)            # ligacao ala leste <-> patio
carve(1, 2, 6, 7)                # camara noroeste (selo 3)
carve(3, 7, 4, 10)               # ligacao noroeste <-> ala oeste
carve(11, 13, 14, 18)            # corredor do santuario
carve(8, 1, 17, 12)              # santuario do pentagrama

# --- entrada ---
put(12, 33, "E")
put(10, 34, "Y"); put(15, 31, "v")

# --- patio de sangue ---
for c, r in [(5, 20), (20, 20), (5, 25), (20, 25)]: put(c, r, "F")
put(7, 24, "s"); put(17, 21, "S"); put(12, 22, "Z")
put(9, 21, "a"); put(16, 25, "a")
put(4, 26, "C"); put(21, 19, "C")
for c, r in [(8, 19), (14, 26), (19, 23), (6, 22), (11, 25)]: put(c, r, "Y")
for c, r in [(10, 23), (15, 20), (18, 24)]: put(c, r, "v")
put(13, 24, "o"); put(6, 26, "~")

# --- inferno: lava, fogo e decoracao ---
for c, r in [(5, 22), (19, 22), (9, 25), (16, 20)]: put(c, r, "L")       # lagos de lava no patio
for c, r in [(12, 30), (13, 17), (3, 20), (22, 20)]: put(c, r, "f")      # fendas de fogo nos corredores
for c, r in [(11, 31), (14, 32), (7, 20), (18, 26), (2, 12), (23, 12)]: put(c, r, "r")
for c, r in [(4, 19), (21, 25), (1, 14), (24, 14)]: put(c, r, "h")
for c, r in [(10, 26), (15, 19), (6, 10), (19, 10)]: put(c, r, "x")
for c, r in [(8, 26), (17, 19)]: put(c, r, "g")
for c, r in [(10, 32), (15, 33)]: put(c, r, "u")
for c, r in [(10, 20), (14, 23), (20, 24)]: put(c, r, "l")

# --- corredor sul ---
put(12, 29, "a"); put(13, 28, "b")

# --- ala oeste: selo 1 ---
put(3, 13, "*"); put(2, 16, "S"); put(5, 11, "s"); put(1, 18, "C")
put(6, 15, "Y"); put(1, 11, "k"); put(5, 17, "b")

# --- ala leste: selo 2 ---
put(22, 13, "*"); put(23, 16, "S"); put(20, 11, "s"); put(24, 18, "C")
put(19, 15, "Y"); put(24, 11, "k"); put(20, 17, "v")

# --- camara noroeste: selo 3 ---
put(3, 4, "*"); put(5, 6, "Z"); put(1, 2, "C"); put(6, 2, "Y"); put(2, 7, "v")
put(4, 9, "a")

# --- corredor do santuario ---
put(12, 16, "Z"); put(13, 14, "b")

# --- santuario ---
carve(10, 5, 15, 9, "P")
put(12, 7, "B")
put(12, 2, "H")
for c, r in [(8, 1), (17, 1), (8, 12), (17, 12)]: put(c, r, "M")
for c, r in [(9, 4), (16, 4), (9, 10), (16, 10)]: put(c, r, "F")
put(11, 11, "Y"); put(14, 3, "v"); put(10, 2, "Y"); put(15, 11, "o")
for c, r in [(9, 6), (16, 8)]: put(c, r, "L")                             # lava ao lado do pentagrama
put(10, 11, "x"); put(15, 2, "x"); put(9, 8, "g"); put(16, 6, "h")

rows = ["".join(r) for r in g]

# --- validacao: tudo alcancavel a partir da entrada ---
walk = lambda ch: ch != "#"
start = next((c, r) for r, row in enumerate(rows) for c, ch in enumerate(row) if ch == "E")
seen = {start}; q = deque([start])
while q:
    c, r = q.popleft()
    for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        n = (c + dc, r + dr)
        if 0 <= n[0] < COLS and 0 <= n[1] < ROWS and n not in seen and walk(rows[n[1]][n[0]]):
            seen.add(n); q.append(n)
for r, row in enumerate(rows):
    for c, ch in enumerate(row):
        if ch in "*BCsSZa":
            assert (c, r) in seen, f"inalcancavel: {ch} em {c},{r}"
assert sum(row.count("*") for row in rows) == 3

out = Path(__file__).resolve().parents[1] / "include" / "map_altar.h"
with open(out, "w") as f:
    f.write("// Gerado por tools/gen_altar_map.py - nao editar a mao\n#pragma once\n")
    f.write(f"#define MAP_ALT_COLS {COLS}\n#define MAP_ALT_ROWS {ROWS}\n")
    f.write("static const char *const MAP_ALT[] = {\n")
    for row in rows:
        f.write(f'    "{row}",\n')
    f.write("};\n")
print("\n".join(rows))
print("ok:", out)
