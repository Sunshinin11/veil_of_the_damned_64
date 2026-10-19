"""Convert the painted source sheets into small, named libdragon sprites.

Run with the bundled Python/Pillow before make. Sources in Sprites/ are never
modified. The fixed cells deliberately preserve direction and animation order.
"""
from pathlib import Path
from math import ceil
from shutil import copyfile
from PIL import Image, ImageChops, ImageDraw, ImageFilter
import numpy as np
from inspect_sprite_components import components

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "Sprites"
OUT = ROOT / "assets" / "sprites"

# Keep one painted-pixel scale across every action. Attack, spell and escape
# frames may need a larger transparent canvas, but must not shrink the actor.
ACTOR_SCALE = {"druid_": .21, "werewolf_": .195, "malakor_": .20, "hunt_": .21, "barb_": .21}

# Cacadora v2: pranchetas no MESMO formato das do druida (4x4: linhas = direcoes, colunas = quadros).
# Enquanto nao existem, ficam os quadros gerados por tools/import_huntress.py.
HUNTRESS_V2 = {
    "hunt_idle": "Cacadora_v2/Huntress Idle Alt.png",  # costas com um unico arco
    "hunt_run": "Cacadora_v2/Huntress Run Corrected.png",
    "hunt_attack": "Cacadora_v2/Huntress Attack Alt.png",  # Alt: postura de tiro mais firme
    "hunt_hurt": "Cacadora_v2/Huntress Hurt.png",
    "hunt_death": "Cacadora_v2/Huntress Death.png",
    "hunt_roll": "Cacadora_v2/Huntress Escape.png",
}
# Barbaro: mesmo formato (4x4). Enquanto nao existe, o jogo desenha o druida pintado de vermelho.
BARBARIAN = {
    "barb_idle": "Barbaro/Barbarian Idle.png",
    "barb_run": "Barbaro/Barbarian Run.png",
    "barb_attack": "Barbaro/Barbarian Attack.png",
    "barb_attack2": "Barbaro/Barbarian Attack2.png",
    "barb_spin": "Barbaro/Barbarian Whirlwind.png",
    "barb_shout": "Barbaro/Barbarian Shout.png",
    "barb_hurt": "Barbaro/Barbarian Hurt.png",
    "barb_death": "Barbaro/Barbarian Death.png",
    "barb_roll": "Barbaro/Barbarian Escape.png",
}

