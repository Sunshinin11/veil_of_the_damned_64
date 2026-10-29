#include "hud.h"
#include "lang.h"
#include "gfx.h"
#include "druid.h"
#include "enemies.h"
#include "world.h"
#include "sprites.h"
#include "huntress.h"
#include "barbarian.h"

static char s_banner_title[48];
static char s_banner_sub[48];
static float s_banner_timer = 0.0f;
static float s_banner_duration = 0.0f;

static char s_msg_text[64];
static float s_msg_timer = 0.0f;

void hud_init(void) {
    s_banner_timer = 0.0f;
    s_msg_timer = 0.0f;
    s_banner_title[0] = '\0';
    s_banner_sub[0] = '\0';
    s_msg_text[0] = '\0';
}

void hud_show_banner(const char *title, const char *sub, float duration) {
    if (title) strncpy(s_banner_title, title, sizeof(s_banner_title) - 1);
    else s_banner_title[0] = '\0';

    if (sub) strncpy(s_banner_sub, sub, sizeof(s_banner_sub) - 1);
    else s_banner_sub[0] = '\0';

    s_banner_duration = duration;
    s_banner_timer = duration;
}

void hud_show_message(const char *msg, float duration) {
    if (msg) strncpy(s_msg_text, msg, sizeof(s_msg_text) - 1);
    s_msg_timer = duration;
}

void hud_update(float dt) {
    if (s_banner_timer > 0.0f) s_banner_timer -= dt;
    if (s_msg_timer > 0.0f) s_msg_timer -= dt;
}

// Draw a filled bevel rectangle with borders
static void draw_bevel_box(int x0, int y0, int x1, int y1, color_t fill_color, color_t border_color) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);

    rdpq_set_prim_color(border_color);
    rdpq_fill_rectangle(x0, y0, x1, y1);

    rdpq_set_prim_color(fill_color);
    rdpq_fill_rectangle(x0 + 1, y0 + 1, x1 - 1, y1 - 1);
}

static void draw_orb_fill(int cx, int cy, float ratio, color_t color) {
    ratio = clampf(ratio, 0.0f, 1.0f);
    int top = cy + 13 - (int)(ratio * 26.0f);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(color);
    for (int y = top; y <= cy + 12; y++) {
        int dy = y - cy;
        int radius = (int)sqrtf(13.0f * 13.0f - (float)(dy * dy));
        rdpq_fill_rectangle(cx - radius, y, cx + radius, y + 1);
    }
}

// ============================================================================
// HUD em layout de controle (Proposta B): A e B ao lado do orbe de vida, losango
// C ao lado do orbe de espirito, pocao Z acima do orbe de vida, pip R no aro do
// orbe de espirito. Sprites nativos gerados por tools/build_hud_cluster.py
// (sem escala em tempo de execucao). Area segura: 16 x 12 px.
// ============================================================================
typedef struct { int16_t x, y, size; } HudSlot;
static const HudSlot s_slot[6] = {   // lados espelhados: mesma distancia ate cada orbe
    {  62, 198, 28 },   // A
    {  50, 170, 22 },   // B
    { 200, 192, 22 },   // C-esquerda
    { 222, 170, 22 },   // C-cima
    { 244, 192, 22 },   // C-direita
    {  16, 160, 20 },   // Z (pocao)
};
#define HUD_ORB_L_X   10
#define HUD_ORB_R_X   266
#define HUD_ORB_Y     186
#define HUD_ORB_L_CX  32
#define HUD_ORB_R_CX  288
#define HUD_ORB_CY    208
#define HUD_R_X       296
#define HUD_R_Y       172
#define HUD_XP_X0     112
#define HUD_XP_X1     209
#define HUD_XP_Y      225
#define HUD_BOSS_X    40
#define HUD_BOSS_Y    12

enum { HB_A, HB_B, HB_CL, HB_CU, HB_CR, HB_Z, HB_R, HB_COUNT };
static const char *s_btn_names[HB_COUNT] = { "a", "b", "cl", "cu", "cr", "z", "r" };
static const char  s_set_keys[3] = { 'd', 'h', 'b' };   // druida/mago, cacadora, barbaro

