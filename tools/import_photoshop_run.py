"""Importa as pranchetas de CORRIDA do druida feitas no Photoshop (ja em 72 px de altura):
  Sprites/Pranchetas Photoshop/Prancheta de frente.png  -> spr_druid_run_front_d0_f*.rgba16.png
  Sprites/Pranchetas Photoshop/Prancheta de costas.png  -> spr_druid_run_back_d0_f*.rgba16.png
  Sprites/Pranchetas Photoshop/Prancheta de lado.png    -> spr_druid_run_left_d0_f*.rgba16.png (virado p/ a esquerda)
Nada e redimensionado nem interpolado: cada figura vai para um quadro 72x72 na MESMA altura em que foi
desenhada (os pes e o sobe-e-desce ficam como no Photoshop); so o eixo do tronco e centralizado em x=36.
Atualiza include/anim_frames.h com o numero de quadros de cada folha.
Uso:  python tools/import_photoshop_run.py
"""
import re
import sys
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parent))
from inspect_sprite_components import components

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "Pranchetas Photoshop"
OUT = ROOT / "assets" / "sprites"
CANVAS, ANCHOR_X = 72, 36
SHEETS = {"front": ("Prancheta de frente.png", "NF_DR_MOVE_FRONT", "RUN_SCALE_FRONT"),
          "back": ("Prancheta de costas.png", "NF_DR_MOVE_BACK", "RUN_SCALE_BACK"),
          "left": ("Prancheta de lado.png", "NF_DR_MOVE_SIDE", "RUN_SCALE_SIDE")}
# Corpo estavel: tudo ACIMA do quadril vem de um unico quadro de referencia (o mais "tipico") e so
# as pernas (abaixo do corte, em fracao da altura da figura) vem de cada quadro, esticadas para ligar
# o quadril da referencia ao pe daquele quadro. Tira os trancos do tronco/capa/cajado.
STABLE_BODY = {}   # desligado: o corte ficou ruim
# Selecao: fica so com os N quadros que formam o ciclo mais suave (sem trancos); os 12 espacos sao
# preenchidos repetindo esses quadros (cada removido vira uma copia do vizinho mais parecido).
KEEP = {"front": 8, "left": 8}
REORDER = True    # reordena os quadros para que vizinhos sejam os mais parecidos (ciclo mais suave)
HEAD_REF = 47.0   # altura de cabeca das folhas antigas, para as quais o jogo foi calibrado
# Frente/costas sao mais estreitas e pareciam menores que as outras animacoes: um pouco maiores.
VIEW_BOOST = {"front": 1.22, "back": 1.32, "left": 1.0}
# Largura extra so da vista de costas (a capa aberta deixava ele "gordinho"): <1 afina sem mudar a altura.
BACK_WIDTH = 1.00
# Altura do CORPO na tela (px de sprite x escala do jogo) igual a das diagonais/idle (~53-55).
TARGET_BODY = 55.0
# Correndo pra cima parecia mais baixo que as diagonais: so mais ALTO (a largura e compensada em BACK_WIDTH).
TARGET_EXTRA = {"back": 1.10, "front": 1.10}
GAME_BOOST = 1.20   # DRUID_SIDE/FRONT/BACK_BOOST em src/sprites.c


def figures(img):
    a = np.asarray(img)
    comps = components(a[..., 3] >= 100, with_runs=True)
    big = max(p[4] for p in comps)
    return sorted((p for p in comps if p[4] > big * 0.12), key=lambda p: p[0])


def frame_dist(a, b):
    """Diferenca entre dois quadros 72x72 (cor sobre cinza + silhueta), com leve desfoque."""
    import cv2
    def prep(f):
        x = np.asarray(f).astype(np.float32)
        al = x[..., 3:] / 255
        rgb = x[..., :3] * al + 90 * (1 - al)
        return cv2.GaussianBlur(np.concatenate([rgb, al * 255], -1), (0, 0), 1.0)
    pa, pb = prep(a), prep(b)
    d = np.abs(pa - pb)
    w = np.ones((72, 1, 1), np.float32)
    w[44:] = 2.0                      # pernas pesam mais: e onde a corrida "pula"
    return float((d * w).mean())


