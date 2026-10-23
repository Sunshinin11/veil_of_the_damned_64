BUILD_DIR = build
T3D_INST ?= deps/tiny3d

N64_ROM_SAVETYPE = eeprom16k   # save no cartucho (Ares le do cabecalho)
include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

N64_CFLAGS += -std=gnu2x -O2 -Wall -Wextra -Iinclude -Ideps/tiny3d/src

src = src/main.c \
      src/gfx.c \
      src/models_gen.c \
      src/sprites.c \
      src/models.c \
      src/audio.c \
      src/druid.c \
      src/spells.c \
      src/enemies.c \
      src/items.c \
      src/world.c \
      src/cemetery.c \
      src/hud.c \
      src/menu.c \
      src/cards.c \
      src/lang.c \
      src/save.c \
      src/mapgen.c \
      src/huntress.c \
      src/barbarian.c

assets_png  = $(wildcard assets/*.png)
sprite_png  = $(wildcard assets/sprites/*.png)
scene_png   = $(wildcard assets/scenery/*.png)
audio_mp3   = $(wildcard assets/audio/*.mp3)
audio_wav   = $(wildcard assets/audio/*.wav)
assets_glb  = $(wildcard assets/models/*.glb)
font_ttf    = $(wildcard assets/fonts/*.ttf)
cemetery_raw = assets/cemetery/cem_opt_pages.ci8 assets/cemetery/cem_opt_index.bin assets/cemetery/cem_painted_palettes.tlut
N64_MKDFS_ROOT = $(BUILD_DIR)/romfs
assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(sprite_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(scene_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(audio_mp3:%.mp3=%.wav64))) \
              $(addprefix filesystem/,$(notdir $(audio_wav:%.wav=%.wav64))) \
              $(addprefix filesystem/,$(notdir $(assets_glb:%.glb=%.t3dm))) \
              $(addprefix filesystem/,$(notdir $(font_ttf:%.ttf=%.font64))) \
              $(addprefix filesystem/,$(notdir $(cemetery_raw)))

all: veil_of_the_damned.z64

# Already-indexed CI8 with its matching TLUT. Do not re-quantize with mksprite.
$(addprefix filesystem/,$(notdir $(cemetery_raw))): filesystem/%: assets/cemetery/%
	@mkdir -p $(dir $@)
	cp "$<" "$@"

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

sprite_conv = $(addprefix filesystem/,$(notdir $(sprite_png:%.png=%.sprite)))
$(sprite_conv): filesystem/%.sprite: assets/sprites/%.png Makefile
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -f CI8 -c 2 -o filesystem "$<"

scene_conv = $(addprefix filesystem/,$(notdir $(scene_png:%.png=%.sprite)))
$(scene_conv): filesystem/%.sprite: assets/scenery/%.png Makefile
	@mkdir -p $(dir $@)
	@echo "    [SCENERY] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -f CI8 -c 2 -o filesystem "$<"

audio_conv = $(addprefix filesystem/,$(notdir $(audio_mp3:%.mp3=%.wav64)))
$(audio_conv): filesystem/%.wav64: assets/audio/%.mp3 Makefile
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	$(N64_AUDIOCONV) --wav-mono --wav-resample 12000 --wav-compress 1 -o filesystem "$<"

audio_wav_conv = $(addprefix filesystem/,$(notdir $(audio_wav:%.wav=%.wav64)))
$(audio_wav_conv): filesystem/%.wav64: assets/audio/%.wav Makefile
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	$(N64_AUDIOCONV) --wav-mono --wav-resample 12000 --wav-compress 1 -o filesystem "$<"

# Fontes goticas (Pirata One, licenca OFL): uma grande (titulos/menu) e uma pequena (HUD/textos).
filesystem/gothic_big.font64: assets/fonts/gothic_big.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) --size 18 --range 20-7E --range A0-FF -o filesystem "$<"

filesystem/gothic_small.font64: assets/fonts/gothic_small.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) --size 13 --range 20-7E --range A0-FF -o filesystem "$<"

filesystem/%.t3dm: assets/models/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=64 --ignore-materials
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

# Stage only the declared/runtime manifest; leave original conversion outputs intact.
$(BUILD_DIR)/rom_assets.stamp: $(assets_conv) $(wildcard filesystem/*) $(src) tools/stage_rom_assets.py Makefile
	@mkdir -p $(BUILD_DIR)
	python3 tools/stage_rom_assets.py --output "$(N64_MKDFS_ROOT)" --report "$(BUILD_DIR)/rom_manifest.json"
	@touch "$@"

$(BUILD_DIR)/veil_of_the_damned.dfs: $(BUILD_DIR)/rom_assets.stamp
$(BUILD_DIR)/veil_of_the_damned.elf: $(src:%.c=$(BUILD_DIR)/%.o)

veil_of_the_damned.z64: N64_ROM_TITLE = "VEIL OF THE DAMNED"
veil_of_the_damned.z64: $(BUILD_DIR)/veil_of_the_damned.dfs

clean:
	rm -rf $(BUILD_DIR) *.z64 filesystem

-include $(wildcard $(BUILD_DIR)/*.d) $(src:%.c=$(BUILD_DIR)/%.d)

.PHONY: all clean