static sprite_t *s_icon[3][5];
static sprite_t *s_potion;
static sprite_t *s_btn[HB_COUNT];
static sprite_t *s_ess[4];
static bool s_cluster_loaded = false;

static void hud_cluster_load(void) {
    if (s_cluster_loaded) return;
    s_cluster_loaded = true;
    char path[48];
    for (int k = 0; k < 3; k++) {
        snprintf(path, sizeof(path), "rom:/spr_hud_ca_%c.rgba16.sprite", s_set_keys[k]);
        s_icon[k][0] = sprite_load(path);
        for (int i = 1; i < 5; i++) {
            snprintf(path, sizeof(path), "rom:/spr_hud_cs_%c%d.rgba16.sprite", s_set_keys[k], i);
            s_icon[k][i] = sprite_load(path);
        }
    }
    s_potion = sprite_load("rom:/spr_hud_cz.rgba16.sprite");
    for (int i = 0; i < HB_COUNT; i++) {
        snprintf(path, sizeof(path), "rom:/spr_hud_btn_%s.rgba16.sprite", s_btn_names[i]);
        s_btn[i] = sprite_load(path);
    }
    for (int i = 0; i < 4; i++) {
        snprintf(path, sizeof(path), "rom:/spr_hud_ess_f%d.rgba16.sprite", i);
        s_ess[i] = sprite_load(path);
    }
}

static int hud_icon_set(void) {
    if (g_hero == HERO_HUNTRESS) return 1;
    if (g_hero == HERO_BARBARIAN) return 2;
    return 0;   // druida e mago (kit compartilhado)
}

static void hud_sprite_mode(void) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_alphacompare(1);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
}

static void hud_blit(sprite_t *s, int x, int y) {
    if (s) rdpq_sprite_blit(s, x, y, NULL);
}

static void hud_shade(int x0, int y0, int x1, int y1, uint8_t alpha) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, alpha));
    rdpq_fill_rectangle(x0, y0, x1, y1);
    rdpq_mode_blender(0);
}

// Sombra recortada no circulo interno do slot (miolo da arte, aro de 3 px fora).
// ratio: fracao do miolo coberta de cima para baixo (1 = inteiro).
static void hud_shade_round(const HudSlot *sl, float ratio, uint8_t alpha) {
    float cx = sl->x + sl->size * 0.5f, cy = sl->y + sl->size * 0.5f;
    float ri = sl->size * 0.5f - 3.0f;
    int y0 = sl->y + 3, y1 = sl->y + sl->size - 3;
    int ylim = y0 + (int)((float)(y1 - y0) * clampf(ratio, 0.0f, 1.0f));
    if (ylim <= y0) return;
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, alpha));
    for (int y = y0; y < ylim; y++) {
        float dy = (float)y + 0.5f - cy;
        float q = ri * ri - dy * dy;
        if (q <= 0.0f) continue;
        float hw = sqrtf(q);
        int xa = (int)(cx - hw + 0.5f), xb = (int)(cx + hw + 0.5f);
        if (xb > xa) rdpq_fill_rectangle(xa, y, xb, y + 1);
    }
    rdpq_mode_blender(0);
}

// Anel de 1 px logo fora da moldura redonda (habilidade ativa)
static void hud_ring(const HudSlot *sl, color_t c) {
    float cx = sl->x + sl->size * 0.5f, cy = sl->y + sl->size * 0.5f;
    float ro = sl->size * 0.5f + 1.0f, ri = sl->size * 0.5f;
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(c);
    for (int y = (int)(cy - ro); y <= (int)(cy + ro); y++) {
        float dy = (float)y + 0.5f - cy;
        float qo = ro * ro - dy * dy;
        if (qo <= 0.0f) continue;
        float wo = sqrtf(qo);
        float qi = ri * ri - dy * dy;
        float wi = qi > 0.0f ? sqrtf(qi) : 0.0f;
        int xo0 = (int)(cx - wo + 0.5f), xi0 = (int)(cx - wi + 0.5f);
        int xi1 = (int)(cx + wi + 0.5f), xo1 = (int)(cx + wo + 0.5f);
        if (xi0 <= xo0) xi0 = xo0 + 1;
        if (xo1 <= xi1) xi1 = xo1 - 1;
        rdpq_fill_rectangle(xo0, y, xi0, y + 1);
        rdpq_fill_rectangle(xi1, y, xo1, y + 1);
    }
}