# name: source, columns, rows, alpha multiplier, output size
SHEETS = {
    "druid_idle": ("Druid Idle.png", 4, 4, 1.0, 64),
    "druid_walk": ("Druid Walk 1.png", 4, 4, 1.0, 64),
    "druid_walk_alt": ("Druid Walk 2.png", 4, 4, 1.0, 64),
    "druid_run": ("Druid Run 1.png", 4, 4, 1.0, 64),
    "druid_run_alt": ("Druid Run 2.png", 4, 4, 1.0, 64),
    "druid_attack": ("Druid Atack 1.png", 4, 4, 1.0, 64),
    "druid_cast": ("Druid Spell 1.png", 4, 4, 1.0, 64),
    "druid_escape": ("Druid Escape 1.png", 4, 4, 1.0, 64),
    "druid_hurt": ("Druid Hurt.png", 4, 4, 1.0, 64),
    "druid_death": ("Druid Death.png", 4, 4, 1.0, 64),
    "druid_transform": ("Druid Transform.png", 4, 4, 1.0, 64),
    "werewolf_idle": ("Werewolf Idle.png", 4, 4, 1.0, 64),
    "werewolf_walk": ("Werewolf Walk.png", 4, 4, 1.0, 64),
    "werewolf_run": ("Werewolf Run.png", 4, 4, 1.0, 64),
    "werewolf_attack": ("Werewolf Attack.png", 4, 4, 1.0, 64),
    "werewolf_cast": ("Werewolf Spell.png", 4, 4, 1.0, 64),
    "werewolf_escape": ("Werewolf Escape.png", 4, 4, 1.0, 64),
    "werewolf_hurt": ("Werewolf Hurt.png", 4, 4, 1.0, 64),
    "werewolf_death": ("Werewolf Death.png", 4, 4, 1.0, 64),
    "skeleton_idle": ("Skeleton Guardian.png", 4, 4, 1.0, 64),
    "skeleton_walk": ("Skeleton Guardian Walk.png", 4, 4, 1.0, 64),
    "skeleton_attack": ("Skeleton Guardian Attack.png", 4, 4, 1.0, 64),
    "skeleton_hurt": ("Skeleton Guardian Hurt Death.png", 4, 4, 1.0, 64),
    "demon_idle": ("Demon Brute.png", 4, 4, 1.0, 64),
    "demon_walk": ("Demon Brute Walk.png", 4, 4, 1.0, 64),
    "demon_attack": ("Demon Brute Attack.png", 4, 4, 1.0, 64),
    "demon_hurt": ("Demon Brute Hurt Death.png", 4, 4, 1.0, 64),
    "cultist_idle": ("Blood Cultist.png", 4, 4, 1.0, 64),
    "cultist_walk": ("Blood Cultist Walk.png", 4, 4, 1.0, 64),
    "cultist_attack": ("Blood Cultist Attack.png", 4, 4, 1.0, 64),
    "cultist_hurt": ("Blood Cultist Hurt Death.png", 4, 4, 1.0, 64),
    "malakor_idle": ("Malakor.png", 4, 4, 1.0, 64),
    "malakor_walk": ("Malakor Walk.png", 4, 4, 1.0, 64),
    "malakor_attack": ("Malakor Attack.png", 4, 4, 1.0, 64),
    "malakor_hurt": ("Malakor Hurt.png", 4, 4, 1.0, 64),
    "malakor_death": ("Malakor Death.png", 4, 4, 1.0, 64),
    "malakor_enrage": ("Malakor Enrage.png", 4, 4, 1.0, 64),
    "wolf_idle": ("Spirit Wolf Idle.png", 4, 4, .64, 64),
    "wolf_run": ("Spirit Wolf Run.png", 4, 4, .64, 64),
    "wolf_attack": ("Spirit Wolf Attack.png", 4, 4, .64, 64),
    "wolf_summon": ("Spirit Wolf Summon.png", 4, 4, .64, 64),
    "spirit_bolt": ("Spirit Bolt FX.png", 4, 4, 1.0, 48),
    "thorn_burst": ("Thorn Burst FX.png", 4, 4, 1.0, 64),
    "dark_orb": ("Dark Blood Orb FX.png", 4, 4, 1.0, 48),
    "item": ("Item Drops.png", 4, 4, 1.0, 48),
    "chest": ("Chest Open.png", 4, 2, 1.0, 64),
}


def content_box(im, cutoff=55):
    alpha = im.getchannel("A").point(lambda a: 255 if a >= cutoff else 0)
    return alpha.getbbox()


def save_sprite(name, im, rgba32=False):
    suffix = "rgba32" if rgba32 else "rgba16"
    im.save(OUT / f"spr_{name}.{suffix}.png", optimize=True)


