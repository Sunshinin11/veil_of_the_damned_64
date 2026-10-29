#ifndef SPRITES_H
#define SPRITES_H
#include "common.h"

typedef enum {
    SPR_UI_ATTACK, SPR_UI_BOLT, SPR_UI_THORNS, SPR_UI_WOLF,
    SPR_UI_BEAST, SPR_UI_POTION, SPR_UI_LIFE_ORB, SPR_UI_SPIRIT_ORB,
    SPR_UI_BELT, SPR_UI_BOSS_BAR, SPR_UI_BANNER,
    SPR_UI_SELECTED, SPR_UI_DISABLED, SPR_UI_COOLDOWN, SPR_UI_LEVEL,
    SPR_UI_H_SHOT, SPR_UI_H_VOLLEY, SPR_UI_H_ROOTS, SPR_UI_H_HAWK, SPR_UI_H_FOCUS,   // cinto da cacadora
    SPR_UI_B_AXE, SPR_UI_B_LEAP, SPR_UI_B_SHOUT, SPR_UI_B_SPIN, SPR_UI_B_BERSERK,    // cinto do barbaro
    SPR_UI_COUNT
} SpriteUi;

void sprites_init(void);
void sprites_preload_area(void);   // com Expansion Pak: pre-carrega os quadros dos monstros da fase
bool sprites_druid_ready(void);
void sprites_draw_world(T3DViewport *vp);
typedef struct {
    uint32_t queue_us, draws, blits, palette_uploads, depth_bands;
    uint32_t texture_tiles, triangles, uploaded_bytes, actors_us, scenery_us, frame_loads;
} SpritePerf;
const SpritePerf *sprites_perf(void);
void sprites_draw_ui(SpriteUi id, float x, float y, float scale);
void sprites_draw_druid_portrait(float x, float y, float scale);
void sprites_draw_hero_portrait(int hero, float x, float y, float scale);
void sprites_draw_hero_portrait_ex(int hero, float x, float y, float scale, bool dark);
#endif
