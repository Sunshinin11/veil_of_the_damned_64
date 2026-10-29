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
      src/review_hero_actions.c \
      src/spells.c \
      src/enemies.c \
      src/campaign_combat.c \
      src/items.c \
      src/world.c \
      src/cemetery_stage_v2.c \
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
cemetery_materials = $(addprefix assets/cemetery/v3/cem_v3_,soil.png paving.png moss.png cliff.png rock.png bark.png masonry.png slate.png door.png iron.png)
cemetery_v4_materials = $(addprefix assets/cemetery/v5/cem_v5_,headstone.png cross.png)
cemetery_conv = $(addprefix filesystem/,$(notdir $(cemetery_materials:%.png=%.sprite))) $(addprefix filesystem/,$(notdir $(cemetery_v4_materials:%.png=%.sprite))) filesystem/cem_v4_kit.t3dm
cemetery_conv += filesystem/cem_v6_soil.sprite
crypt_materials = $(addprefix assets/crypt/v1/crypt_v1_,floor.png wall.png)
crypt_conv = $(addprefix filesystem/,$(notdir $(crypt_materials:%.png=%.sprite))) filesystem/crypt_v1_kit.t3dm
altar_materials = $(addprefix assets/altar/v1/altar_v1_,floor.png lava.png rail.png)
altar_conv = $(addprefix filesystem/,$(notdir $(altar_materials:%.png=%.sprite))) filesystem/altar_v1_kit.t3dm
forest_conv = filesystem/forest_v6_ground.sprite filesystem/forest_v5_grass.sprite filesystem/forest_v7_kit.t3dm filesystem/forest_v2_dirt.sprite
swamp_conv = filesystem/swamp_v1_mud.sprite filesystem/swamp_v1_water.sprite filesystem/swamp_v1_kit.t3dm
heart_conv = filesystem/heart_v1_rock.sprite filesystem/heart_v1_kit.t3dm filesystem/heart_v3_planks.sprite
snow_conv = filesystem/snow_v1_snow.sprite filesystem/snow_v1_trail.sprite filesystem/snow_v1_kit.t3dm
ice_conv = filesystem/ice_v1_floor.sprite filesystem/ice_v1_wall.sprite filesystem/ice_v1_slick.sprite filesystem/ice_v1_kit.t3dm
jarl_conv = filesystem/jarl_v1_floor.sprite filesystem/jarl_v1_procession.sprite filesystem/jarl_v1_wall.sprite filesystem/jarl_v1_cap.sprite filesystem/jarl_v1_kit.t3dm
expansion_conv = $(addprefix filesystem/ex_,sand.sprite sandstone.sprite limestone.sprite basalt.sprite cobble.sprite brick.sprite slate.sprite mosaic.sprite window.sprite door.sprite tombdoor.sprite glass.sprite v1_kit.t3dm)
polish_conv = $(addprefix filesystem/polish_,cemetery.sprite swamp.sprite trail.sprite bedrock.sprite)
N64_MKDFS_ROOT = $(BUILD_DIR)/romfs
assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(sprite_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(scene_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(audio_mp3:%.mp3=%.wav64))) \
              $(addprefix filesystem/,$(notdir $(audio_wav:%.wav=%.wav64))) \
              $(addprefix filesystem/,$(notdir $(assets_glb:%.glb=%.t3dm))) \
              $(addprefix filesystem/,$(notdir $(font_ttf:%.ttf=%.font64))) \
              $(cemetery_conv) $(crypt_conv) $(altar_conv) $(forest_conv) $(swamp_conv) $(heart_conv) $(snow_conv) $(ice_conv) $(jarl_conv) $(expansion_conv) $(polish_conv) filesystem/druid.t3dm

all: veil_of_the_damned.z64

# Prepare this castle's code and converted art without touching the ROM/DFS.
# libdragon normally inherits these compiler settings only from a .z64 target.
prepare-jarl: CC=$(N64_CC)
prepare-jarl: CXX=$(N64_CXX)
prepare-jarl: AS=$(N64_AS)
prepare-jarl: LD=$(N64_LD)
prepare-jarl: CFLAGS+=$(N64_CFLAGS)
prepare-jarl: CXXFLAGS+=$(N64_CXXFLAGS)
prepare-jarl: LDFLAGS+=$(N64_LDFLAGS)
prepare-jarl: $(BUILD_DIR)/veil_of_the_damned.elf $(jarl_conv) filesystem/scene_frost_cage.rgba16.sprite filesystem/scene_frost_throne.rgba16.sprite
.PHONY: prepare-jarl

# Review-approved art conversion and ELF validation only; no ROM/DFS target.
approved_v2_png = $(wildcard assets/sprites/spr_ha_*.png) $(wildcard assets/sprites/spr_apv_1_enforcado_brejo_andar_*.png) $(wildcard assets/sprites/spr_apv_1_oraculo_fungico_andar_*.png) $(wildcard assets/sprites/spr_apv_2_guerreiro_sepulcro_andar_*.png)
approved_v2_conv = $(addprefix filesystem/,$(notdir $(approved_v2_png:%.png=%.sprite)))
prepare-approved-art: prepare-jarl $(approved_v2_conv)
.PHONY: prepare-approved-art

# Shared 64x64 CI4 materials; one sixteen-colour palette per material.
$(addprefix filesystem/,$(notdir $(cemetery_materials:%.png=%.sprite))): filesystem/%.sprite: assets/cemetery/v3/%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

