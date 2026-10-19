"""Exporta as folhas de CORRIDA do druida (Sprites/Druid Run <Left|Front|Back> 4 Frames.png) em
quadros 72x72 com UMA escala por folha (a mesma nos 4 quadros), calibrada para as tres vistas terem o mesmo tamanho.

Antes cada folha era encaixada num quadro 64x64 com escala propria, e os quadros mais largos eram
encolhidos para caber: por isso uns saiam menores que outros. Aqui a escala e a mesma em tudo (os
desenhos ja guardam a proporcao correta entre os quadros); so o alinhamento muda:
  - pes na linha FOOT_ROW (68 = mesma folga de 4px dos outros quadros 72x72);
  - horizontal pelo centro de massa do torso (a capa que voa nao empurra o corpo de lado).
A escala de cada folha e calibrada para a altura media da cabeca ser ~47 px (idle = 49).
Aceita folhas com QUALQUER numero de quadros (4 a 16): procura "Sprites/Druid Run <Left|Front|Back> N Frames.png"
(usa o maior N encontrado), le as figuras em ordem de leitura (linhas de cima p/ baixo, esquerda p/ direita)
e grava include/anim_frames.h com a contagem de cada vista.
Se a folha tiver menos quadros que TARGET_FRAMES (12), cria quadros INTERMEDIARIOS entre cada par
(fluxo optico + mistura, em 4x e reduzido), tratando o ciclo como circular (ultimo -> primeiro).
Uso:  python tools/export_druid_run.py            (todas as vistas)
      python tools/export_druid_run.py front back (so as citadas)
"""
import sys
from pathlib import Path
import numpy as np
import cv2
from PIL import Image, ImageDraw
from inspect_sprite_components import components

ROOT = Path(__file__).resolve().parents[1]
CANVAS, FOOT_ROW, ANCHOR_X = 72, 68, 36
TARGET_FRAMES = 12   # quadros por ciclo no jogo (4 originais + intermediarios)
UP = 4
# Trava parcial da oscilacao vertical (0 = como pintado, 1 = cabeca sempre na mesma altura). De costas as
# pernas ficam escondidas pela capa e o sobe-e-desce do corpo parece um pulo sem pernas.
BOB_LOCK = {"left": 0.0, "front": 0.6, "back": 0.6}
# Poses descartadas do ciclo (indices na folha). De costas as poses 1 e 3 sao as "encolhidas" (pernas
# juntas, escondidas pela capa) e davam a impressao de pulo sem pernas.
DROP_POSES = {}
# Frente/costas: as folhas tem so UMA perna a frente (quadros 0 e 2 sao quase iguais). O ciclo completo
# usa as pernas ESPELHADAS (so as pernas: cajado, ombreira e capa de cima ficam no lugar) para fazer o
# passo da outra perna:  contato A, descida, joelho subindo | contato B, descida, joelho subindo.
# (indice na folha, pernas espelhadas?)
# O quadro 3 das folhas (as duas pernas no alto) NAO entra: sempre ha um pe no chao.
CYCLE = {"front": [(0, False), (1, False), (0, True), (1, True)],
         "back":  [(0, False), (1, False), (0, True), (1, True)]}
LEG_CUT, LEG_BAND = 0.56, 0.19   # pernas: abaixo de 56% da altura, ate 19% da altura para cada lado do eixo
# De costas as pernas so aparecem sob a barra da capa e o corpo encolhe/estica a cada quadro. Aqui o corpo
# (tudo acima da barra) vem SEMPRE do quadro de referencia (estavel, sem pulo) e so as pernas vem de cada
# quadro da folha (esticadas para a altura da referencia): o corpo fica parado e as pernas alternam.
LEG_TRANSPLANT = {}
HEAD_MEAN = 47   # media do ciclo (o idle parado tem 49; a corrida inclina o corpo um pouco)
VIEWS = {"left": "Druid Run Left", "front": "Druid Run Front", "back": "Druid Run Back"}
MACROS = {"left": "NF_DR_MOVE_SIDE", "front": "NF_DR_MOVE_FRONT", "back": "NF_DR_MOVE_BACK"}