// Estado de cada slot (mesmas regras do cinto anterior). i: 0 A, 1 B, 2 C<, 3 C^, 4 C>, 5 Z
typedef struct { bool enabled, selected; float cd, cd_total; } HudSlotState;
static HudSlotState hud_slot_state(int i) {
    HudSlotState s = { false, false, 0.0f, 0.0f };
    if (g_hero == HERO_BARBARIAN) {
        s.enabled = i == 0 || (i == 1 && g_druid.spirit >= barb_leap_cost()) ||
            (i == 2 && g_druid.spirit >= BARB_SHOUT_COST) || (i == 3 && g_druid.spirit >= BARB_SPIN_COST) ||
            (i == 4 && (g_druid.berserk_timer > 0 || g_druid.spirit >= BARB_BERSERK_COST)) ||
            (i == 5 && (g_druid.health_potions || g_druid.spirit_potions));
        s.selected = (i == 0 && g_druid.state == DRUID_STATE_ATTACK) || (i == 3 && g_druid.spin_timer > 0) ||
                     (i == 4 && g_druid.berserk_timer > 0);
        if (i >= 1 && i <= 4) {
            s.cd = barb_cooldown((BarbCooldown)(i - 1));
            s.cd_total = barb_cooldown_time((BarbCooldown)(i - 1));
            if (s.cd > 0.0f) s.enabled = false;
        }
    } else if (g_hero == HERO_HUNTRESS) {
        s.enabled = i == 0 || (i == 1 && g_druid.spirit >= hunt_volley_cost()) ||
            (i == 2 && g_druid.spirit >= HUNT_ROOTS_COST) || (i == 3 && g_druid.spirit >= HUNT_HAWK_COST) ||
            (i == 4 && (g_druid.focus_timer > 0 || g_druid.spirit >= HUNT_FOCUS_COST)) ||
            (i == 5 && (g_druid.health_potions || g_druid.spirit_potions));
        s.selected = (i == 0 && g_druid.state == DRUID_STATE_ATTACK) || (i == 4 && g_druid.focus_timer > 0);
    } else {
        s.enabled = i == 0 || (i == 1 && g_druid.spirit >= 15) ||
            (i == 2 && g_druid.spirit >= 25) ||
            (i == 3 && g_druid.spirit >= 40) ||
            (i == 4 && (g_druid.is_beast || g_druid.spirit >= 25)) ||
            (i == 5 && (g_druid.health_potions || g_druid.spirit_potions));
        s.selected = (i == 0 && g_druid.state == DRUID_STATE_ATTACK) || (i == 4 && g_druid.is_beast);
    }
    return s;
}

static bool hud_boss_active(void) {
    Enemy *boss = enemies_get_boss();
    return boss && boss->hp > 0;
}