$(addprefix filesystem/,$(notdir $(cemetery_v4_materials:%.png=%.sprite))): filesystem/%.sprite: assets/cemetery/v5/%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

$(addprefix filesystem/,$(notdir $(crypt_materials:%.png=%.sprite))): filesystem/%.sprite: assets/crypt/v1/%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

$(addprefix filesystem/,$(notdir $(altar_materials:%.png=%.sprite))): filesystem/%.sprite: assets/altar/v1/%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/forest_v7_kit.t3dm: assets/forest/v7/forest_v7_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/forest_v6_ground.sprite: assets/forest/v6/forest_v6_ground.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/swamp_v1_%.sprite: assets/swamp/v1/swamp_v1_%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/swamp_v1_kit.t3dm: assets/swamp/v1/swamp_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/heart_v1_rock.sprite: assets/heart/v1/heart_v1_rock.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/heart_v3_planks.sprite: assets/heart/v3/heart_v3_planks.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/ex_%.sprite: assets/expansion/v1/ex_%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/polish_%.sprite: assets/terrain_polish/v1/polish_%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/cem_v6_soil.sprite: assets/cemetery/v6/cem_v6_soil.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/ex_v1_kit.t3dm: assets/expansion/v1/ex_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/jarl_v1_%.sprite: assets/jarl/v1/jarl_v1_%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/jarl_v1_kit.t3dm: assets/jarl/v1/jarl_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/ice_v1_%.sprite: assets/ice/v1/ice_v1_%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/ice_v1_kit.t3dm: assets/ice/v1/ice_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/snow_v1_%.sprite: assets/snow/v1/snow_v1_%.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/snow_v1_kit.t3dm: assets/snow/v1/snow_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/heart_v1_kit.t3dm: assets/heart/v1/heart_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/forest_v5_grass.sprite: assets/forest/v5/forest_v5_grass.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/forest_v2_dirt.sprite: assets/forest/v2/forest_v2_dirt.png
	@mkdir -p $(dir $@)
	$(N64_MKSPRITE) -f CI4 -c 2 -o filesystem "$<"

filesystem/altar_v1_kit.t3dm: assets/altar/v1/altar_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/crypt_v1_kit.t3dm: assets/crypt/v1/crypt_v1_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/cem_v4_kit.t3dm: assets/cemetery/v4/cem_v4_kit.glb
	@mkdir -p $(dir $@)
	$(T3D_GLTF_TO_3D) "$<" $@ --base-scale=1
	$(N64_BINDIR)/mkasset -c 2 -w 256 -o filesystem $@

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

sprite_conv = $(addprefix filesystem/,$(notdir $(sprite_png:%.png=%.sprite)))
$(sprite_conv): filesystem/%.sprite: assets/sprites/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -f CI8 -c 2 -o filesystem "$<"

scene_conv = $(addprefix filesystem/,$(notdir $(scene_png:%.png=%.sprite)))
$(scene_conv): filesystem/%.sprite: assets/scenery/%.png
	@mkdir -p $(dir $@)
	@echo "    [SCENERY] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -f CI8 -c 2 -o filesystem "$<"

audio_conv = $(addprefix filesystem/,$(notdir $(audio_mp3:%.mp3=%.wav64)))
# Keep the established low-cost VADPCM decoder, with its smallest sample width.
filesystem/bgm_forest.wav64 filesystem/bgm_swamp.wav64 filesystem/bgm_boss2.wav64 filesystem/bgm_boss3.wav64 filesystem/bgm_snow1.wav64 filesystem/bgm_snow2.wav64: AUDIO_COMPRESSION = vadpcm,bits=2,huffman=true
AUDIO_COMPRESSION ?= 1
$(audio_conv): filesystem/%.wav64: assets/audio/%.mp3
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	$(N64_AUDIOCONV) --wav-mono --wav-resample 12000 --wav-compress $(AUDIO_COMPRESSION) -o filesystem "$<"

audio_wav_conv = $(addprefix filesystem/,$(notdir $(audio_wav:%.wav=%.wav64)))
$(audio_wav_conv): filesystem/%.wav64: assets/audio/%.wav
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

filesystem/druid.t3dm: assets/models/druid.t3dm
	@mkdir -p $(dir $@)
	cp assets/models/druid* filesystem/

# Stage only the declared/runtime manifest; leave original conversion outputs intact.
$(BUILD_DIR)/rom_assets.stamp: $(assets_conv) $(wildcard filesystem/*) $(src) tools/stage_rom_assets.py tools/rom_asset_rules.py Makefile
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

# Compile approved campaign sources and convert only their CI8 assets, without a ROM.
approved_campaign_png = $(wildcard assets/sprites/spr_apv_*.png assets/sprites/spr_desert_warden_*.png)
approved_campaign_conv = $(addprefix filesystem/,$(notdir $(approved_campaign_png:%.png=%.sprite)))
prepare-approved: CC=$(N64_CC)
prepare-approved: CXX=$(N64_CXX)
prepare-approved: AS=$(N64_AS)
prepare-approved: LD=$(N64_LD)
prepare-approved: CFLAGS+=$(N64_CFLAGS)
prepare-approved: CXXFLAGS+=$(N64_CXXFLAGS)
prepare-approved: LDFLAGS+=$(N64_LDFLAGS)
prepare-approved: $(BUILD_DIR)/veil_of_the_damned.elf $(approved_campaign_conv)
.PHONY: prepare-approved
