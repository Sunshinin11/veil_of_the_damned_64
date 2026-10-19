"""Gera include/map_crypt.h: mapa da Cripta Vazia (area 2), salas ligadas por corredores.

Rode de novo depois de mexer:   python tools/gen_crypt_map.py
Cada tile = 72 unidades; linha 0 = norte. A camera mostra so ~2,6 tiles de largura, entao as
salas sao pequenas e os corredores tem 2 tiles. O jogador entra no sul e o portal de saida fica
no salao do trono, no norte (aparece quando os 3 guardioes caem: salas oeste, leste e ossuario).

Legenda (alem das do mapa 1: # muro, E entrada, P portal, s S Z q a C marcadores, U estatua,
b v k o decalques, Y ossos, ~ corpo):
  L coluna     Q sarcofago     A altar de sangue     F braseiro     V nicho de caveiras
  B grade de cela    W correntes na parede    H teia de aranha    I correntes penduradas
  M vitral
"""
from pathlib import Path
from collections import deque

COLS, ROWS = 24, 36
g = [['#'] * COLS for _ in range(ROWS)]


def room(c0, r0, c1, r1):
    for r in range(r0, r1 + 1):
        for c in range(c0, c1 + 1):
            g[r][c] = '.'


def put(c, r, ch):
    assert 0 < c < COLS - 1 and 0 < r < ROWS - 1, (c, r, ch)
    assert g[r][c] == '.', f"({c},{r}) nao e chao: {g[r][c]!r} -> {ch!r}"
    g[r][c] = ch


# ------------------------------------------------------------------ salas e corredores
room(9, 31, 14, 34)        # camara de entrada
room(11, 27, 12, 30)       # corredor A
room(7, 17, 16, 26)        # nave das colunas
room(1, 19, 5, 26)         # sala dos sarcofagos (oeste)
room(6, 22, 6, 23)         # porta oeste da nave
room(18, 19, 22, 26)       # sala do altar de sangue (leste)
room(17, 22, 17, 23)       # porta leste da nave
room(11, 16, 12, 16)       # passagem norte da nave
room(1, 12, 22, 15)        # galeria leste-oeste
room(1, 4, 6, 10)          # prisao (noroeste)
room(3, 11, 4, 11)         # porta da prisao
room(17, 4, 22, 10)        # ossuario (nordeste)
room(19, 11, 20, 11)       # porta do ossuario
room(8, 1, 15, 10)         # salao do trono (norte)
room(11, 11, 12, 11)       # porta do salao

# ------------------------------------------------------------------ camara de entrada
put(12, 33, 'E')
for c, r in [(9, 31), (14, 31), (9, 34), (14, 34)]: put(c, r, 'F')
put(10, 32, 'v'); put(13, 33, 'b'); put(11, 34, 'k')
put(12, 29, 'k'); put(11, 28, 'b')

# ------------------------------------------------------------------ nave das colunas
for r in (18, 20, 25):
    put(8, r, 'L'); put(15, r, 'L')
put(11, 19, 'F'); put(12, 25, 'F')
put(7, 17, 'H'); put(16, 17, 'H'); put(7, 26, 'H')
put(10, 21, 'S'); put(14, 23, 's'); put(9, 24, 's')
put(11, 20, 'a')
put(16, 26, 'C'); put(9, 19, 'C')
put(11, 22, 'v'); put(13, 20, 'k'); put(10, 24, 'b'); put(13, 26, 'o')

# ------------------------------------------------------------------ sarcofagos (oeste) - guardiao 1
for c, r in [(2, 20), (4, 20), (2, 25), (4, 25)]: put(c, r, 'Q')
put(3, 24, 'q'); put(3, 21, 's')
put(1, 19, 'C'); put(5, 26, 'C')
put(1, 22, 'V'); put(1, 23, 'V'); put(5, 19, 'H')
put(4, 22, 'b'); put(3, 23, 'v')