static void hud_draw_sprites(void) {
    hud_cluster_load();

    // Orbes: o liquido fica atras do centro transparente da moldura pintada.
    draw_orb_fill(HUD_ORB_L_CX, HUD_ORB_CY, (float)g_druid.hp / (float)g_druid.max_hp,
                  RGBA32(205, 20, 36, 255));
    draw_orb_fill(HUD_ORB_R_CX, HUD_ORB_CY, g_druid.spirit / g_druid.max_spirit,
                  g_hero == HERO_BARBARIAN ? RGBA32(230, 110, 20, 255) : RGBA32(28, 145, 230, 255));   // barbaro: furia laranja
    sprites_draw_ui(SPR_UI_LIFE_ORB, HUD_ORB_L_X, HUD_ORB_Y, 1);
    sprites_draw_ui(SPR_UI_SPIRIT_ORB, HUD_ORB_R_X, HUD_ORB_Y, 1);

    // Icones dos slots (um unico modo para todos os blits)
    int set = hud_icon_set();
    hud_sprite_mode();
    for (int i = 0; i < 6; i++)
        hud_blit(i < 5 ? s_icon[set][i] : s_potion, s_slot[i].x, s_slot[i].y);

    // Recarga, indisponivel e ativo
    for (int i = 0; i < 6; i++) {
        const HudSlot *sl = &s_slot[i];
        HudSlotState st = hud_slot_state(i);
        if (st.cd > 0.0f && st.cd_total > 0.0f) hud_shade_round(sl, st.cd / st.cd_total, 160);
        else if (!st.enabled) hud_shade_round(sl, 1.0f, 140);
        if (st.selected) hud_ring(sl, RGBA32(214, 170, 82, 255));
    }

    // Glifos dos botoes + pip R
    hud_sprite_mode();
    for (int i = 0; i < 6; i++)
        hud_blit(s_btn[i], s_slot[i].x - 3, s_slot[i].y + s_slot[i].size - 8);
    hud_blit(s_btn[HB_R], HUD_R_X, HUD_R_Y);
    if (g_hero == HERO_BARBARIAN && g_druid.rush_timer <= 0.0f && barb_cooldown(BARB_CD_RUSH) > 0.0f)
        hud_shade(HUD_R_X + 1, HUD_R_Y + 1, HUD_R_X + 8, HUD_R_Y + 8, 150);

    // Barra de XP fina no centro inferior
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(18, 12, 8, 255));
    rdpq_fill_rectangle(HUD_XP_X0, HUD_XP_Y, HUD_XP_X1, HUD_XP_Y + 3);
    if (g_druid.exp_next > 0) {
        float xr = clampf((float)g_druid.exp / (float)g_druid.exp_next, 0.0f, 1.0f);
        int w = (int)((float)(HUD_XP_X1 - HUD_XP_X0) * xr);
        if (w > 0) {
            rdpq_set_prim_color(RGBA32(214, 170, 82, 255));
            rdpq_fill_rectangle(HUD_XP_X0, HUD_XP_Y, HUD_XP_X0 + w, HUD_XP_Y + 3);
        }
    }

    Enemy *boss = enemies_get_boss();
    if (boss && boss->hp > 0) {
        // Preenchimento alinhado ao vao da moldura (x 28..210, y 9..19 do sprite)
        float ratio = clampf((float)boss->hp / (float)boss->max_hp, 0, 1);
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(boss->boss_phase == 2 ?
            RGBA32(235, 36, 22, 255) : RGBA32(170, 22, 40, 255));
        rdpq_fill_rectangle(HUD_BOSS_X + 28, HUD_BOSS_Y + 9, HUD_BOSS_X + 28 + (int)(183 * ratio), HUD_BOSS_Y + 20);
        sprites_draw_ui(SPR_UI_BOSS_BAR, HUD_BOSS_X, HUD_BOSS_Y, 1);
    } else {
        // Essencia demoniaca (moeda do mascate): oculta durante o chefe
        sprite_t *icon = s_ess[(get_ticks_ms() / 160) % 4];
        if (icon) { hud_sprite_mode(); hud_blit(icon, 14, 10); rdpq_mode_blender(0); }
    }
    if (s_banner_timer > 0) sprites_draw_ui(SPR_UI_BANNER, 20, 41, 1);
}

