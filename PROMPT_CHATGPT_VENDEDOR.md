# Prompt ChatGPT — Mascate Demoníaco + Essência

Gere as duas imagens abaixo, uma de cada vez. Salve em `Sprites/Vendedor/` com os nomes indicados
e rode `python tools/import_vendor.py`. Até lá o jogo usa um placeholder.

---

## V1 — `v1_mascate.png` (mascate parado, 3 quadros)

```
Create a sprite sheet for a dark fantasy isometric action RPG (Diablo-like, gothic, grim, painterly pixel-art look, same style as a 1990s PC ARPG).

Subject: a "Demon Peddler" merchant NPC. A hunched, hooded wandering trader with a tattered dark crimson-and-black cloak, long bony grey fingers, two small curved horns poking through the hood, and glowing ember-orange eyes in the shadow of the hood. He stands behind a small crooked wooden cart/stall loaded with trinkets: red potion bottles, blue elixir vials, a few sealed cards with wax seals, skulls, chains and a small glowing red demonic crystal. A hanging iron lantern with a warm orange flame is attached to a pole on the cart. A heavy sack on his back.

View: 3/4 top-down isometric camera (about 45 degrees from above), character facing the viewer slightly to the left, full body plus the whole cart visible, feet/wheels at the bottom.

Layout: exactly 3 frames side by side in a single row (3 columns x 1 row), each frame the same size and the figure in the same position. It is an idle loop: frame 1 neutral, frame 2 he slightly leans forward and rubs his hands, frame 3 back to neutral while the lantern flame flickers brighter. Only small differences between frames (subtle breathing, lantern flicker, eye glow).

Style: dark, desaturated palette with strong red and orange accents, strong silhouette readable at small size (it will be shrunk to 64x80 pixels), soft shading, no outline glow, no ground shadow, no floor.

Background: fully TRANSPARENT (PNG with alpha). No text, no labels, no frame borders, no grid lines, no watermark.
```

---

## V2 — `v2_essencia.png` (drop de essência, 4 quadros)

```
Create a sprite sheet for the same dark fantasy isometric action RPG.

Subject: "Demon Essence", the game's currency dropped by enemies. A small floating jagged crystal shard of dark blood-red stone with a burning orange-red core, wrapped in thin wisps of crimson smoke/flame, with a few tiny embers around it.

Layout: exactly 4 frames side by side in a single row (4 columns x 1 row), same size and centered position in each frame. Animation loop: the crystal pulses and rotates slightly, the inner glow gets brighter in frames 2-3 and dimmer in 1 and 4, smoke wisps move upward.

Style: dark gothic, saturated red/orange glow against a dark crystal, very strong readable silhouette (it will be shrunk to 32x32 pixels), no ground shadow.

Background: fully TRANSPARENT (PNG with alpha). No text, no labels, no borders, no grid lines, no watermark.
```

---

## V3 — `v3_selos.png` (ícone dos selos/pilares no HUD, 2 ícones)

```
Create a small UI icon pair for the same dark fantasy gothic action RPG HUD.

Subject: an icon representing a demonic seal pillar (a dark obsidian obelisk/rune stone).
Icon 1 (left) - INTACT seal: a dark black-red obsidian rune stone with a dim, dormant dark-red demonic rune carved on it, faint ominous glow.
Icon 2 (right) - DESTROYED seal: the same rune stone cracked and broken in half, the rune burning bright orange-yellow, small sparks and embers bursting out of the crack.

Layout: exactly 2 icons side by side in a single row (2 columns x 1 row), same size, centered, same framing in both.

Style: bold, simple, very high contrast, thick readable silhouette - it will be shrunk to 16x16 pixels, so avoid fine details. Front view, no perspective, no ground shadow.

Background: fully TRANSPARENT (PNG with alpha). No text, no labels, no borders, no grid lines, no watermark.
```

---

Dica: se o ChatGPT colocar fundo xadrez "falso" ou branco, peça: *"regenerate with a real transparent background (alpha channel), not a checkerboard pattern"*.
