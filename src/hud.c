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

static void hud_draw_sprites(void) {
    // Dynamic fills sit behind the transparent centers of the painted frames.
    draw_orb_fill(26, 214, (float)g_druid.hp / (float)g_druid.max_hp,
                  RGBA32(205, 20, 36, 255));
    draw_orb_fill(294, 214, g_druid.spirit / g_druid.max_spirit,
                  g_hero == HERO_BARBARIAN ? RGBA32(230, 110, 20, 255) : RGBA32(28, 145, 230, 255));   // barbaro: furia laranja
    sprites_draw_ui(SPR_UI_LIFE_ORB, 4, 192, 1);
    sprites_draw_ui(SPR_UI_SPIRIT_ORB, 272, 192, 1);
    sprites_draw_ui(SPR_UI_BELT, 50, 201, 1);

    if (g_hero == HERO_BARBARIAN) {  // cinto do barbaro: machado, salto, grito, redemoinho, berserker, pocao
        for (int i = 0; i < 6; i++) {
            bool enabled = i == 0 || (i == 1 && g_druid.spirit >= barb_leap_cost()) ||
                (i == 2 && g_druid.spirit >= BARB_SHOUT_COST) || (i == 3 && g_druid.spirit >= BARB_SPIN_COST) ||
                (i == 4 && (g_druid.berserk_timer > 0 || g_druid.spirit >= BARB_BERSERK_COST)) ||
                (i == 5 && (g_druid.health_potions || g_druid.spirit_potions));
            bool selected = (i == 0 && g_druid.state == DRUID_STATE_ATTACK) || (i == 3 && g_druid.spin_timer > 0) ||
                            (i == 4 && g_druid.berserk_timer > 0);
            if (!enabled || selected)
                sprites_draw_ui(selected ? SPR_UI_SELECTED : SPR_UI_DISABLED, 51 + i * 36, 201, 1);
            sprites_draw_ui(i < 5 ? (SpriteUi)(SPR_UI_B_AXE + i) : SPR_UI_POTION, 56 + i * 36, 206, .8f);
        }
    } else
    if (g_hero == HERO_HUNTRESS) {   // cinto da cacadora: tiro, rajada, raizes, gaviao, foco, pocao
        for (int i = 0; i < 6; i++) {
            bool enabled = i == 0 || (i == 1 && g_druid.spirit >= hunt_volley_cost()) ||
                (i == 2 && g_druid.spirit >= HUNT_ROOTS_COST) || (i == 3 && g_druid.spirit >= HUNT_HAWK_COST) ||
                (i == 4 && (g_druid.focus_timer > 0 || g_druid.spirit >= HUNT_FOCUS_COST)) ||
                (i == 5 && (g_druid.health_potions || g_druid.spirit_potions));
            bool selected = (i == 0 && g_druid.state == DRUID_STATE_ATTACK) || (i == 4 && g_druid.focus_timer > 0);
            if (!enabled || selected)
                sprites_draw_ui(selected ? SPR_UI_SELECTED : SPR_UI_DISABLED, 51 + i * 36, 201, 1);
            sprites_draw_ui(i < 5 ? (SpriteUi)(SPR_UI_H_SHOT + i) : SPR_UI_POTION, 56 + i * 36, 206, .8f);
        }
    } else
    for (int i = 0; i < 6; i++) {
        bool enabled = i == 0 || (i == 1 && g_druid.spirit >= 15) ||
            (i == 2 && g_druid.spirit >= 25) ||
            (i == 3 && g_druid.spirit >= 40) ||
            (i == 4 && (g_druid.is_beast || g_druid.spirit >= 25)) ||
            (i == 5 && (g_druid.health_potions || g_druid.spirit_potions));
        bool selected = (i == 0 && g_druid.state == DRUID_STATE_ATTACK) ||
            (i == 4 && g_druid.is_beast);
        if (!enabled || selected)
            sprites_draw_ui(selected ? SPR_UI_SELECTED : SPR_UI_DISABLED,
                            51 + i * 36, 201, 1);
        sprites_draw_ui((SpriteUi)i, 56 + i * 36, 206, .8f);
    }

    Enemy *boss = enemies_get_boss();
    if (boss && boss->hp > 0) {
        float ratio = clampf((float)boss->hp / (float)boss->max_hp, 0, 1);
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(boss->boss_phase == 2 ?
            RGBA32(235, 36, 22, 255) : RGBA32(170, 22, 40, 255));
        rdpq_fill_rectangle(67, 13, 67 + (int)(186 * ratio), 23);
        sprites_draw_ui(SPR_UI_BOSS_BAR, 40, 2, 1);
    }
    if (s_banner_timer > 0) sprites_draw_ui(SPR_UI_BANNER, 20, 41, 1);
}

void hud_draw(void) {
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

// Passo de texto (rdpq_text) - chamar enquanto o display ainda esta anexado.
void hud_draw_text(void) {
    if (sprites_druid_ready()) {
        ui_textf(14, 210, "%d", g_druid.hp);
        ui_textf(282, 210, "%d", (int)g_druid.spirit);
        ui_textf(230, 225, "x%d", g_druid.health_potions);
        ui_textf(12, 183, T(S_HUD_LV), g_druid.level);
        if (g_druid.continues > 0) ui_textf(12, 168, T(S_HUD_CONT), g_druid.continues);
        {   // essencia demoniaca (moeda do mascate): icone animado no canto superior esquerdo + quantidade
            static sprite_t *ess[4];
            static bool tried = false;
            if (!tried) {
                tried = true;
                for (int i = 0; i < 4; i++) {
                    char path[48];
                    snprintf(path, sizeof(path), "rom:/spr_essence_d0_f%d.rgba16.sprite", i);
                    ess[i] = sprite_load(path);
                }
            }
            sprite_t *icon = ess[(get_ticks_ms() / 160) % 4];
            if (icon) {
                rdpq_set_mode_standard();
                rdpq_mode_alphacompare(1);
                rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
                rdpq_sprite_blit(icon, 6, 6, &(rdpq_blitparms_t){ .scale_x = 0.8f, .scale_y = 0.8f });
                rdpq_mode_blender(0);
            }
            char eb[16]; snprintf(eb, sizeof(eb), "%d", g_druid.essence);
            ui_print(true, 32, 26, UI_STYLE_NORMAL, 0, 0, eb);
        }
        if (world_near_vendor()) ui_print(true, 0, 150, UI_STYLE_GOLD, 1, SCREEN_WIDTH, T(S_SHOP_PROMPT));
        Enemy *boss = enemies_get_boss();
        if (boss && boss->hp > 0) ui_text_center(31,
            boss->boss_phase == 2 ? T(g_campaign == 2 ? S_BOSS2N : g_campaign == 1 ? S_BOSS2F : S_BOSS2) : T(g_campaign == 2 ? S_BOSS1N : g_campaign == 1 ? S_BOSS1F : S_BOSS1));
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
        if (boss->boss_phase == 2) {
            ui_text_center(3, "MALAKOR - FURIA DAS TREVAS");
        } else {
            ui_text_center(3, "MALAKOR - GUARDIAO PROFANADO");
        }
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