void hud_draw(void) {
    // The minimap moves below the boss HUD instead of disappearing during combat.
    world_draw_minimap();
    world_draw_objectives();
    if (sprites_druid_ready()) { hud_draw_sprites(); return; }
    // 1. Bottom Left: LIFE ORB
    int orb_x = 10;
    int orb_y = 196;
    int orb_w = 34;
    int orb_h = 34;

    draw_bevel_box(orb_x - 2, orb_y - 2, orb_x + orb_w + 2, orb_y + orb_h + 2, RGBA32(10, 10, 14, 255), RGBA32(65, 68, 75, 255));
    draw_bevel_box(orb_x, orb_y, orb_x + orb_w, orb_y + orb_h, RGBA32(18, 5, 5, 255), RGBA32(40, 15, 15, 255));

    float hp_ratio = (g_druid.max_hp > 0) ? (float)g_druid.hp / (float)g_druid.max_hp : 0.0f;
    hp_ratio = clampf(hp_ratio, 0.0f, 1.0f);
    int fill_h = (int)(hp_ratio * (orb_h - 2));
    if (fill_h > 0) {
        int fill_top = (orb_y + orb_h - 1) - fill_h;
        rdpq_set_prim_color(RGBA32(190, 20, 25, 255));
        rdpq_fill_rectangle(orb_x + 1, fill_top, orb_x + orb_w - 1, orb_y + orb_h - 1);
        rdpq_set_prim_color(RGBA32(245, 90, 95, 255));
        int gloss_bot = (fill_top + 4 < orb_y + orb_h - 1) ? fill_top + 4 : orb_y + orb_h - 1;
        rdpq_fill_rectangle(orb_x + 3, fill_top, orb_x + 8, gloss_bot);
    }

    // 2. Bottom Right: SPIRIT ORB
    int s_orb_x = SCREEN_WIDTH - 10 - orb_w;
    int s_orb_y = orb_y;

    draw_bevel_box(s_orb_x - 2, s_orb_y - 2, s_orb_x + orb_w + 2, s_orb_y + orb_h + 2, RGBA32(10, 10, 14, 255), RGBA32(65, 68, 75, 255));
    draw_bevel_box(s_orb_x, s_orb_y, s_orb_x + orb_w, s_orb_y + orb_h, RGBA32(5, 12, 18, 255), RGBA32(12, 28, 40, 255));

    float mp_ratio = (g_druid.max_spirit > 0) ? g_druid.spirit / g_druid.max_spirit : 0.0f;
    mp_ratio = clampf(mp_ratio, 0.0f, 1.0f);
    int s_fill_h = (int)(mp_ratio * (orb_h - 2));
    if (s_fill_h > 0) {
        int s_fill_top = (s_orb_y + orb_h - 1) - s_fill_h;
        rdpq_set_prim_color(RGBA32(0, 160, 220, 255));
        rdpq_fill_rectangle(s_orb_x + 1, s_fill_top, s_orb_x + orb_w - 1, s_orb_y + orb_h - 1);
        rdpq_set_prim_color(RGBA32(120, 230, 255, 255));
        int s_gloss_bot = (s_fill_top + 4 < s_orb_y + orb_h - 1) ? s_fill_top + 4 : s_orb_y + orb_h - 1;
        rdpq_fill_rectangle(s_orb_x + 3, s_fill_top, s_orb_x + 8, s_gloss_bot);
    }

    // 3. Bottom Center: ABILITY & POTION BELT
    int belt_x = 52;
    int belt_y = 208;
    int slot_w = 34;
    int slot_h = 22;
    int slot_spacing = 36;

    color_t a_border = (g_druid.state == DRUID_STATE_ATTACK) ? RGBA32(240, 180, 50, 255) : RGBA32(65, 68, 75, 255);
    draw_bevel_box(belt_x + 0 * slot_spacing, belt_y, belt_x + 0 * slot_spacing + slot_w, belt_y + slot_h, RGBA32(24, 24, 28, 255), a_border);

    color_t b_border = (g_druid.spirit >= 15.0f) ? RGBA32(0, 190, 240, 255) : RGBA32(40, 45, 50, 255);
    draw_bevel_box(belt_x + 1 * slot_spacing, belt_y, belt_x + 1 * slot_spacing + slot_w, belt_y + slot_h, RGBA32(24, 24, 28, 255), b_border);

    color_t cl_border = (g_druid.spirit >= 25.0f) ? RGBA32(180, 40, 60, 255) : RGBA32(40, 45, 50, 255);
    draw_bevel_box(belt_x + 2 * slot_spacing, belt_y, belt_x + 2 * slot_spacing + slot_w, belt_y + slot_h, RGBA32(24, 24, 28, 255), cl_border);

    color_t cu_border = (g_druid.spirit >= 40.0f) ? RGBA32(50, 220, 160, 255) : RGBA32(40, 45, 50, 255);
    draw_bevel_box(belt_x + 3 * slot_spacing, belt_y, belt_x + 3 * slot_spacing + slot_w, belt_y + slot_h, RGBA32(24, 24, 28, 255), cu_border);

    color_t cr_border = g_druid.is_beast ? RGBA32(255, 60, 0, 255) : RGBA32(160, 100, 50, 255);
    draw_bevel_box(belt_x + 4 * slot_spacing, belt_y, belt_x + 4 * slot_spacing + slot_w, belt_y + slot_h, g_druid.is_beast ? RGBA32(60, 15, 10, 255) : RGBA32(24, 24, 28, 255), cr_border);

    draw_bevel_box(belt_x + 5 * slot_spacing, belt_y, belt_x + 5 * slot_spacing + slot_w, belt_y + slot_h, RGBA32(24, 24, 28, 255), RGBA32(190, 30, 40, 255));

    // 4. TOP: BOSS HEALTH BAR
    Enemy *boss = enemies_get_boss();
    if (boss && boss->hp > 0) {
        int bb_x = 55;
        int bb_y = 12;
        int bb_w = 210;
        int bb_h = 10;

        draw_bevel_box(bb_x - 2, bb_y - 2, bb_x + bb_w + 2, bb_y + bb_h + 2, RGBA32(15, 12, 15, 255), RGBA32(180, 140, 40, 255));

        float boss_ratio = (float)boss->hp / (float)boss->max_hp;
        int boss_fill_w = (int)(boss_ratio * (bb_w - 2));
        if (boss_fill_w > 0) {
            color_t fill_c = (boss->boss_phase == 2) ? RGBA32(255, 30, 10, 255) : RGBA32(180, 20, 30, 255);
            rdpq_set_prim_color(fill_c);
            rdpq_fill_rectangle(bb_x + 1, bb_y + 1, bb_x + 1 + boss_fill_w, bb_y + bb_h - 1);
        }
    }

    // 5. BANNER BACKDROP
    if (s_banner_timer > 0.0f) {
        int ban_y = 45;
        int ban_h = 32;
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(5, 5, 8, 200));
        rdpq_fill_rectangle(20, ban_y, SCREEN_WIDTH - 20, ban_y + ban_h);
        rdpq_set_prim_color(RGBA32(180, 140, 50, 255));
        rdpq_fill_rectangle(20, ban_y, SCREEN_WIDTH - 20, ban_y + 1);
        rdpq_fill_rectangle(20, ban_y + ban_h - 1, SCREEN_WIDTH - 20, ban_y + ban_h);
    }
}