def find_sheet(prefix):
    """Maior folha '<prefix> N Frames.png' encontrada em Sprites/. Devolve (nome, N)."""
    best = None
    for f in (ROOT / "Sprites").glob(f"{prefix} * Frames*.png"):
        parts = f.stem.replace(prefix, "").split()
        if parts and parts[0].isdigit() and "Alt" not in f.stem:
            n = int(parts[0])
            if best is None or n > best[1]:
                best = (f.stem, n)
    if best is None:
        raise FileNotFoundError(f"nenhuma folha '{prefix} N Frames.png' em Sprites/")
    return best


def leg_mask(a, cx):
    """Mascara das pernas (calca/botas) abaixo da capa, pela cor: marrom = perna, oliva = capa."""
    H, W = a.shape[:2]
    y0, bw, c = int(LEG_CUT * H), int(LEG_BAND * H), int(round(cx))
    op = a[..., 3] >= 100
    hsv = cv2.cvtColor(np.ascontiguousarray(a[..., :3]), cv2.COLOR_RGB2HSV).astype(int)
    h, s, v = hsv[..., 0], hsv[..., 1], hsv[..., 2]
    region = np.zeros((H, W), bool)
    region[y0:, max(0, c - bw):min(W, c + bw + 1)] = True
    legc = op & (h <= 25) & (s >= 35) & (v >= 35)
    clkc = op & (h >= 29) & (h <= 60) & (s >= 35) & (v >= 35)
    # contorno/sombra (sem cor definida) fica com o rotulo do nucleo mais proximo
    dl = cv2.distanceTransform((~legc).astype(np.uint8), cv2.DIST_L2, 3)
    dc = cv2.distanceTransform((~clkc).astype(np.uint8), cv2.DIST_L2, 3)
    leg = op & region & (legc | (~clkc & (dl < dc)))
    n, lab, st, _ = cv2.connectedComponentsWithStats(leg.astype(np.uint8), 8)
    keep = np.zeros_like(leg)
    for i in range(1, n):
        if st[i, cv2.CC_STAT_AREA] >= 0.004 * H * H:
            keep |= lab == i
    keep = cv2.morphologyEx(keep.astype(np.uint8), cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8)) > 0
    return keep & op & region


def mirror_legs(pose, cx):
    """Espelha SO as pernas em volta do eixo do torso (troca a perna que esta a frente)."""
    a = np.asarray(pose).copy()
    H, W = a.shape[:2]
    c = int(round(cx))
    lm = leg_mask(a, cx)
    xs = np.arange(W)
    sx = 2 * c - xs
    ok = (sx >= 0) & (sx < W)
    mlm = np.zeros_like(lm)
    ma = np.zeros_like(a)
    mlm[:, ok] = lm[:, sx[ok]]
    ma[:, ok] = a[:, sx[ok]]
    out = a.copy()
    hole = lm & ~mlm                 # onde a perna saiu e a espelhada nao cobre: capa do lado oposto
    out[hole] = ma[hole]
    out[mlm] = ma[mlm]
    return Image.fromarray(out, "RGBA")


