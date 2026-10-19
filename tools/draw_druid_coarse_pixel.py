"""Native 24x32 chunky pixel-art druid study, drawn with a tiny fixed palette."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Sprites" / "Pranchetas"
OUT.mkdir(parents=True, exist_ok=True)

P = {
    "k": "#151a17",   # outline
    "d": "#263a2d",   # dark green
    "g": "#466044",   # robe green
    "h": "#779063",   # lit green
    "b": "#473628",   # leather and dark boot
    "l": "#896540",   # lit boot / staff
    "m": "#b48c56",   # brass
    "s": "#aa7859",   # skin
    "c": "#31a65b",   # crystal green
    "w": "#dcf4af",   # crystal highlight
}


def rect(d, box, color):
    d.rectangle(box, fill=P[color])


def leg(d, hip, knee, ankle, near, toe):
    color = "l" if near else "b"
    # A continuous stepped thigh, shin and square boot, each drawn at source size.
    d.line([hip, knee, ankle], fill=P["k"], width=4)
    d.line([hip, knee, ankle], fill=P[color], width=2)
    rect(d, (ankle[0]-2, ankle[1], ankle[0]+toe+1, 30), "k")
    rect(d, (ankle[0]-1, ankle[1], ankle[0]+toe, 29), color)
    rect(d, (ankle[0]+toe-1, 30, ankle[0]+toe+1, 30), "m" if near else "l")


def draw_frame(n):
    im = Image.new("RGBA", (24, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Each tuple gives far and near legs. Frame 0: near leg forward;
    # frame 2: near leg back. Color makes the swapped depth unmistakable.
    poses = [
        (((12, 21), (9, 25), (6, 28), 0), ((15, 21), (17, 25), (19, 28), 2)),
        (((12, 21), (11, 25), (10, 28), 1), ((15, 21), (16, 25), (16, 28), 1)),
        (((12, 21), (17, 25), (19, 28), 2), ((15, 21), (10, 25), (6, 28), 0)),
        (((12, 21), (14, 25), (15, 28), 1), ((15, 21), (13, 25), (12, 28), 1)),
    ]
    far, near = poses[n]
    leg(d, far[0], far[1], far[2], False, far[3])

    # Blocky wind-swept cape: no gradients, no airbrush, every edge is a pixel.
    rect(d, (7, 12, 14, 22), "k")
    rect(d, (4, 17, 12, 23), "k")
    rect(d, (2, 20, 10, 24), "k")
    rect(d, (1, 23, 7, 25), "k")
    rect(d, (6, 17, 12, 22), "d")
    rect(d, (3, 20, 10, 23), "d")
    rect(d, (2, 23, 8, 24), "g")
    rect(d, (6, 18, 11, 19), "g")
    rect(d, (4, 21, 9, 21), "h")
    rect(d, (5, 24, 7, 24), "h")

    # Torso, leather diagonal and brass pauldron.
    rect(d, (10, 12, 17, 21), "k")
    rect(d, (11, 13, 16, 20), "g")
    rect(d, (12, 15, 17, 16), "b")
    rect(d, (14, 17, 17, 18), "b")
    rect(d, (11, 19, 17, 20), "b")
    rect(d, (14, 20, 14, 20), "m")
    rect(d, (9, 13, 12, 16), "k")
    rect(d, (10, 13, 12, 15), "m")
    rect(d, (10, 13, 11, 13), "w")

    # Hood and the two-pixel dark face.
    rect(d, (11, 5, 16, 12), "k")
    rect(d, (10, 8, 17, 11), "k")
    rect(d, (11, 7, 15, 10), "d")
    rect(d, (12, 6, 14, 6), "g")
    rect(d, (11, 8, 12, 9), "g")
    rect(d, (15, 9, 17, 10), "k")
    rect(d, (16, 10, 17, 10), "s")
    rect(d, (16, 9, 16, 9), "w")
    rect(d, (12, 11, 16, 13), "d")

    # Near leg overlays the hem, creating a real depth change at frame 2.
    leg(d, near[0], near[1], near[2], True, near[3])
    rect(d, (14, 21, 16, 22), "g")

    # Staff, hand and sharp crystal in the same position in all four frames.
    rect(d, (19, 9, 21, 26), "k")
    rect(d, (20, 10, 20, 25), "l")
    rect(d, (18, 5, 22, 9), "k")
    rect(d, (19, 3, 21, 10), "k")
    rect(d, (19, 5, 21, 8), "c")
    rect(d, (20, 4, 20, 7), "w")
    rect(d, (17, 15, 20, 17), "k")
    rect(d, (18, 15, 19, 16), "s")
    return im


frames = [draw_frame(i) for i in range(4)]
sheet = Image.new("RGBA", (96, 32), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    sheet.alpha_composite(frame, (i * 24, 0))
sheet.save(OUT / "Druida corrida pixel verdadeiro 24x32.png")

scale = 12
large = sheet.resize((96*scale, 32*scale), Image.Resampling.NEAREST)
bg = Image.new("RGBA", large.size, (37, 35, 42, 255))
bg.alpha_composite(large)
bg.convert("RGB").save(OUT / "Druida corrida pixel verdadeiro ampliado.png")

gif_frames = []
for frame in frames:
    large_frame = frame.resize((24*scale, 32*scale), Image.Resampling.NEAREST)
    backdrop = Image.new("RGBA", large_frame.size, (37, 35, 42, 255))
    backdrop.alpha_composite(large_frame)
    gif_frames.append(backdrop.convert("RGB"))
gif_frames[0].save(OUT / "Druida corrida pixel verdadeiro.gif", save_all=True,
                   append_images=gif_frames[1:], duration=145, loop=0, optimize=False)
print(OUT / "Druida corrida pixel verdadeiro ampliado.png")
