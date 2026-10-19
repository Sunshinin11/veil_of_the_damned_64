"""Show every native 16x24 art pixel as a clearly separated square."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Sprites" / "Pranchetas"
source = Image.open(OUT / "Druida corrida pixel verdadeiro 24x32.png").convert("RGBA")

# Reduce each authored frame to a deliberately coarse native 16x24 grid.
frames = [source.crop((i*24, 0, (i+1)*24, 32)).resize((16, 24), Image.Resampling.NEAREST)
          for i in range(4)]
native = Image.new("RGBA", (64, 24), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    native.alpha_composite(frame, (i*16, 0))
native.save(OUT / "Druida corrida quadrados nativo 16x24.png")

BLOCK = 20
def visible_squares(frame):
    im = Image.new("RGB", (16*BLOCK, 24*BLOCK), (21, 20, 25))
    d = ImageDraw.Draw(im)
    for y in range(24):
        for x in range(16):
            rgba = frame.getpixel((x, y))
            if rgba[3] == 0:
                color = (39, 37, 44) if (x+y)%2 == 0 else (48, 45, 52)
            else:
                color = rgba[:3]
            x0, y0 = x*BLOCK, y*BLOCK
            d.rectangle((x0+1, y0+1, x0+BLOCK-2, y0+BLOCK-2), fill=color)
    return im

panels = [visible_squares(frame) for frame in frames]
preview = Image.new("RGB", (len(panels)*panels[0].width, panels[0].height), (21, 20, 25))
for i, panel in enumerate(panels):
    preview.paste(panel, (i*panel.width, 0))
preview.save(OUT / "Druida corrida QUADRADOS visiveis.png")
panels[0].save(OUT / "Druida corrida QUADRADOS.gif", save_all=True,
               append_images=panels[1:], duration=150, loop=0, optimize=False)
print(OUT / "Druida corrida QUADRADOS visiveis.png")