def _span(f):
    a = np.asarray(f.getchannel("A")) > 0
    ys = np.nonzero(a.any(1))[0]
    return ys[0], ys[-1]


def stabilize_body(frames, cut):
    """Mesmo tronco em todos os quadros; pernas de cada quadro."""
    def upper_dist(a, b):
        ta, ba = _span(a); tb, bb = _span(b)
        ya, yb = int(ta + cut * (ba - ta)), int(tb + cut * (bb - tb))
        ua = np.asarray(a)[:ya].astype(np.float32); ub = np.asarray(b)[:yb].astype(np.float32)
        h = min(len(ua), len(ub))
        return float(np.abs(ua[-h:] - ub[-h:]).mean())
    n = len(frames)
    ref = min(range(n), key=lambda i: sum(upper_dist(frames[i], frames[j]) for j in range(n)))
    R = frames[ref]
    rt, rb = _span(R)
    ry = int(rt + cut * (rb - rt))
    out = []
    for f in frames:
        t, b = _span(f)
        y = int(t + cut * (b - t))
        g = Image.new("RGBA", f.size)
        g.alpha_composite(R.crop((0, 0, f.width, ry)), (0, 0))
        legs = f.crop((0, y, f.width, b + 1))
        newh = max(1, b + 1 - ry)                      # do quadril da referencia ate o pe deste quadro
        legs = legs.resize((f.width, newh), Image.Resampling.NEAREST)
        g.alpha_composite(legs, (0, ry))
        out.append(g)
    print(f"   corpo estavel: referencia = quadro {ref + 1} da prancheta")
    return out