def reading_order(parts):
    """Ordena as figuras em linhas (de cima p/ baixo) e, dentro da linha, da esquerda p/ direita."""
    parts = sorted(parts, key=lambda p: (p[1] + p[3]) / 2)
    med_h = sorted(p[3] - p[1] for p in parts)[len(parts) // 2]
    rows, cur = [], [parts[0]]
    for p in parts[1:]:
        if abs((p[1] + p[3]) / 2 - (cur[-1][1] + cur[-1][3]) / 2) < med_h * 0.5: cur.append(p)
        else: rows.append(cur); cur = [p]
    rows.append(cur)
    return [p for r in rows for p in sorted(r, key=lambda p: p[0])]


def load(name, n):
    src = Image.open(ROOT / "Sprites" / f"{name}.png").convert("RGBA")
    comps = components(np.asarray(src.getchannel("A")) >= 100, with_runs=True)
    big = max(p[4] for p in comps)
    parts = reading_order([p for p in comps if p[4] > big * 0.12])
    if len(parts) != n:
        raise ValueError(f"{name}: esperava {n} figuras, achei {len(parts)} (figuras coladas ou capa solta?)")
    poses = []
    for part in parts:
        x0, y0, x1, y1 = part[:4]
        pose = src.crop((x0, y0, x1, y1))
        mask = Image.new("L", pose.size)
        d = ImageDraw.Draw(mask)
        for left, right, y in part[5]:
            d.line((left - x0, y - y0, right - x0 - 1, y - y0), fill=255)
        pose.putalpha(mask)
        a = np.asarray(mask) > 0
        h = a.shape[0]
        band = a[int(.35 * h):int(.65 * h)]
        cx = float(np.nonzero(band)[1].mean())      # centro de massa do torso
        poses.append((pose, cx))
    return poses


def render(pose, cx, scale, sy=1.0):
    w, h = round(pose.width * scale), round(pose.height * scale * sy)
    im = pose.resize((w, h), Image.Resampling.LANCZOS)
    im.putalpha(im.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
    frame = Image.new("RGBA", (CANVAS, CANVAS))
    frame.alpha_composite(im, (round(ANCHOR_X - cx * scale), FOOT_ROW - h))
    return frame


def head_height(frame):
    counts = (np.asarray(frame.getchannel("A")) > 0).sum(axis=1)
    top = next((y for y in range(CANVAS) if counts[y] >= 7), 0)
    return FOOT_ROW - top


def _prep(img):
    im = img.resize((img.width * UP, img.height * UP), Image.Resampling.LANCZOS)
    return np.asarray(im).astype(np.float32)


def _gray(a):
    al = a[..., 3:] / 255
    g = cv2.cvtColor(np.clip(a[..., :3] * al + 90 * (1 - al), 0, 255).astype(np.uint8), cv2.COLOR_RGB2GRAY)
    return cv2.GaussianBlur(g, (0, 0), 2)


def _flow(a, b):
    return cv2.calcOpticalFlowFarneback(_gray(a), _gray(b), None, 0.5, 5, 41, 5, 7, 1.5, 0)


def _warp(img, fl, t):
    h, w = img.shape[:2]
    gx, gy = np.meshgrid(np.arange(w), np.arange(h))
    pm = img.copy()
    pm[..., :3] *= pm[..., 3:] / 255
    return cv2.remap(pm, (gx - fl[..., 0] * t).astype(np.float32), (gy - fl[..., 1] * t).astype(np.float32),
                     cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT, borderValue=0)


def between(a, b, t):
    """Quadro intermediario entre as poses a e b (arrays 4x), t em (0,1)."""
    m = (1 - t) * _warp(a, _flow(a, b), t) + t * _warp(b, _flow(b, a), 1 - t)
    al = m[..., 3:]
    rgb = m[..., :3] / np.maximum(al / 255, 1e-3)
    im = Image.fromarray(np.concatenate([np.clip(rgb, 0, 255), al], -1).astype(np.uint8), "RGBA")
    arr = np.asarray(im.resize((im.width // UP, im.height // UP), Image.Resampling.BOX)).copy()
    arr[..., 3] = np.where(arr[..., 3] >= 110, 255, 0)
    arr[arr[..., 3] == 0, :3] = 0
    return Image.fromarray(arr, "RGBA")


def lock_bob(frames, amount):
    """Desloca cada quadro na vertical para a altura da cabeca variar so (1-amount) do original."""
    if amount <= 0:
        return frames
    hs = [head_height(f) for f in frames]
    mean = sum(hs) / len(hs)
    out = []
    for f, h in zip(frames, hs):
        dy = round((h - mean) * amount)      # quadro alto desce, quadro baixo sobe
        g = Image.new("RGBA", f.size)
        g.alpha_composite(f, (0, dy)) if dy >= 0 else g.alpha_composite(f.crop((0, -dy, f.width, f.height)), (0, 0))
        out.append(g)
    return out


def _hem_info(f):
    a = np.asarray(f.getchannel("A")) > 0
    rows = np.nonzero(a.any(1))[0]
    top, bot = rows[0], rows[-1]
    w = a.sum(1)
    hem = max(y for y in range(bot + 1) if w[y] > 16)           # ultima linha larga = barra da capa
    band = a[int(.35 * (bot - top) + top):int(.65 * (bot - top) + top)]
    tcx = np.nonzero(band)[1].mean()                              # centro do torso
    leg = a[hem + 1:bot + 1]
    lcx = np.nonzero(leg)[1].mean() if leg.any() else tcx          # centro das pernas
    return hem, bot, tcx, lcx


def transplant_legs(frames, ref):
    R = frames[ref]
    hr, br, tr, lr = _hem_info(R)
    out = []
    for f in frames:
        h, b, tc, lc = _hem_info(f)
        g = Image.new("RGBA", f.size)
        g.alpha_composite(R.crop((0, 0, f.width, hr + 1)), (0, 0))
        leg = f.crop((0, h + 1, f.width, b + 1))
        newh = br - hr
        if newh > 0 and leg.height > 0:
            leg = leg.resize((f.width, newh), Image.Resampling.NEAREST)
            g.alpha_composite(leg, (round(tr + (lc - tc) - lc), hr + 1))
        out.append(g)
    return out


def build_frames(key, poses, scale):
    frames = [render(p, c, scale) for p, c in poses]
    amt = BOB_LOCK.get(key, 0.0)
    if amt > 0:
        # Reduz o sobe-e-desce ACHATANDO/ESTICANDO de leve na vertical (pes sempre no chao),
        # em vez de deslocar o quadro (o que afundava ou levantava os pes).
        hs = [head_height(f) for f in frames]
        mean = sum(hs) / len(hs)
        frames = [render(p, c, scale, (mean + (h - mean) * (1 - amt)) / h) for (p, c), h in zip(poses, hs)]
    if key in LEG_TRANSPLANT:
        frames = transplant_legs(frames, LEG_TRANSPLANT[key])
    return frames


def densify(frames, target):
    """4 quadros -> target quadros (target multiplo de len(frames))."""
    n = len(frames)
    if n >= target or target % n:
        return frames
    k = target // n - 1
    big = [_prep(f) for f in frames]
    out = []
    for i in range(n):
        out.append(frames[i])
        for j in range(1, k + 1):
            out.append(between(big[i], big[(i + 1) % n], j / (k + 1)))
    return out


def main():
    out = ROOT / "assets" / "sprites"
    counts = {}
    only = set(sys.argv[1:])          # ex.: python tools/export_druid_run.py front back
    for key, prefix in VIEWS.items():
        if only and key not in only:
            continue
        name, n = find_sheet(prefix)
        poses = [pc for i, pc in enumerate(load(name, n)) if i not in DROP_POSES.get(key, ())]
        if key in CYCLE:
            raw = load(name, n)
            poses = [((mirror_legs(*raw[i]) if m else raw[i][0]), raw[i][1]) for i, m in CYCLE[key]]
        n = len(poses)
        counts[key] = n
        # Uma escala por FOLHA (igual para todos os quadros, a oscilacao natural da corrida fica), tal
        # que a altura media da cabeca seja HEAD_MEAN: as tres vistas ficam do mesmo tamanho.
        scale = 0.05
        while scale < 0.2:
            mean = sum(head_height(f) for f in build_frames(key, poses, scale)) / n
            if mean >= HEAD_MEAN:
                break
            scale += 0.0005
        print(f"{key}: {name} ({n} quadros) escala {scale:.4f}")
        frames = build_frames(key, poses, scale)
        frames = densify(frames, TARGET_FRAMES)
        counts[key] = len(frames)
        for i, frame in enumerate(frames):
            bbox = frame.getchannel("A").getbbox()
            if bbox is None or bbox[0] <= 0 or bbox[2] >= CANVAS or bbox[1] <= 0:
                print("  AVISO: quadro encosta na borda/vazio", key, i, bbox)
            frame.save(out / f"spr_druid_run_{key}_d0_f{i}.rgba16.png", optimize=True)
        print("  quadros gravados:", len(frames))
    hdr = ROOT / "include" / "anim_frames.h"
    text = hdr.read_text()
    import re
    for key, macro in MACROS.items():
        if key not in counts:
            continue
        text = re.sub(rf"(#define {macro}\s+)\d+", rf"\g<1>{counts[key]}", text)
    hdr.write_text(text)
    print("anim_frames.h:", counts)


if __name__ == "__main__":
    main()
