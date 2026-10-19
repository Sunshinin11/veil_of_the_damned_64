"""Refine the authored druid run on a native 32x48 grid with visible pixels."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Sprites" / "Pranchetas"
source = Image.open(OUT / "Druida corrida pixel verdadeiro 24x32.png").convert("RGBA")

COL = {
    "outline": "#151a17", "deep": "#263a2d", "green": "#466044",
    "green_light": "#779063", "green_mid": "#5f7953", "leather": "#473628",
    "brown": "#896540", "brown_light": "#ae8350", "brass": "#b48c56",
    "brass_light": "#d0a96d", "skin": "#aa7859", "skin_light": "#c4936b",
    "gem": "#31a65b", "gem_light": "#dcf4af",
}


def refine(source_frame, index):
    # Nearest-neighbour only establishes the silhouette; all additional detail
    # below is placed on individual 32x48 source pixels, never smoothed.
    im = source_frame.resize((32, 48), Image.Resampling.NEAREST)
    d = ImageDraw.Draw(im)
    def p(x, y, c): d.point((x, y), fill=COL[c])
    def line(x1, y1, x2, y2, c): d.line((x1, y1, x2, y2), fill=COL[c], width=1)

    # Hood: directional planes, stitched hem and a readable face opening.
    line(16, 11, 19, 11, "green_mid")
    line(15, 12, 15, 16, "green_mid")
    line(17, 9, 19, 9, "green_light")
    line(19, 18, 22, 19, "deep")
    p(22, 15, "skin_light")
    p(22, 14, "gem_light")
    p(20, 18, "leather")

    # Distinct folded cloth clusters, rather than a gradient over the cloak.
    line(4, 31, 10, 31, "green_mid")
    line(8, 29, 14, 29, "green_light")
    line(3, 35, 9, 35, "green_mid")
    line(7, 37, 11, 37, "green_light")
    line(12, 32, 17, 32, "deep")
    line(14, 26, 17, 25, "green_mid")
    for x, y in [(5, 34), (9, 30), (11, 34), (14, 28), (7, 37)]:
        p(x, y, "green_light")
    # A tiny frame-dependent fold follows the step.
    if index % 2 == 0:
        line(5, 38, 8, 38, "green_mid")
    else:
        line(6, 39, 10, 39, "green_mid")

    # Armor and leather straps use single-pixel edge highlights.
    line(15, 20, 17, 20, "brass_light")
    p(14, 22, "brass")
    line(17, 25, 20, 27, "brown")
    line(17, 30, 22, 30, "brown_light")
    p(20, 30, "brass_light")
    line(22, 25, 24, 27, "green_mid")
    p(24, 28, "skin_light")

    # Shin straps and boot tips: check alpha before painting so no stray pixels.
    alpha = im.getchannel("A")
    for y in (38, 41):
        for x in range(5, 29):
            if alpha.getpixel((x, y)) == 255 and im.getpixel((x, y))[:3] in {
                tuple(bytes.fromhex(COL["brown"][1:])),
                tuple(bytes.fromhex(COL["leather"][1:])),
            }:
                p(x, y, "brown_light")
                break
    for x in range(5, 29):
        if alpha.getpixel((x, 45)) == 255:
            p(x, 45, "brown_light")
            break

    # Staff bark and hard-edged gem glint.
    line(27, 17, 27, 23, "brown_light")
    p(26, 27, "brown_light")
    p(27, 34, "brown_light")
    p(27, 8, "gem_light")
    p(26, 12, "gem")
    p(28, 12, "gem")
    return im


frames = [refine(source.crop((i*24, 0, (i+1)*24, 32)), i) for i in range(4)]
sheet = Image.new("RGBA", (32*4, 48), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    sheet.alpha_composite(frame, (i*32, 0))
sheet.save(OUT / "Druida corrida pixels 32x48 nativo.png")

SCALE = 12
def pixel_grid(frame):
    im = Image.new("RGB", (32*SCALE, 48*SCALE), (20, 19, 23))
    d = ImageDraw.Draw(im)
    for y in range(48):
        for x in range(32):
            rgba = frame.getpixel((x, y))
            color = rgba[:3] if rgba[3] else ((38, 36, 43) if (x+y)%2 else (45, 42, 49))
            x0, y0 = x*SCALE, y*SCALE
            d.rectangle((x0+1, y0+1, x0+SCALE-2, y0+SCALE-2), fill=color)
    return im

panels = [pixel_grid(frame) for frame in frames]
preview = Image.new("RGB", (panels[0].width*4, panels[0].height), (20, 19, 23))
for i, panel in enumerate(panels):
    preview.paste(panel, (i*panel.width, 0))
preview.save(OUT / "Druida corrida pixels 32x48 QUADRADOS.png")
panels[0].save(OUT / "Druida corrida pixels 32x48.gif", save_all=True,
               append_images=panels[1:], duration=145, loop=0, optimize=False)
print(OUT / "Druida corrida pixels 32x48 QUADRADOS.png")