# ------------------------------------------------------------------ altar de sangue (leste) - guardiao 2
put(20, 22, 'A')
put(19, 23, 'v'); put(21, 23, 'v'); put(20, 24, 'v'); put(19, 21, 'v')
put(18, 19, 'F'); put(22, 19, 'F')
put(20, 25, 'q'); put(19, 20, 's'); put(21, 20, 'k')
put(22, 26, 'C'); put(18, 26, 'C'); put(22, 22, 'V'); put(22, 23, 'V')
put(18, 25, '~')

# ------------------------------------------------------------------ galeria
put(6, 12, 'F'); put(17, 12, 'F')
put(4, 13, 'L'); put(8, 14, 'L'); put(15, 13, 'L'); put(19, 14, 'L')
put(2, 12, 'W'); put(21, 12, 'W'); put(10, 12, 'W'); put(13, 12, 'W')
put(6, 14, 's'); put(17, 14, 's'); put(11, 13, 'a')
put(1, 15, 'C'); put(22, 15, 'C'); put(11, 15, 'k')

# ------------------------------------------------------------------ prisao (noroeste)
for c in (1, 2, 5, 6): put(c, 6, 'B')
put(2, 8, 'S')
put(5, 9, 'a'); put(4, 7, 's')
put(1, 4, 'C'); put(6, 10, 'C'); put(2, 10, '~'); put(3, 5, 'I'); put(5, 4, 'I')
put(1, 9, 'W'); put(4, 10, 'b'); put(2, 7, 'k')

# ------------------------------------------------------------------ ossuario (nordeste) - guardiao 3
put(19, 6, 'q'); put(21, 8, 'S')
for c, r in [(17, 5), (20, 4), (22, 7), (18, 9), (21, 5)]: put(c, r, 'Y')
put(17, 7, 'V'); put(22, 9, 'V'); put(19, 9, 'b'); put(20, 8, 'k')
put(22, 4, 'C'); put(17, 10, 'C'); put(18, 4, 'H')

# ------------------------------------------------------------------ salao do trono (saida)
put(12, 5, 'P')
put(12, 2, 'A'); put(9, 1, 'F'); put(14, 1, 'F')
for c, r in [(9, 3), (14, 3), (9, 6), (14, 6), (9, 9), (14, 9)]: put(c, r, 'L')
put(10, 8, 'Z'); put(13, 8, 's'); put(12, 8, 'a')
put(10, 10, 'U'); put(13, 10, 'U')
put(8, 1, 'C'); put(15, 1, 'C')
put(11, 4, 'v'); put(13, 6, 'v'); put(10, 6, 'k'); put(8, 5, 'M'); put(15, 5, 'M')

# ------------------------------------------------------------------ validacao
start = next((c, r) for r in range(ROWS) for c in range(COLS) if g[r][c] == 'E')
seen = {start}
dq = deque([start])
while dq:
    c, r = dq.popleft()
    for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        n = (c + dc, r + dr)
        if g[n[1]][n[0]] != '#' and n not in seen:
            seen.add(n); dq.append(n)
solid = set('LQAVBWHIM')
for r in range(ROWS):
    for c in range(COLS):
        ch = g[r][c]
        if ch != '#' and (c, r) not in seen:
            # props nao caminhaveis encostam em chao alcancavel
            if not any((c + dc, r + dr) in seen for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                raise SystemExit(f"tile inalcancavel: ({c},{r}) {ch!r}")
n_elite = sum(row.count('q') for row in g)
assert n_elite == 3, n_elite
print("elites", n_elite, "| baus", sum(row.count('C') for row in g),
      "| matilhas", sum(row.count(k) for row in g for k in 'sSZ'),
      "| emboscadas", sum(row.count('a') for row in g))

lines = ['// GERADO por tools/gen_crypt_map.py - nao edite a mao.', '#ifndef MAP_CRYPT_H', '#define MAP_CRYPT_H',
         f'#define MAP_CRYPT_COLS {COLS}', f'#define MAP_CRYPT_ROWS {ROWS}', 'static const char *const MAP_CRYPT[] = {']
for row in g:
    lines.append('    "' + ''.join(row) + '",')
lines += ['};', '#endif', '']
root = Path(__file__).resolve().parents[1]
(root / 'include' / 'map_crypt.h').write_text('\n'.join(lines), encoding='utf-8')
print('\n'.join(''.join(r) for r in g))
