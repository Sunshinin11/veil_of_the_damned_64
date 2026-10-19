"""Importa as pranchetas da Cacadora (Sprites/DLC) para o jogo:
  Cacadora <Acao> <Vista> 4 Quadros.png  -> assets/sprites/spr_hunt_<acao>_d<dir>_f<0..3>.rgba16.png (72x72)
     acoes: idle, run (Correndo), attack (Atacando), hurt (Dano), death (Morrendo), roll (Rolando)
     dir: 0 Frente, 1 Diagonal Cima Direita, 2 Esquerda (a direita e espelhada no jogo), 3 Costas
  Poder Gaviao Voo <Vista>               -> spr_hawk_d<dir>_f*.rgba16.png (56x48)
  Poder Foco / Raizes / Manto            -> spr_hunt_focus / spr_hunt_roots / spr_hunt_camo _d0_f*.rgba16.png
  Cacadora Flecha Projetil               -> spr_arrow_d0_f0.rgba16.png (apontando para a direita)
  + icones do HUD (spr_hud_hicon_0..4, 30x30) e o retrato das cartas da cacadora (card_24..47, provisorio)
Todas as poses usam a MESMA escala (altura do idle = 60 px), pes no fundo do quadro e o eixo
das pernas centralizado (o arco esticado nao empurra o corpo para o lado).
Uso:  python tools/import_huntress.py
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageEnhance
import cv2

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Sprites" / "DLC"
OUT = ROOT / "assets" / "sprites"
ACTIONS = {"idle": "Idle", "run": "Correndo", "attack": "Atacando", "hurt": "Dano", "death": "Morrendo", "roll": "Rolando"}
VIEWS = {0: "Frente", 1: "Diagonal Cima Direita", 2: "Esquerda", 3: "Costas"}
IDLE_PX = 60.0


def figures(path, n=4):
    """Separa os n desenhos da folha (componentes conectados, agrupados por coluna)."""
    img = np.asarray(Image.open(path).convert("RGBA"))
    al = img[..., 3]
    H, W = al.shape
    mask = (al > 40).astype(np.uint8)
    k = np.ones((9, 9), np.uint8)
    cnt, lab, st, cen = cv2.connectedComponentsWithStats(cv2.dilate(mask, k), 8)
    comps = [(i, st[i, 4], cen[i][0]) for i in range(1, cnt) if st[i, 4] > 0]
    if not comps:
        return []
    big = max(c[1] for c in comps)
    comps = [c for c in comps if c[1] > big * 0.002]
    cw = W / n
    groups = [[] for _ in range(n)]
    for i, area, cx in comps:              # cada pedaco vai para a coluna do seu centro
        groups[min(n - 1, int(cx // cw))].append(i)
    out = []
    for g in groups:
        if not g:
            out.append(None); continue
        m = np.isin(lab, g) & (al > 10)
        ys, xs = np.nonzero(m)
        x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
        a = img[y0:y1, x0:x1].copy()
        a[~m[y0:y1, x0:x1], 3] = 0
        out.append(Image.fromarray(a, "RGBA"))
    return out


def shrink(fig, s):
    w, h = max(1, round(fig.width * s)), max(1, round(fig.height * s))
    arr = np.asarray(fig).astype(np.float32)
    arr[..., :3] *= arr[..., 3:] / 255
    small = np.asarray(Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA").resize((w, h), Image.Resampling.LANCZOS)).astype(np.float32)
    al = small[..., 3:]
    small[..., :3] = np.where(al > 0, small[..., :3] * 255 / np.maximum(al, 1), 0)
    small[..., 3] = np.where(small[..., 3] < 70, 0, 255)      # rgba16: alfa de 1 bit
    return Image.fromarray(small.clip(0, 255).astype(np.uint8), "RGBA")


def shrink_hero(fig, s, thr=150):
    """Reducao da personagem pensada para tela pequena: corta as franjas finas da capa (alfa alto),
    mais cor/contraste e um contorno escuro de 1 px (como as pranchetas do druida)."""
    from PIL import ImageEnhance, ImageFilter
    w, h = max(1, round(fig.width * s)), max(1, round(fig.height * s))
    arr = np.asarray(fig).astype(np.float32)
    arr[..., :3] *= arr[..., 3:] / 255
    sm = np.asarray(Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA").resize((w, h), Image.Resampling.LANCZOS)).astype(np.float32)
    al = sm[..., 3:]
    sm[..., :3] = np.where(al > 0, sm[..., :3] * 255 / np.maximum(al, 1), 0)
    m = sm[..., 3] >= thr
    rgb = Image.fromarray(sm.clip(0, 255).astype(np.uint8), "RGBA").convert("RGB")
    rgb = ImageEnhance.Color(rgb).enhance(1.35)
    rgb = ImageEnhance.Contrast(rgb).enhance(1.2)
    rgb = ImageEnhance.Brightness(rgb).enhance(1.12)
    rgb = rgb.filter(ImageFilter.UnsharpMask(0.8, 80, 1))
    out = np.dstack([np.asarray(rgb), np.where(m, 255, 0).astype(np.uint8)])
    ring = (cv2.dilate(m.astype(np.uint8), np.ones((3, 3), np.uint8)) > 0) & ~m
    out[ring] = [18, 14, 10, 255]
    return Image.fromarray(out, "RGBA")


def place(fig, canvas, foot_center=True, bottom=1):
    W, H = canvas
    out = Image.new("RGBA", canvas, (0, 0, 0, 0))
    a = np.asarray(fig)[..., 3] > 0
    if foot_center and a.any():
        rows = np.nonzero(a.any(1))[0]
        band = a[max(rows[-1] - max(3, fig.height // 4), 0):rows[-1] + 1]
        xs = np.nonzero(band.any(0))[0]
        cx = (xs.min() + xs.max() + 1) / 2 if len(xs) else fig.width / 2
    else:
        cx = fig.width / 2
    x = int(round(W / 2 - cx))
    y = H - bottom - fig.height if foot_center else (H - fig.height) // 2
    out.alpha_composite(fig, (max(-fig.width, x), max(-fig.height, y))) if x >= 0 and y >= 0 else out.paste(fig, (x, y), fig)
    return out


def sheet(name):
    p = SRC / f"{name}.png"
    if not p.exists():                       # v2 incompleta: usa a prancheta antiga dessa acao/vista
        p = ROOT / "Sprites" / "DLC" / f"{name}.png"
    if not p.exists():
        raise SystemExit(f"faltando: {p.name}")
    figs = figures(p)
    for i in range(len(figs)):            # coluna vazia: repete o quadro vizinho
        if figs[i] is None:
            figs[i] = next((figs[j] for j in list(range(i - 1, -1, -1)) + list(range(i + 1, len(figs))) if figs[j] is not None), None)
    return figs


def run():
    # escala unica pela altura do idle de frente
    idle = [f for f in sheet("Cacadora Idle Frente 4 Quadros") if f]
    s = IDLE_PX / np.median([f.height for f in idle])
    n = 0
    v2 = (ROOT / "Sprites" / "Cacadora_v2" / "Huntress Idle.png").exists()
    for act, label in ({} if v2 else ACTIONS).items():   # v2: a personagem vem do build_sprite_assets.py
        for d, view in VIEWS.items():
            figs = sheet(f"Cacadora {label} {view} 4 Quadros")
            for i, f in enumerate(figs):
                if f is None:
                    f = figs[i - 1]
                small = shrink_hero(f, s)
                if small.width > 70 or small.height > 70:   # morte deitada / rolagem: cabe no quadro (com o contorno)
                    small = shrink_hero(f, s * min(70 / small.width, 70 / small.height))
                place(small, (72, 72)).save(OUT / f"spr_hunt_{act}_d{d}_f{i}.rgba16.png", optimize=True)
                n += 1
    # Usa o extrator dedicado: a separacao antiga juntava duas aves no mesmo quadro.
    from build_hawk_assets import build as build_hawk
    build_hawk()
    # efeitos (4 quadros, uma vista)
    for key, name, canvas, fit in (("focus", "Poder Foco Cacador 4 Quadros", (48, 48), 46),
                                   ("roots", "Poder Raizes Armadilha 4 Quadros", (64, 48), 62),
                                   ("camo", "Poder Manto Camuflagem 4 Quadros", (64, 80), 62)):
        figs = sheet(name)
        fs = fit / max(max(f.width, f.height * (canvas[0] / canvas[1])) for f in figs if f)
        for i, f in enumerate(figs):
            place(shrink(f, fs), canvas, foot_center=(key != "focus")).save(OUT / f"spr_hunt_{key}_d0_f{i}.rgba16.png", optimize=True)
    # flecha: o desenho inteiro (cruza as 4 colunas)
    img = np.asarray(Image.open(SRC / "Cacadora Flecha Projetil.png").convert("RGBA"))
    ys, xs = np.nonzero(img[..., 3] > 40)
    arrow = Image.fromarray(img[ys.min():ys.max() + 1, xs.min():xs.max() + 1], "RGBA")
    a = shrink(arrow, 34.0 / arrow.width)
    canvas = Image.new("RGBA", (36, 10), (0, 0, 0, 0))
    canvas.alpha_composite(a, ((36 - a.width) // 2, (10 - a.height) // 2))
    big = Image.new("RGBA", (36, 36), (0, 0, 0, 0))
    big.alpha_composite(canvas, (0, 13))
    for k in range(8):   # 8 direcoes de tela (0 = direita, sentido anti-horario de 45 em 45 graus)
        r = big.rotate(k * 45, resample=Image.Resampling.BICUBIC)
        ra = np.asarray(r).copy(); ra[..., 3] = np.where(ra[..., 3] < 90, 0, 255)
        Image.fromarray(ra, "RGBA").save(OUT / f"spr_arrow_d0_f{k}.rgba16.png", optimize=True)
    canvas.save(OUT / "spr_arrow_flat.rgba16.png", optimize=True)
    icons()
    print(f"cacadora: {n} quadros + gaviao, efeitos, flecha e icones")
    # 12 quadros suaves (fluxo optico), igual ao druida
    import subprocess, sys as _s
    subprocess.run([_s.executable, str(Path(__file__).resolve().parent / "densify_anims.py"),
                    "hunt_idle", "hunt_run", "hunt_attack", "hunt_hurt", "hunt_death", "hunt_roll"], check=False)


def icon_from(img, size=30):
    from hud_icon_frame import frame_icon   # moldura padrao (a mesma dos icones do Druida)
    return frame_icon(img)
    a = np.asarray(img)
    ys, xs = np.nonzero(a[..., 3] > 40)
    img = Image.fromarray(a[ys.min():ys.max() + 1, xs.min():xs.max() + 1], "RGBA")
    sm = shrink(img, (size - 4) / max(img.width, img.height))
    out = Image.new("RGBA", (size, size), (14, 18, 12, 255))
    out.alpha_composite(sm, ((size - sm.width) // 2, (size - sm.height) // 2))
    return out


def icons():
    """Icones provisorios do cinto (tiro, rajada, raizes, gaviao, foco) a partir das proprias pranchetas."""
    f_att = sheet("Cacadora Atacando Direita 4 Quadros")[2]
    f_arrow = Image.open(OUT / "spr_arrow_flat.rgba16.png").convert("RGBA").resize((72, 20), Image.Resampling.NEAREST)
    rajada = Image.new("RGBA", (80, 80), (0, 0, 0, 0))
    for k, ang in enumerate((-25, 0, 25)):
        rajada.alpha_composite(f_arrow.rotate(ang, expand=True, resample=Image.Resampling.BICUBIC).resize((76, 40)), (2, 4 + k * 18))
    src = [f_att, rajada, sheet("Poder Raizes Armadilha 4 Quadros")[3],
           sheet("Poder Gaviao Voo Direita 4 Quadros")[0], sheet("Poder Foco Cacador 4 Quadros")[2]]
    for i, im in enumerate(src):
        icon_from(im).save(OUT / f"spr_hud_hicon_{i}.rgba16.png", optimize=True)
    # retratos provisorios das 24 cartas da cacadora (64x80): pose/efeito do tipo da carta, tingido
    poses = {"attack": sheet("Cacadora Atacando Esquerda 4 Quadros")[2], "trap": sheet("Poder Raizes Armadilha 4 Quadros")[3],
             "hawk": sheet("Poder Gaviao Voo Frente 4 Quadros")[1], "body": sheet("Cacadora Idle Frente 4 Quadros")[0],
             "focus": sheet("Poder Foco Cacador 4 Quadros")[2], "camo": sheet("Poder Manto Camuflagem 4 Quadros")[2]}
    kinds = ["attack", "trap", "trap", "hawk", "body", "body", "body", "body",
             "attack", "attack", "trap", "hawk", "hawk", "attack", "focus", "body", "body", "camo",
             "attack", "hawk", "trap", "attack", "body", "focus"]
    tints = [(1.0, 1.0, 1.0), (0.85, 1.1, 0.8), (1.1, 0.95, 0.75), (0.8, 0.95, 1.15)]
    for k, kind in enumerate(kinds):
        base = poses[kind]
        a = np.asarray(base)
        ys, xs = np.nonzero(a[..., 3] > 40)
        base = Image.fromarray(a[ys.min():ys.max() + 1, xs.min():xs.max() + 1], "RGBA")
        sm = shrink(base, min(60 / base.width, 76 / base.height))
        t = tints[k % 4]
        arr = np.asarray(sm).astype(np.float32)
        arr[..., 0] *= t[0]; arr[..., 1] *= t[1]; arr[..., 2] *= t[2]
        sm = Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA")
        bg = Image.new("RGBA", (64, 80), (18, 22, 16, 255))
        g = np.zeros((80, 64, 4), np.uint8)
        yy, xx = np.mgrid[0:80, 0:64]
        r = np.sqrt((xx - 32) ** 2 + (yy - 40) ** 2)
        g[..., 1] = np.clip(70 - r * 1.6, 0, 70).astype(np.uint8); g[..., 0] = g[..., 1] // 2; g[..., 3] = 255
        bg = Image.alpha_composite(bg, Image.fromarray(g, "RGBA"))
        bg.alpha_composite(sm, ((64 - sm.width) // 2, (80 - sm.height) // 2))
        out = ROOT / "assets" / f"card_{24 + k:02d}.rgba16.png"
        if not out.exists() or (ROOT / "assets" / f".card_{24 + k:02d}.placeholder").exists():
            bg.save(out, optimize=True)
            (ROOT / "assets" / f".card_{24 + k:02d}.placeholder").write_text("provisorio\n")


if __name__ == "__main__":
    run()