def sheet(name, filename, cols, rows, opacity, size):
    src = Image.open(SOURCES / filename).convert("RGBA")
    alpha = np.asarray(src.getchannel("A"))
    found = [part for part in components(alpha >= 180, with_runs=True)
             if part[4] > 3000]
    # Duas figuras encostadas viram um componente so: corta o mais alto (ou mais largo)
    # na divisa de celula mais proxima do meio dele, ate dar a contagem esperada.
    while 0 < len(found) < cols * rows:
        med_h = sorted(p[3] - p[1] for p in found)[len(found) // 2]
        med_w = sorted(p[2] - p[0] for p in found)[len(found) // 2]
        big = max(found, key=lambda p: max((p[3] - p[1]) / med_h, (p[2] - p[0]) / med_w))
        vert = (big[3] - big[1]) / med_h >= (big[2] - big[0]) / med_w
        if (max((big[3] - big[1]) / med_h, (big[2] - big[0]) / med_w)) < 1.5:
            break
        step = src.height / rows if vert else src.width / cols
        lo, hi = (big[1], big[3]) if vert else (big[0], big[2])
        cut = round(((lo + hi) / 2) / step) * step
        if not (lo < cut < hi):
            cut = (lo + hi) / 2
        found.remove(big)
        halves = []
        for side in (0, 1):
            if vert:
                runs = [r for r in big[5] if (r[2] < cut) == (side == 0)]
            else:
                runs = [(max(r[0], cut) if side else r[0], min(r[1], cut) if not side else r[1], r[2])
                        for r in big[5] if (r[0] < cut if side == 0 else r[1] > cut)]
                runs = [r for r in runs if r[1] > r[0]]
            if runs:
                halves.append((min(r[0] for r in runs), min(r[2] for r in runs),
                               max(r[1] for r in runs), max(r[2] for r in runs) + 1,
                               sum(r[1] - r[0] for r in runs), runs))
        found.extend(h for h in halves if h[4] > 3000)
    if len(found) != cols * rows:
        raise ValueError(f"{filename}: expected {cols*rows} figures; found {len(found)}")
    # Each figure has its own connected component. Group their centers into
    # rows, then columns; this handles uneven spacing and overlapping boxes.
    ordered = sorted(found, key=lambda p: (p[1] + p[3]) / 2)
    cells = []
    for row in range(rows):
        group = sorted(ordered[row * cols:(row + 1) * cols],
                       key=lambda p: (p[0] + p[2]) / 2)
        for col, part in enumerate(group):
            margin = 10
            x0 = max(0, part[0] - margin)
            y0 = max(0, part[1] - margin)
            x1 = min(src.width, part[2] + margin)
            y1 = min(src.height, part[3] + margin)
            box = (x0, y0, x1, y1)
            image = src.crop(box)
            mask = Image.new("L", image.size)
            draw = ImageDraw.Draw(mask)
            for rx0, rx1, ry in part[5]:
                draw.line((rx0 - x0, ry - y0, rx1 - x0 - 1, ry - y0),
                          fill=255)
            # Keep antialiased edges and nearby spectral glow, but exclude
            # the adjacent frame even when their bounding boxes overlap.
            mask = mask.filter(ImageFilter.MaxFilter(19))
            image.putalpha(ImageChops.multiply(image.getchannel("A"), mask))
            cells.append((row, col, image))
    max_w = max(im.width for _, _, im in cells)
    max_h = max(im.height for _, _, im in cells)
    actor_scale = next((value for prefix, value in ACTOR_SCALE.items()
                        if name.startswith(prefix)), None)
    if actor_scale is None:
        scale = min((size - 4) / max_w, (size - 4) / max_h)
        canvas_size = size
    else:
        scale = actor_scale
        canvas_size = max(size, ceil((max(max_w, max_h) * scale + 4) / 8) * 8)
    for row, col, im in cells:
        w = max(1, round(im.width * scale))
        h = max(1, round(im.height * scale))
        frame = im.resize((w, h), Image.Resampling.LANCZOS)
        a = frame.getchannel("A")
        if opacity < 1:
            a = a.point(lambda v: round(v * opacity) if v >= 28 else 0)
        else:
            a = a.point(lambda v: 255 if v >= 100 else 0)
        frame.putalpha(a)
        canvas = Image.new("RGBA", (canvas_size, canvas_size))
        x = (canvas_size - w) // 2
        # Stable feet anchor for all frames, while retaining room for a long tail.
        y = canvas_size - 2 - h
        canvas.alpha_composite(frame, (x, y))
        save_sprite(f"{name}_d{row}_f{col}", canvas, opacity < 1)


def hud():
    icons = Image.open(SOURCES / "HUD Icons.png").convert("RGBA")
    for i in range(6):
        col, row = i % 3, i // 3
        cell = icons.crop((col * 512, row * 512, (col + 1) * 512, (row + 1) * 512))
        box = content_box(cell)
        if box:
            cell = cell.crop(box).resize((30, 30), Image.Resampling.LANCZOS)
            cell.putalpha(cell.getchannel("A").point(lambda a: 255 if a >= 100 else 0))
            save_sprite(f"hud_icon_{i}", cell)

    orbs = Image.open(SOURCES / "HUD Empty Orbs.png").convert("RGBA")
    for i in range(2):
        cell = orbs.crop((i * orbs.width // 2, 0, (i + 1) * orbs.width // 2, orbs.height))
        box = content_box(cell)
        if box:
            cell = cell.crop(box).resize((44, 44), Image.Resampling.LANCZOS)
            # The generated glass is opaque; clear the center so live fill is visible.
            pix = cell.load()
            for y in range(44):
                for x in range(44):
                    if (x - 22) ** 2 + (y - 22) ** 2 < 13 ** 2:
                        r, g, b, _ = pix[x, y]
                        pix[x, y] = r, g, b, 0
            save_sprite(f"hud_orb_{i}", cell)

    frames = Image.open(SOURCES / "HUD Frames.png").convert("RGBA")
    for name, box, dims in [
        ("hud_belt", (280, 335, 1510, 545), (220, 34)),
        ("hud_boss", (65, 545, 1710, 720), (240, 28)),
        ("hud_banner", (360, 715, 1425, 865), (280, 38)),
    ]:
        cell = frames.crop(box).resize(dims, Image.Resampling.LANCZOS)
        cell.putalpha(cell.getchannel("A").point(lambda a: 255 if a >= 90 else 0))
        if name == "hud_boss":
            pix = cell.load()
            for y in range(9, 20):
                for x in range(27, 213):
                    r, g, b, _ = pix[x, y]
                    pix[x, y] = r, g, b, 0
        save_sprite(name, cell)

    states = Image.open(SOURCES / "HUD States.png").convert("RGBA")
    for i in range(4):
        col, row = i % 2, i // 2
        cell = states.crop((col * 768, row * 512, (col + 1) * 768, (row + 1) * 512))
        box = content_box(cell)
        if box:
            cell = cell.crop(box).resize((34, 34), Image.Resampling.LANCZOS)
            cell.putalpha(cell.getchannel("A").point(lambda a: 255 if a >= 90 else 0))
            save_sprite(f"hud_state_{i}", cell)


def _leg_stride(im, f, side):
    """A corrida do Bárbaro veio com as 4 poses de perna quase iguais. Gera a passada deformando
    só as pernas (abaixo do quadril), com transição suave para o tronco ficar intacto.
    side=True (perfil/diagonal): as pernas abrem e fecham (tesoura).
    side=False (frente/costas): uma perna sobe enquanto a outra fica no chão, alternando."""
    a = np.asarray(im.convert("RGBA")).astype(np.float32)
    h, w = a.shape[:2]
    rows = np.flatnonzero((a[..., 3] > 60).sum(axis=1) >= 2)
    cols = np.flatnonzero((a[..., 3] > 60).sum(axis=0) >= 2)
    if len(rows) < 8:
        return im
    top, bot = rows[0], rows[-1]
    hip = int(top + (bot - top) * 0.58)
    lower = a[hip:bot + 1]
    cx = (np.flatnonzero(lower[..., 3].max(axis=0) > 60).mean()) if lower[..., 3].max() > 60 else w / 2
    out = a.copy()
    out[hip:] = 0
    ys = np.arange(hip, h)
    t = np.clip((ys - hip) / max(1, bot - hip), 0, 1)[:, None]
    xs = np.arange(w)[None, :].astype(np.float32)
    if side:
        sx = [1.0, 0.45, 0.92, 0.5][f]
        k = 1 + (sx - 1) * t                       # 1 no quadril -> sx nos pes
        src_x = cx + (xs - cx) / k
        src_y = np.repeat(ys[:, None], w, axis=1).astype(np.float32)
        lift = [0, 2.5, 0, 2.5][f]
        src_y = src_y + lift * t                   # na passagem os pes sobem um pouco
    else:
        up_l, up_r = [(0, 0), (4, 0), (0, 0), (0, 4)][f]
        lift = np.where(xs < cx, up_l, up_r)
        src_y = ys[:, None] + lift * t
        src_x = np.repeat(xs, len(ys), axis=0)
    xi = np.clip(np.round(src_x).astype(int), 0, w - 1)
    yi = np.clip(np.round(src_y).astype(int), 0, h - 1)
    valid = (src_x >= 0) & (src_x < w) & (src_y < h)
    seg = a[yi, xi]
    seg[~valid] = 0
    out[hip:] = seg
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGBA")


def barb_spin_sheet(filename="Barbaro/Barbarian Whirlwind.png"):
    """Redemoinho novo: prancheta 4x8 (8 direcoes x 4 quadros, celulas 384x256) com rastro de vento
    semitransparente. Corta por grade (o recorte por figura apagaria o vento) e usa 4 das 8 linhas:
    d0 frente = linha 0, d1 diagonal = linha 1, d2 perfil (olhando para a direita, como as outras
    pranchetas do Barbaro) = linha 6, d3 costas = linha 4."""
    src = Image.open(SOURCES / filename).convert("RGBA")
    rows_used = {0: 0, 1: 1, 2: 6, 3: 4}
    cw, ch = src.width // 4, src.height // 8
    cells = []
    for d, r in rows_used.items():
        for c in range(4):
            cell = src.crop((c * cw, r * ch, (c + 1) * cw, (r + 1) * ch))
            # apaga pedacos das figuras vizinhas (cabelo/pes que invadem a celula pelas bordas)
            al = np.asarray(cell.getchannel("A")) >= 40
            parts = components(al, with_runs=True)
            if parts:
                biggest = max(p[4] for p in parts)
                keep = Image.new("L", cell.size, 0)
                dr = ImageDraw.Draw(keep)
                for p in parts:
                    edge = p[1] <= 1 or p[3] >= cell.height - 1 or p[0] <= 1 or p[2] >= cell.width - 1
                    if edge and p[4] < biggest * 0.35:
                        continue
                    if p[4] < 40:
                        continue
                    for rx0, rx1, ry in p[5]:
                        dr.line((rx0, ry, rx1 - 1, ry), fill=255)
                keep = keep.filter(ImageFilter.MaxFilter(3))
                cell.putalpha(ImageChops.multiply(cell.getchannel("A"), keep))
            box = cell.getchannel("A").point(lambda v: 255 if v >= 40 else 0).getbbox()
            cells.append((d, c, cell.crop(box) if box else cell))
    scale = ACTOR_SCALE["barb_"]
    big = max(max(im.width, im.height) for _, _, im in cells)
    canvas_size = max(64, ceil((big * scale + 4) / 8) * 8)
    for d, c, im in cells:
        w, h = max(1, round(im.width * scale)), max(1, round(im.height * scale))
        frame = im.resize((w, h), Image.Resampling.LANCZOS)
        frame.putalpha(frame.getchannel("A").point(lambda v: 255 if v >= 70 else 0))
        canvas = Image.new("RGBA", (canvas_size, canvas_size))
        canvas.alpha_composite(frame, ((canvas_size - w) // 2, canvas_size - 2 - h))
        save_sprite(f"barb_spin_d{d}_f{c}", canvas)


def barb_postprocess():
    """Bárbaro (pranchetas do ChatGPT, cada uma numa escala):
    - a diagonal (d1) vem olhando para a direita e o jogo espera a esquerda (como a Caçadora): espelha d1;
    - iguala a ALTURA do corpo (pés até o topo do cabelo ruivo, ignorando o machado) de todas as
      animações e direções à do idle de frente. Usa o quadro mais alto de cada animação, assim
      poses agachadas (salto, morte) não são ampliadas."""
    names = list(BARBARIAN)
    # (corrida nova do ChatGPT ja tem passada; _leg_stride fica so de reserva para pranchetas sem perna)
    for name in names:
        for f in range(4):
            path = OUT / f"spr_{name}_d1_f{f}.rgba16.png"
            Image.open(path).transpose(Image.Transpose.FLIP_LEFT_RIGHT).save(path, optimize=True)

    def body_h(im):
        a = np.asarray(im.convert("RGBA")).astype(int)
        r, g, b, al = a[..., 0], a[..., 1], a[..., 2], a[..., 3]
        hair = (al > 128) & (r > 85) & (r > g * 1.45) & (r > b * 1.6)
        rows = np.flatnonzero(hair.sum(axis=1) >= 2)
        solid = np.flatnonzero((al > 128).sum(axis=1) >= 3)
        if len(rows) == 0 or len(solid) == 0:
            return None
        return float(solid[-1] - rows[0] + 1)

    def measure(name, d):
        hs = [body_h(Image.open(OUT / f"spr_{name}_d{d}_f{f}.rgba16.png")) for f in range(4)]
        hs = [h for h in hs if h]
        return max(hs) if hs else None

    target = measure("barb_idle", 0)
    if not target:
        return
    for name in names:
        if name == "barb_spin":
            continue      # o redemoinho agora e so o furacao (vermelho: a medida pelo cabelo nao vale)
        for d in range(4):
            m = measure(name, d)
            if not m:
                continue
            k = max(0.7, min(1.4, target / m))
            if abs(k - 1.0) < 0.03:
                continue
            for f in range(4):
                q = OUT / f"spr_{name}_d{d}_f{f}.rgba16.png"
                im = Image.open(q).convert("RGBA")
                w, h = max(1, round(im.width * k)), max(1, round(im.height * k))
                small = im.resize((w, h), Image.Resampling.LANCZOS)
                cw, ch = ceil(w / 8) * 8, ceil(h / 8) * 8      # tela em multiplos de 8, pes embaixo
                canvas = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
                canvas.alpha_composite(small, ((cw - w) // 2, ch - h))
                canvas.save(q, optimize=True)
    # Furacao do redemoinho: um pouco mais alto que o Barbaro (ele fica "dentro" do vento).
    spin_h = max(Image.open(OUT / f"spr_barb_spin_d{d}_f{f}.rgba16.png").getchannel("A").getbbox()[3] -
                 Image.open(OUT / f"spr_barb_spin_d{d}_f{f}.rgba16.png").getchannel("A").getbbox()[1]
                 for d in range(4) for f in range(4))
    k = max(0.5, min(2.0, target * 1.3 / spin_h))
    for d in range(4):
        for f in range(4):
            q = OUT / f"spr_barb_spin_d{d}_f{f}.rgba16.png"
            im = Image.open(q).convert("RGBA")
            w, h = max(1, round(im.width * k)), max(1, round(im.height * k))
            small = im.resize((w, h), Image.Resampling.LANCZOS)
            small.putalpha(small.getchannel("A").point(lambda v: 255 if v >= 100 else 0))
            cw, ch = ceil(w / 8) * 8, ceil(h / 8) * 8
            canvas = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
            canvas.alpha_composite(small, ((cw - w) // 2, ch - h))
            canvas.save(q, optimize=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name, args in SHEETS.items():
        sheet(name, *args)
    if (SOURCES / BARBARIAN["barb_idle"]).exists():   # so com o conjunto (o jogo detecta pelo idle)
        for name, src in BARBARIAN.items():
            if (SOURCES / src).exists():
                if name == "barb_spin" and Image.open(SOURCES / src).height > Image.open(SOURCES / src).width:
                    barb_spin_sheet(src)      # prancheta nova de 8 direcoes
                else:
                    sheet(name, src, 4, 4, 1.0, 64)
        if all((SOURCES / src).exists() for src in BARBARIAN.values()):
            barb_postprocess()
    for name, src in HUNTRESS_V2.items():
        if (SOURCES / src).exists():
            sheet(name, src, 4, 4, 1.0, 64)
    # O primeiro idle de costas tem um arco extra; o segundo e a pose limpa.
    copyfile(OUT / "spr_hunt_idle_d3_f1.rgba16.png",
             OUT / "spr_hunt_idle_d3_f0.rgba16.png")
    hud()
    print(f"Generated {len(list(OUT.glob('*.png')))} N64 sprite frames in {OUT}")


if __name__ == "__main__":
    main()