// Texto com contorno de 1 px (4 direcoes) centralizado em [x, x+w)
static void hud_outlined(int x, int y, int w, int style, const char *s) {
    static const int8_t off[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
    for (int k = 0; k < 4; k++) ui_print(false, x + off[k][0], y + off[k][1], UI_STYLE_SHADOW, 1, w, s);
    ui_print(false, x, y, style, 1, w, s);
}

// Passo de texto (rdpq_text) - chamar enquanto o display ainda esta anexado.
void hud_draw_text(void) {
    const char *objective=world_boss_objective_text();
    if(objective[0])ui_text_center(86,objective);
    if (sprites_druid_ready()) {
        char b[40];
        // Vida e espirito centralizados nos orbes
        snprintf(b, sizeof(b), "%d", g_druid.hp);
        hud_outlined(HUD_ORB_L_CX - 20, HUD_ORB_CY + 4, 40, UI_STYLE_NORMAL, b);
        snprintf(b, sizeof(b), "%d", (int)g_druid.spirit);
        hud_outlined(HUD_ORB_R_CX - 20, HUD_ORB_CY + 4, 40, UI_STYLE_NORMAL, b);
        // Pocoes de vida no canto do slot Z
        snprintf(b, sizeof(b), "%d", g_druid.health_potions);
        hud_outlined(s_slot[5].x + 11, s_slot[5].y + 18, 12, UI_STYLE_GOLD, b);
        // Nivel sobre a barra de XP
        snprintf(b, sizeof(b), T(S_HUD_LV), g_druid.level);
        ui_print(false, 0, HUD_XP_Y - 3, UI_STYLE_GOLD, 1, SCREEN_WIDTH, b);
        if (g_druid.continues > 0) ui_textf(16, 46, T(S_HUD_CONT), g_druid.continues);
        if (g_hero == HERO_BARBARIAN) {
            for (int i = 1; i <= 4; i++) {   // segundos de recarga no centro do slot
                float cd = barb_cooldown((BarbCooldown)(i - 1));
                if (cd > 0.0f) {
                    snprintf(b, sizeof(b), "%d", (int)ceilf(cd));
                    hud_outlined(s_slot[i].x, s_slot[i].y + s_slot[i].size / 2 + 4, s_slot[i].size, UI_STYLE_GOLD, b);
                }
            }
            float cd = barb_cooldown(BARB_CD_RUSH);   // R: tempo de impulso (dourado) ou de recarga
            if (g_druid.rush_timer > 0.0f || cd > 0.0f) {
                snprintf(b, sizeof(b), "%d", (int)ceilf(g_druid.rush_timer > 0.0f ? g_druid.rush_timer : cd));
                ui_print(false, HUD_R_X - 26, HUD_R_Y + 8, g_druid.rush_timer > 0.0f ? UI_STYLE_GOLD : UI_STYLE_NORMAL, 2, 24, b);
            }
        }
        if (!hud_boss_active()) {   // quantidade de essencia ao lado do icone
            char eb[16]; snprintf(eb, sizeof(eb), "%d", g_druid.essence);
            ui_print(true, 40, 29, UI_STYLE_NORMAL, 0, 0, eb);
        }
        if (world_near_vendor()) ui_print(true, 0, 150, UI_STYLE_GOLD, 1, SCREEN_WIDTH, T(S_SHOP_PROMPT));
        Enemy *boss = enemies_get_boss();
        if (boss && boss->hp > 0) ui_text_center(52,
            campaign_boss_name(g_campaign,boss->boss_phase==2));
        if (s_banner_timer > 0) {   // fonte gotica: linha de base mais baixa que a fonte de debug
            if (s_banner_title[0]) ui_print(true, 0, 64, UI_STYLE_GOLD, 1, SCREEN_WIDTH, s_banner_title);
            if (s_banner_sub[0]) ui_text_center(75, s_banner_sub);
        }
        if (s_msg_timer > 0 && s_msg_text[0]) ui_text_center(168, s_msg_text);
        return;
    }
    // HP / MP numericos
    ui_textf(16, 208, "%d", g_druid.hp);
    ui_textf(SCREEN_WIDTH - 36, 208, "%d", (int)g_druid.spirit);

    int belt_x = 52;
    int belt_y = 214;
    int slot_spacing = 36;

    ui_text(belt_x + 0 * slot_spacing + 4, belt_y, g_druid.is_beast ? "A:GARRA" : "A:ATQ");
    ui_text(belt_x + 1 * slot_spacing + 5, belt_y, "B:RAIO");
    ui_text(belt_x + 2 * slot_spacing + 5, belt_y, "C<ESP");
    ui_text(belt_x + 3 * slot_spacing + 5, belt_y, "C+LOBO");
    ui_text(belt_x + 4 * slot_spacing + 5, belt_y, g_druid.is_beast ? "FERA" : "C>FERA");
    ui_textf(belt_x + 5 * slot_spacing + 5, belt_y, "Zx%d", g_druid.health_potions);

    // Nome do chefe
    Enemy *boss = enemies_get_boss();
    if (boss && boss->hp > 0) {
        ui_text_center(3, campaign_boss_name(g_campaign,boss->boss_phase==2));
    }

    // Nivel
    ui_textf(12, 184, "NV %d", g_druid.level);

    // Banner
    if (s_banner_timer > 0.0f) {
        if (s_banner_title[0] != '\0') ui_text_center(50, s_banner_title);
        if (s_banner_sub[0]   != '\0') ui_text_center(62, s_banner_sub);
    }

    // Mensagem
    if (s_msg_timer > 0.0f && s_msg_text[0] != '\0') {
        ui_text_center(168, s_msg_text);
    }
}