def select_cycle(frames, k):
    """Melhor ciclo com k quadros: menor 'maior salto' entre vizinhos (depois menor soma)."""
    import itertools
    n = len(frames)
    D = [[frame_dist(frames[i], frames[j]) for j in range(n)] for i in range(n)]
    best = None
    for sub in itertools.combinations(range(n), k):
        s0, rest = sub[0], sub[1:]
        for perm in itertools.permutations(rest):
            if perm[0] > perm[-1]:
                continue
            cyc = (s0,) + perm
            es = [D[a][b] for a, b in zip(cyc, cyc[1:] + cyc[:1])]
            key = (round(max(es), 2), sum(es))
            if best is None or key < best[0]:
                best = (key, list(cyc))
    order = best[1]
    def fwd(o):
        return sum(1 for a, b in zip(o, o[1:] + o[:1]) if 0 < (b - a) % n <= n // 2)
    rev = [order[0]] + order[1:][::-1]
    if fwd(rev) > fwd(order):
        order = rev
    full = [order[i * k // n] for i in range(n)]
    cur = max(D[i][(i + 1) % n] for i in range(n))
    print(f"   selecao: maior salto {cur:.1f} -> {best[0][0]:.1f}; fora: {[i + 1 for i in range(n) if i not in order]}")
    return full


def best_cycle(frames):
    """Ordem circular de menor diferenca total entre vizinhos (Held-Karp, exato para 12 quadros)."""
    n = len(frames)
    D = [[frame_dist(frames[i], frames[j]) for j in range(n)] for i in range(n)]
    INF = float("inf")
    dp = [[INF] * n for _ in range(1 << n)]
    par = [[-1] * n for _ in range(1 << n)]
    dp[1][0] = 0.0
    for m in range(1 << n):
        if not m & 1:
            continue
        for j in range(n):
            if dp[m][j] == INF or not m >> j & 1:
                continue
            for k in range(n):
                if m >> k & 1:
                    continue
                v = dp[m][j] + D[j][k]
                if v < dp[m | 1 << k][k]:
                    dp[m | 1 << k][k] = v
                    par[m | 1 << k][k] = j
    full = (1 << n) - 1
    last = min(range(1, n), key=lambda j: dp[full][j] + D[j][0])
    order, m, j = [], full, last
    while j != -1:
        order.append(j)
        pj = par[m][j]
        m ^= 1 << j
        j = pj
    order.reverse()
    # sentido: o que mais respeita a ordem em que os quadros foram desenhados (nao correr de costas)
    def fwd(o):
        return sum(1 for a, b in zip(o, o[1:] + o[:1]) if 0 < (b - a) % n <= n // 2)
    rev = [order[0]] + order[1:][::-1]
    if fwd(rev) > fwd(order):
        order = rev
    total = lambda o: sum(D[a][b] for a, b in zip(o, o[1:] + o[:1]))
    print(f"   custo do ciclo: original {total(list(range(n))):.1f} -> novo {total(order):.1f}")
    return order


def main():
    hdr = ROOT / "include" / "anim_frames.h"
    text = hdr.read_text()
    for key, (fname, macro, smacro) in SHEETS.items():
        path = SRC / fname
        if not path.exists():
            print("sem", fname); continue
        img = Image.open(path).convert("RGBA")
        # alfa de 1 bit (RGBA16): o que for meio transparente vira opaco ou some
        a = np.asarray(img).copy()
        a[..., 3] = np.where(a[..., 3] >= 100, 255, 0)
        a[a[..., 3] == 0, :3] = 0
        img = Image.fromarray(a, "RGBA")
        figs = figures(img)
        heads = []
        frames = []
        for i, p in enumerate(figs):
            x0, y0, x1, y1 = p[:4]
            mask = Image.new("L", img.size)
            m = np.zeros((img.height, img.width), np.uint8)
            for left, right, y in p[5]:
                m[y, left:right] = 255
            mask = Image.fromarray(m)
            piece = Image.new("RGBA", img.size)
            piece.paste(img, (0, 0), mask)
            piece = piece.crop((x0, 0, x1 + 1, img.height))          # mantem a altura original
            al = np.asarray(piece.getchannel("A")) > 0
            ys = np.nonzero(al.any(1))[0]
            top, bot = ys[0], ys[-1]
            band = al[int(top + .35 * (bot - top)):int(top + .65 * (bot - top))]
            cx = float(np.nonzero(band)[1].mean())                    # eixo do tronco
            frame = Image.new("RGBA", (CANVAS, CANVAS))
            frame.alpha_composite(piece, (round(ANCHOR_X - cx), CANVAS - img.height))
            rows = np.nonzero((np.asarray(frame.getchannel("A")) > 0).sum(1) >= 7)[0]
            fa = np.asarray(frame.getchannel("A")) > 0
            body = np.nonzero(fa.sum(1) >= 10)[0]
            heads.append(np.nonzero(fa.any(1))[0][-1] - body[0])      # altura do corpo (sem o cajado)
            frames.append(frame)
        if key in STABLE_BODY:
            frames = stabilize_body(frames, STABLE_BODY[key])
        if key in KEEP:
            order = select_cycle(frames, KEEP[key])
        else:
            order = best_cycle(frames) if REORDER else list(range(len(frames)))
        print(f"   ordem: {[o + 1 for o in order]}  (numeros = posicao na prancheta, 1..{len(frames)})")
        for i, o in enumerate(order):
            frames[o].save(OUT / f"spr_druid_run_{key}_d0_f{i}.rgba16.png", optimize=True)
        text = re.sub(rf"(#define {macro}\s+)\d+", rf"\g<1>{len(figs)}", text)
        k = TARGET_BODY * TARGET_EXTRA.get(key, 1.0) / (GAME_BOOST * sum(heads) / len(heads))   # mesma altura na tela que as outras      # mesmo tamanho aparente das outras animacoes
        text = re.sub(rf"(#define {smacro}\s+)[0-9.]+f", rf"\g<1>{k:.3f}f", text)
        print(f"{key}: {len(figs)} quadros, corpo medio {sum(heads)/len(heads):.1f} px, escala {k:.3f}")
    text = re.sub(r"(#define RUN_BACK_WIDTH\s+)[0-9.]+f", rf"\g<1>{BACK_WIDTH:.2f}f", text)
    hdr.write_text(text)


if __name__ == "__main__":
    main()
