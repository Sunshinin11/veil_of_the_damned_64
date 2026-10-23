#include "menu.h"
#include <stdarg.h>
#include "gfx.h"
#include "sprites.h"
#include "audio.h"
#include "world.h"
#include "druid.h"
#include "cards.h"
#include "lang.h"
#include "save.h"
#include "huntress.h"
#include "barbarian.h"

static sprite_t *s_bg = NULL, *s_logo = NULL;
static int s_cursor = 0;
enum { PAGE_MAIN, PAGE_STATUS, PAGE_ABILITIES, PAGE_CARDS };
static int s_page = PAGE_MAIN;   // pagina do menu de pausa
static int s_nav_dir = 0;            // direcao segurada no analogico (para repetir)
static uint32_t s_nav_next = 0;

void menu_init(void) {
    s_bg = sprite_load("rom:/menu_bg.rgba16.sprite");
    s_logo = sprite_load("rom:/menu_logo.rgba32.sprite");
}

void menu_reset_cursor(void) { s_cursor = 0; s_page = PAGE_MAIN; }

// -1 = cima, +1 = baixo (d-pad, C ou analogico com repeticao ao segurar)
static int nav_vertical(const joypad_inputs_t *in, const joypad_buttons_t *p) {
    if (p->d_up || p->c_up) return -1;
    if (p->d_down || p->c_down) return 1;
    int d = in->stick_y > 45 ? -1 : in->stick_y < -45 ? 1 : 0;
    uint32_t now = get_ticks_ms();
    if (d == 0) { s_nav_dir = 0; return 0; }
    if (d != s_nav_dir) { s_nav_dir = d; s_nav_next = now + 330; return d; }
    if (now >= s_nav_next) { s_nav_next = now + 140; return d; }
    return 0;
}

static void move_cursor(int d, int count, const bool *enabled) {
    if (!d) return;
    for (int k = 0; k < count; k++) {
        s_cursor = (s_cursor + d + count) % count;
        if (!enabled || enabled[s_cursor]) break;
    }
    sound_play(SFX_SWING);
}

// Fundo com efeito Ken Burns: zoom e deslocamento lentos e continuos (sempre cobrindo a tela).
static void draw_ken_burns(int dim_alpha) {
    rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (s_bg) {
        float t = (float)get_ticks_ms() * 0.001f;
        float base = (float)SCREEN_WIDTH / (float)s_bg->width;               // 384 -> 320 cobre a tela
        float zoom = 1.08f + 0.07f * sinf(t * 0.045f);
        float sc = base * zoom;
        float w = s_bg->width * sc, h = s_bg->height * sc;
        float px = 0.5f + 0.5f * sinf(t * 0.031f), py = 0.5f + 0.5f * cosf(t * 0.023f);
        float x = -(w - SCREEN_WIDTH) * px, y = -(h - SCREEN_HEIGHT) * py;
        rdpq_set_mode_standard();
        rdpq_mode_filter(FILTER_BILINEAR);
        rdpq_sprite_blit(s_bg, x, y, &(rdpq_blitparms_t){ .scale_x = sc, .scale_y = sc });
    }
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    if (dim_alpha > 0) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, dim_alpha));
        rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    // escurece a faixa da esquerda (onde ficam logo e menu) em degrade
    for (int i = 0; i < 12; i++) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 150 - i * 12));
        rdpq_fill_rectangle(i * 16, 0, i * 16 + 16, SCREEN_HEIGHT);
    }
    rdpq_mode_blender(0);
}

static void draw_logo(int x, int y) {
    if (!s_logo) return;
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_sprite_blit(s_logo, x, y, NULL);
}

static void draw_items(const char *const *items, const bool *enabled, int count, int x, int y0, int step) {
    for (int i = 0; i < count; i++) {
        bool sel = (i == s_cursor);
        int style = !enabled[i] ? UI_STYLE_DISABLED : sel ? UI_STYLE_SELECTED : UI_STYLE_NORMAL;
        char buf[48];
        snprintf(buf, sizeof(buf), "%s%s", sel ? "\xC2\xBB " : "", items[i]);   // "» "
        ui_print(true, sel ? x : x + 10, y0 + i * step, style, 0, 0, buf);
    }
}

// ---- Tela inicial: Novo Jogo / Escolher Fase / Carregar / Opcoes (+ pagina de Opcoes) ----
static int s_nav_h = 0;
static bool s_in_options = false;
static int s_opt_cursor = 0;
static float s_msg_t = 0.0f;           // aviso curto (salvo / sem save)
static StrId s_msg_id = S_SAVED;

static int nav_horizontal(const joypad_inputs_t *in, const joypad_buttons_t *p) {
    if (p->d_left || p->c_left || p->l) return -1;
    if (p->d_right || p->c_right || p->r) return 1;
    int d = in->stick_x > 50 ? 1 : in->stick_x < -50 ? -1 : 0;
    if (d != 0 && d != s_nav_h) { s_nav_h = d; return d; }
    if (d == 0) s_nav_h = 0;
    return 0;
}

// Versao final: sem seletor de fase (era so para teste).
#define TITLE_N 3
static void title_items(const char **items, bool *enabled) {
    items[0] = T(S_NEW_GAME); items[1] = T(S_LOAD_GAME); items[2] = T(S_OPTIONS);
    enabled[0] = enabled[2] = true;
    enabled[1] = save_has_game();
}

// Opcoes: idioma, neblina, volume da musica, volume dos efeitos, controles
#define OPT_COUNT 5
static void options_change(int row, int d) {
    switch (row) {
        case 0: g_opt.lang = (g_opt.lang + d + LANG_COUNT) % LANG_COUNT; break;
        case 1: g_opt.fog = !g_opt.fog; break;
        case 2: g_opt.music += d; if (g_opt.music < 0) g_opt.music = 0; if (g_opt.music > 10) g_opt.music = 10; break;
        case 3: g_opt.sfx += d;   if (g_opt.sfx < 0) g_opt.sfx = 0;     if (g_opt.sfx > 10) g_opt.sfx = 10; break;
        default: g_opt.controls = !g_opt.controls; break;
    }
    options_apply();            // aplica na hora (o idioma muda o proprio menu)
}

static MenuAction options_update(const joypad_inputs_t *in, const joypad_buttons_t *p) {
    int d = nav_vertical(in, p);
    if (d) { s_opt_cursor = (s_opt_cursor + d + OPT_COUNT) % OPT_COUNT; sound_play(SFX_SWING); }
    int h = nav_horizontal(in, p);
    if (h) { options_change(s_opt_cursor, h); sound_play(SFX_SWING); }
    if (p->a) { options_change(s_opt_cursor, 1); sound_play(SFX_SWING); }
    if (p->b || p->start) { save_write_options(); s_in_options = false; sound_play(SFX_SWING); }
    return MENU_NONE;
}

static void bar(int x, int y, int v) {        // barrinha de volume 0..10
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    for (int i = 0; i < 10; i++) {
        rdpq_set_prim_color(i < v ? RGBA32(214, 60, 40, 255) : RGBA32(60, 40, 36, 255));
        rdpq_fill_rectangle(x + i * 7, y - 8, x + i * 7 + 5, y);
    }
}

static void options_draw(void) {
    draw_ken_burns(120);
    ui_print(true, 22, 40, UI_STYLE_GOLD, 0, 0, T(S_OPTIONS));
    StrId labels[OPT_COUNT] = { S_OPT_LANGUAGE, S_OPT_FOG, S_OPT_MUSIC, S_OPT_SFX, S_OPT_CONTROLS };
    for (int i = 0; i < OPT_COUNT; i++) {
        int y = 74 + i * 26;
        bool sel = (i == s_opt_cursor);
        ui_print(false, 26, y, sel ? UI_STYLE_SELECTED : UI_STYLE_NORMAL, 0, 0, T(labels[i]));
        int st = sel ? UI_STYLE_GOLD : UI_STYLE_NORMAL;
        switch (i) {
            case 0: ui_print(false, 150, y, st, 0, 0, T(S_LANG_NAME)); break;
            case 1: ui_print(false, 150, y, st, 0, 0, T(g_opt.fog ? S_ON : S_OFF)); break;
            case 2: bar(150, y, g_opt.music); break;
            case 3: bar(150, y, g_opt.sfx); break;
            default: ui_print(false, 26, y + 13, st, 0, 0, T(g_opt.controls ? S_CTRL_SWAP : S_CTRL_CLASSIC)); break;
        }
    }
    ui_print(false, 22, 230, UI_STYLE_DISABLED, 0, 0, T(S_OPT_HINT));
}

MenuAction menu_title_update(const joypad_inputs_t *in, const joypad_buttons_t *p, int *start_area) {
    if (s_msg_t > 0.0f) s_msg_t -= 1.0f / 60.0f;
    // Unico atalho (escondido): Z+L+R+START libera/trava tudo.
    if (p->start && in->btn.z && in->btn.l && in->btn.r) {
        progress_toggle_all();
        sound_play(SFX_VICTORY);
        return MENU_NONE;
    }
    if (s_in_options) return options_update(in, p);
    const char *items[TITLE_N]; bool en[TITLE_N];
    title_items(items, en);
    if (s_cursor >= TITLE_N || !en[s_cursor]) s_cursor = 0;
    move_cursor(nav_vertical(in, p), TITLE_N, en);
    if (p->a || p->start) {
        sound_play(SFX_SHAPESHIFT);
        if (s_cursor == 0) { *start_area = 0; return MENU_NEW_GAME; }   // Novo Jogo: fase 1 da campanha escolhida
        if (s_cursor == 1) return MENU_LOAD;
        s_in_options = true; s_opt_cursor = 0;
        return MENU_OPTIONS;
    }
    return MENU_NONE;
}

void menu_title_draw(int start_area) {
    (void)start_area;
    if (s_in_options) { options_draw(); return; }
    draw_ken_burns(0);
    draw_logo(14, 14);
    const char *items[TITLE_N]; bool en[TITLE_N];
    title_items(items, en);
    draw_items(items, en, TITLE_N, 22, 112, 28);
    if (s_cursor == 1) ui_print(false, 36, 112 + 28 + 13, UI_STYLE_DISABLED, 0, 0, T(S_SAVE_NOTE));
    ui_print(false, 22, 230, UI_STYLE_DISABLED, 0, 0, T(S_A_SELECT));
}

// ---- Escolha de personagem ----
static int s_hero_sel = HERO_BARBARIAN, s_camp_sel = 2, s_diff_sel = DIFF_NORMAL;
int menu_selected_hero(void) { return s_hero_sel; }
int menu_selected_campaign(void) { return s_camp_sel; }
int menu_selected_difficulty(void) { return s_diff_sel; }
static const StrId DIFF_NAMES[DIFF_COUNT] = { S_DIFF0, S_DIFF1, S_DIFF2 };

MenuAction menu_char_update(const joypad_inputs_t *in, const joypad_buttons_t *p, int *start_area) {
    static int last_start = -1;
    if (*start_area != last_start) {          // veio do titulo: a fase escolhida define a campanha
        last_start = *start_area;
        s_camp_sel = *start_area / AREA_COUNT;
        if (!campaign_unlocked(s_camp_sel)) s_camp_sel = 2;   // Norte: sempre aberto
    }
    // d-pad / analogico: heroi; L/R: campanha; C-cima/C-baixo: dificuldade
    int h = p->d_left ? -1 : p->d_right ? 1 : 0;
    if (!h) { int d = in->stick_x > 50 ? 1 : in->stick_x < -50 ? -1 : 0; if (d != 0 && d != s_nav_h) h = d; s_nav_h = d; }
    if (h) { s_hero_sel = (s_hero_sel + h + HERO_COUNT) % HERO_COUNT; sound_play(SFX_SWING); }
    if (p->l || p->r || p->c_left || p->c_right) {
        int d = (p->l || p->c_left) ? -1 : 1;
        s_camp_sel = (s_camp_sel + d + CAMPAIGN_COUNT) % CAMPAIGN_COUNT;
        sound_play(SFX_SWING);
    }
    int maxd = campaign_unlocked(s_camp_sel) ? campaign_max_difficulty(s_camp_sel) : 0;
    if (p->c_up && s_diff_sel < maxd) { s_diff_sel++; sound_play(SFX_SWING); }
    if (p->c_down && s_diff_sel > 0)  { s_diff_sel--; sound_play(SFX_SWING); }
    if (s_diff_sel > maxd) s_diff_sel = maxd;
    if (p->a || p->start) {
        if (!hero_unlocked(s_hero_sel) || !campaign_unlocked(s_camp_sel)) { sound_play(SFX_PLAYER_HURT); return MENU_NONE; }
        sound_play(SFX_SHAPESHIFT);
        return MENU_CONFIRM;
    }
    if (p->b) { sound_play(SFX_SWING); return MENU_BACK; }
    return MENU_NONE;
}

void menu_char_draw(int start_area) {
    (void)start_area;
    draw_ken_burns(110);
    ui_print(true, 22, 34, UI_STYLE_NORMAL, 0, 0, T(S_CHOOSE_HERO));
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(10, 6, 6, 150));
    rdpq_fill_rectangle(22, 50, 150, 186);
    rdpq_set_prim_color(RGBA32(150, 30, 22, 220));
    rdpq_fill_rectangle(22, 50, 150, 51);
    rdpq_fill_rectangle(22, 185, 150, 186);
    rdpq_mode_blender(0);
    bool open = hero_unlocked(s_hero_sel);
    bool hu = s_hero_sel == HERO_HUNTRESS, ba = s_hero_sel == HERO_BARBARIAN;
    sprites_draw_hero_portrait_ex(s_hero_sel, 86, 178, 1.9f, !open);   // travado: so a silhueta
    if (open) {
        StrId base = hu ? S_HUNT_NAME : ba ? S_BARB_NAME : s_hero_sel==HERO_MAGE ? S_MAGE_NAME : S_DRUID_NAME;
        ui_print(true, 166, 76, UI_STYLE_GOLD, 0, 0, T(base));
        for (int k = 1; k <= 4; k++) ui_print(false, 166, 82 + k * 16, UI_STYLE_NORMAL, 0, 0, T((StrId)(base + k)));
    } else if (s_hero_sel==HERO_ROGUE) {
        ui_print(true,166,76,UI_STYLE_DISABLED,0,0,T(S_ROGUE_NAME));
        ui_print(false,166,98,UI_STYLE_DISABLED,0,0,T(S_LOCKED));
        ui_print(false,166,120,UI_STYLE_NORMAL,0,0,T(S_ART_PENDING));
    } else {                                   // nada de dica: so o misterio
        ui_print(true, 166, 76, UI_STYLE_DISABLED, 0, 0, "???");
        ui_print(false, 166, 98, UI_STYLE_DISABLED, 0, 0, T(S_LOCKED));
    }
    char nb[24]; snprintf(nb, sizeof(nb), "\xC2\xAB  %d / %d  \xC2\xBB", s_hero_sel + 1, HERO_COUNT);
    ui_print(false, 166, 172, UI_STYLE_DISABLED, 0, 0, nb);
    char buf[80];
    bool copen = campaign_unlocked(s_camp_sel);
    snprintf(buf, sizeof(buf), T(S_CAMP_LINE), copen ? T((StrId)(S_CAMP0 + s_camp_sel)) : "???");
    ui_print(false, 22, 200, copen ? UI_STYLE_NORMAL : UI_STYLE_DISABLED, 0, 0, buf);
    snprintf(buf, sizeof(buf), T(S_DIFF_LINE), T(DIFF_NAMES[s_diff_sel]));
    ui_print(false, 22, 214, s_diff_sel == DIFF_HELL ? UI_STYLE_SELECTED : s_diff_sel ? UI_STYLE_GOLD : UI_STYLE_NORMAL, 0, 0, buf);
    ui_print(false, 22, 230, UI_STYLE_DISABLED, 0, 0, T(S_CHAR_KEYS));
}

// ---- Pausa: Continuar / Status / Habilidades / Cartas / Salvar / Sair ----
static const StrId PAUSE_IDS[6] = { S_RESUME, S_STATUS, S_ABILITIES, S_CARDS, S_SAVE_GAME, S_QUIT_TITLE };
static bool PAUSE_ENABLED[6] = { true, true, true, true, true, true };
static int abil_count(void);
static int s_sub = 0;            // cursor dentro da pagina (habilidade / carta)
static int s_main_cursor = 0;

static bool rom_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

MenuAction menu_pause_update(const joypad_inputs_t *in, const joypad_buttons_t *p) {
    if (p->start) { s_page = PAGE_MAIN; return MENU_RESUME; }
    if (s_page != PAGE_MAIN) {
        int count = s_page == PAGE_ABILITIES ? abil_count() : s_page == PAGE_CARDS ? CARD_SLOTS : 0;
        int d = nav_vertical(in, p);
        if (d && count > 0) { s_sub = (s_sub + d + count) % count; sound_play(SFX_SWING); }
        if (p->b) { s_page = PAGE_MAIN; s_cursor = s_main_cursor; sound_play(SFX_SWING); }
        return MENU_NONE;
    }
    if (s_msg_t > 0.0f) s_msg_t -= 1.0f / 60.0f;
    PAUSE_ENABLED[4] = save_available();
    if (!PAUSE_ENABLED[s_cursor]) s_cursor = 0;
    move_cursor(nav_vertical(in, p), 6, PAUSE_ENABLED);
    if (p->b) return MENU_RESUME;
    if (p->a) {
        switch (s_cursor) {
            case 0: return MENU_RESUME;
            case 1: case 2: case 3:
                s_main_cursor = s_cursor;
                s_page = s_cursor == 1 ? PAGE_STATUS : s_cursor == 2 ? PAGE_ABILITIES : PAGE_CARDS;
                s_sub = 0;
                sound_play(SFX_SHAPESHIFT);
                return MENU_NONE;
            case 4: return MENU_SAVE;   // o main salva e chama menu_notice()
            default: s_page = PAGE_MAIN; return MENU_QUIT;
        }
    }
    return MENU_NONE;
}

static void panel(int x0, int y0, int x1, int y1) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(8, 4, 4, 190));
    rdpq_fill_rectangle(x0, y0, x1, y1);
    rdpq_set_prim_color(RGBA32(150, 30, 22, 230));
    rdpq_fill_rectangle(x0, y0, x1, y0 + 1);
    rdpq_fill_rectangle(x0, y1 - 1, x1, y1);
    rdpq_mode_blender(0);
}

static int pct(float f) { return (int)(f * 100.0f + (f >= 0 ? 0.5f : -0.5f)); }

static void line(int x, int *y, int style, const char *fmt, ...) {
    char buf[96];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    ui_print(false, x, *y, style, 0, 0, buf);
    *y += 15;
}

static void draw_status_hunt(void) {
    panel(16, 26, 304, 214);
    ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, T(S_STATUS));
    int y = 70, y2 = 70;
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_LEVEL), g_druid.level);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_EXP), g_druid.exp, g_druid.exp_next);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_LIFE), g_druid.hp, g_druid.max_hp);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_ENERGY), (int)g_druid.spirit, (int)g_druid.max_spirit);
    line(26, &y, (g_mods.melee_pct + g_mods.all_pct) ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_ARROW), hunt_arrow_damage());
    line(26, &y, (g_mods.wolf_pct + g_mods.all_pct) ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_HAWK), hunt_hawk_damage());
    line(170, &y2, g_mods.crit > 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_CRIT), pct(g_mods.crit + hero_crit()));
    line(170, &y2, g_mods.armor != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_ARMOR), pct(g_mods.armor + hero_armor()));
    line(170, &y2, g_mods.lifesteal > 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_STEAL), pct(g_mods.lifesteal));
    line(170, &y2, g_mods.speed_pct != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_SPEED), 100 + pct(g_mods.speed_pct));
    line(170, &y2, g_mods.regen_pct != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_REGEN), 100 + pct(g_mods.regen_pct));
    line(170, &y2, UI_STYLE_NORMAL, T(S_ST_POTIONS), g_druid.health_potions);
    line(170, &y2, UI_STYLE_NORMAL, T(S_ST_CARDS), cards_owned_count());
    if (g_seeds > 0) line(170, &y2, UI_STYLE_GOLD, T(S_ST_SEEDS), g_seeds);
    ui_print(false, 26, 204, UI_STYLE_DISABLED, 0, 0, T(S_ST_FOOT));
}

static void draw_status_barb(void) {
    panel(16, 26, 304, 214);
    ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, T(S_STATUS));
    int y = 70, y2 = 70;
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_LEVEL), g_druid.level);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_EXP), g_druid.exp, g_druid.exp_next);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_LIFE), g_druid.hp, g_druid.max_hp);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_FURY), (int)g_druid.spirit, (int)g_druid.max_spirit);
    line(26, &y, (g_mods.melee_pct + g_mods.all_pct) ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_AXE), barb_axe_damage(0));
    line(170, &y2, g_mods.crit > 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_CRIT), pct(g_mods.crit + hero_crit()));
    line(170, &y2, g_mods.armor != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_ARMOR), pct(g_mods.armor + hero_armor()));
    line(170, &y2, g_mods.lifesteal > 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_STEAL), pct(g_mods.lifesteal));
    line(170, &y2, g_mods.speed_pct != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_SPEED), 100 + pct(g_mods.speed_pct));
    line(170, &y2, UI_STYLE_NORMAL, T(S_ST_POTIONS), g_druid.health_potions);
    line(170, &y2, UI_STYLE_NORMAL, T(S_ST_CARDS), cards_owned_count());
    ui_print(false, 26, 204, UI_STYLE_DISABLED, 0, 0, T(S_FURY_NOTE));
}

static void draw_status(void) {
    if (g_hero == HERO_HUNTRESS) { draw_status_hunt(); return; }
    if (g_hero == HERO_BARBARIAN) { draw_status_barb(); return; }
    panel(16, 26, 304, 214);
    ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, T(S_STATUS));
    int staff = (int)((g_druid.attack_damage) * (1.0f + g_mods.melee_pct + g_mods.all_pct));
    int claw = (int)((g_druid.attack_damage + 14) * (1.0f + g_mods.melee_pct + g_mods.beast_pct + g_mods.all_pct));
    int bolt = (int)((32 + g_druid.level * 6) * (1.0f + g_mods.bolt_pct + g_mods.all_pct));
    int y = 70, y2 = 70;
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_LEVEL), g_druid.level);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_EXP), g_druid.exp, g_druid.exp_next);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_LIFE), g_druid.hp, g_druid.max_hp);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_SPIRIT), (int)g_druid.spirit, (int)g_druid.max_spirit);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_STAFF), staff);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_CLAW), claw);
    line(26, &y, UI_STYLE_NORMAL, T(S_ST_SPELL), bolt);
    line(170, &y2, g_mods.crit > 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_CRIT), pct(g_mods.crit + hero_crit()));
    line(170, &y2, g_mods.armor != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_ARMOR), pct(g_mods.armor + hero_armor()));
    line(170, &y2, g_mods.lifesteal > 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_STEAL), pct(g_mods.lifesteal));
    line(170, &y2, g_mods.speed_pct != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_SPEED), 100 + pct(g_mods.speed_pct));
    line(170, &y2, g_mods.regen_pct != 0 ? UI_STYLE_GOLD : UI_STYLE_NORMAL, T(S_ST_REGEN), 100 + pct(g_mods.regen_pct));
    line(170, &y2, UI_STYLE_NORMAL, T(S_ST_POTIONS), g_druid.health_potions);
    line(170, &y2, UI_STYLE_NORMAL, T(S_ST_CARDS), cards_owned_count());
    if (g_seeds > 0) line(170, &y2, UI_STYLE_GOLD, T(S_ST_SEEDS), g_seeds);
    ui_print(false, 26, 204, UI_STYLE_DISABLED, 0, 0, T(S_ST_FOOT));
}

static StrId ABIL_NAME(int i) { return (StrId)(S_AB0 + i); }
static const char *abil_key(int i) {
    switch (i) { case 0: return "A"; case 1: return "B"; case 2: return T(S_K_CLEFT); case 3: return T(S_K_CUP);
                 case 4: return T(S_K_CRIGHT); case 5: return "R"; default: return "Z"; }
}

// Descricao com os numeros atuais (nivel + cartas). Linhas alteradas por cartas saem em dourado.
static void draw_ability_desc(int id, int x, int y) {
    int N = UI_STYLE_NORMAL, G = UI_STYLE_GOLD;
    switch (id) {
        case 0: {
            int d = (int)(g_druid.attack_damage * (1.0f + g_mods.melee_pct + g_mods.all_pct));
            line(x, &y, N, "%s", T(S_A0_1));
            line(x, &y, (g_mods.melee_pct + g_mods.all_pct) ? G : N, T(S_A0_2), d);
            line(x, &y, N, "%s", T(S_A0_3));
            if (g_mods.lifesteal > 0) line(x, &y, G, T(S_A0_4), pct(g_mods.lifesteal));
            if (g_mods.crit > 0) line(x, &y, G, T(S_A0_5), pct(g_mods.crit));
        } break;
        case 1: {
            float c = 15.0f + g_mods.bolt_cost; if (c < 5) c = 5;
            int n = 1 + g_mods.bolt_extra, d = (int)((32 + g_druid.level * 6) * (1.0f + g_mods.bolt_pct + g_mods.all_pct));
            line(x, &y, g_mods.bolt_cost ? G : N, T(S_COST), (int)c);
            if (n > 1) line(x, &y, G, T(S_A1_1N), n); else line(x, &y, N, "%s", T(S_A1_1));
            line(x, &y, (g_mods.bolt_pct + g_mods.all_pct) ? G : N, T(S_A1_2), d);
            if (g_mods.bolt_pierce) line(x, &y, G, "%s", T(S_A1_3));
        } break;
        case 2: {
            int d = (int)((45 + g_druid.level * 8) * (1.0f + g_mods.thorn_pct + g_mods.all_pct));
            line(x, &y, N, T(S_COST), 25);
            line(x, &y, N, "%s", T(S_A2_1));
            line(x, &y, (g_mods.thorn_pct + g_mods.all_pct) ? G : N, T(S_A2_2), d);
            if (g_mods.thorn_radius_pct > 0) line(x, &y, G, T(S_A2_3), pct(g_mods.thorn_radius_pct));
        } break;
        case 3: {
            int n = 2 + g_mods.wolf_extra; if (n > 5) n = 5;
            int d = (int)((16 + g_druid.level * 4) * (1.0f + g_mods.wolf_pct + g_mods.all_pct));
            line(x, &y, N, T(S_COST), 40);
            line(x, &y, (g_mods.wolf_extra || g_mods.wolf_life) ? G : N, T(S_A3_1), n, (int)(18 + g_mods.wolf_life));
            line(x, &y, (g_mods.wolf_pct + g_mods.all_pct) ? G : N, T(S_A3_2), d);
        } break;
        case 4: {
            int d = (int)((g_druid.attack_damage + 14) * (1.0f + g_mods.melee_pct + g_mods.beast_pct + g_mods.all_pct));
            line(x, &y, N, T(S_COST), 25);
            line(x, &y, g_mods.beast_time ? G : N, T(S_A4_1), (int)(20 + g_mods.beast_time));
            line(x, &y, N, "%s", T(S_A4_2));
            line(x, &y, (g_mods.beast_pct + g_mods.all_pct + g_mods.melee_pct) ? G : N, T(S_A4_3), d);
            line(x, &y, N, "%s", T(S_A4_4));
        } break;
        case 5:
            line(x, &y, N, "%s", T(S_A5_1));
            line(x, &y, N, "%s", T(S_A5_2));
            line(x, &y, N, "%s", T(S_A5_3));
            break;
        default: {
            int h = (int)(45.0f * (1.0f + g_mods.potion_pct));
            line(x, &y, g_mods.potion_pct ? G : N, T(S_A6_1), h);
            line(x, &y, N, "%s", T(S_A6_2));
            line(x, &y, N, "%s", T(S_A6_3));
        } break;
    }
}

static int abil_count(void) { return g_hero == HERO_HUNTRESS ? 8 : 7; }   // barbaro: 7 (como o druida)
static const char *barb_key(int i) {
    switch (i) { case 0: return "A"; case 1: return "B"; case 2: return T(S_K_CLEFT); case 3: return T(S_K_CUP);
                 case 4: return T(S_K_CRIGHT); case 5: return "R"; default: return "Z"; }
}
static void draw_barb_desc(int id, int x, int y) {
    int N = UI_STYLE_NORMAL, G = UI_STYLE_GOLD;
    switch (id) {
        case 0:
            line(x, &y, (g_mods.melee_pct + g_mods.all_pct) ? G : N, T(S_BA0_1), barb_axe_damage(0));
            line(x, &y, g_mods.heavy_pct ? G : N, T(S_BA0_2), barb_axe_damage(2));
            line(x, &y, N, "%s", T(S_BA0_3));
            if (g_mods.lifesteal > 0) line(x, &y, G, T(S_A0_4), pct(g_mods.lifesteal));
            break;
        case 1:
            line(x, &y, g_mods.bolt_cost ? G : N, T(S_COST), (int)barb_leap_cost());
            line(x, &y, g_mods.leap_pct ? G : N, T(S_BA1_1), barb_leap_damage());
            line(x, &y, N, "%s", T(S_BA1_2));
            break;
        case 2:
            line(x, &y, N, T(S_COST), (int)BARB_SHOUT_COST);
            line(x, &y, (g_mods.root_time || g_mods.thorn_radius_pct) ? G : N, T(S_BA2_1), (double)barb_shout_stun());
            line(x, &y, N, "%s", T(S_BA2_2));
            break;
        case 3:
            line(x, &y, N, T(S_COST), (int)BARB_SPIN_COST);
            line(x, &y, (g_mods.wolf_pct || g_mods.wolf_life) ? G : N, T(S_BA3_1), (double)barb_spin_time(), barb_spin_damage());
            line(x, &y, N, "%s", T(S_BA3_2));
            break;
        case 4:
            line(x, &y, N, T(S_COST), (int)BARB_BERSERK_COST);
            line(x, &y, g_mods.beast_time ? G : N, T(S_BA4_1), (int)barb_berserk_time());
            line(x, &y, N, "%s", T(S_BA4_2));
            line(x, &y, N, "%s", T(S_BA4_3));
            line(x, &y, N, "%s", T(S_BA4_4));
            break;
        case 5:
            line(x, &y, N, "%s", T(S_BA5_1)); line(x, &y, N, "%s", T(S_BA5_2)); line(x, &y, N, "%s", T(S_BA5_3));
            break;
        default: draw_ability_desc(id, x, y); break;   // pocao: igual ao druida
    }
    if (id>=1 && id<=5) line(x,&y,N,T(S_B_CD_DESC),(int)barb_cooldown_time((BarbCooldown)(id-1)));
}
static const char *hunt_key(int i) {
    switch (i) { case 0: return "A"; case 1: return "B"; case 2: return T(S_K_CLEFT); case 3: return T(S_K_CUP);
                 case 4: return T(S_K_CRIGHT); case 5: return "L"; case 6: return "R"; default: return "Z"; }
}
static void draw_hunt_desc(int id, int x, int y) {
    int N = UI_STYLE_NORMAL, G = UI_STYLE_GOLD;
    switch (id) {
        case 0:
            line(x, &y, (g_mods.melee_pct + g_mods.all_pct) ? G : N, T(S_HA0_1), hunt_arrow_damage());
            line(x, &y, N, "%s", T(S_HA0_2));
            line(x, &y, N, "%s", T(S_HA0_3));
            if (g_mods.bolt_pierce) line(x, &y, G, "%s", T(S_A1_3));
            if (g_mods.crit > 0) line(x, &y, G, T(S_A0_5), pct(g_mods.crit));
            break;
        case 1:
            line(x, &y, g_mods.bolt_cost ? G : N, T(S_COST), (int)hunt_volley_cost());
            line(x, &y, g_mods.bolt_extra ? G : N, T(S_HA1_1), hunt_volley_count());
            break;
        case 2:
            line(x, &y, N, T(S_COST), (int)HUNT_ROOTS_COST);
            line(x, &y, g_mods.trap_extra ? G : N, T(S_HA2_1), 2 + g_mods.trap_extra);
            line(x, &y, (g_mods.root_time || g_mods.thorn_pct) ? G : N, T(S_HA2_2), (double)hunt_roots_time(), hunt_roots_damage());
            break;
        case 3:
            line(x, &y, N, T(S_COST), (int)HUNT_HAWK_COST);
            line(x, &y, (g_mods.wolf_extra || g_mods.wolf_life) ? G : N, T(S_HA3_1), hunt_hawk_count(), (int)hunt_hawk_life());
            line(x, &y, g_mods.wolf_pct ? G : N, T(S_HA3_2), hunt_hawk_damage());
            break;
        case 4:
            line(x, &y, N, T(S_COST), (int)HUNT_FOCUS_COST);
            line(x, &y, g_mods.beast_time ? G : N, T(S_HA4_1), (int)hunt_focus_time());
            line(x, &y, N, "%s", T(S_HA4_2));
            break;
        case 5:
            line(x, &y, g_mods.camo_cost ? G : N, T(S_COST), (int)hunt_camo_cost());
            line(x, &y, g_mods.camo_time ? G : N, T(S_HA5_1), (int)hunt_camo_time());
            line(x, &y, N, "%s", T(S_HA5_2));
            line(x, &y, N, "%s", T(S_HA5_3));
            break;
        case 6:
            line(x, &y, N, "%s", T(S_A5_1)); line(x, &y, N, "%s", T(S_A5_2)); line(x, &y, N, "%s", T(S_A5_3));
            break;
        default: draw_ability_desc(6, x, y); break;
    }
}

static void draw_abilities(void) {
    if (g_hero == HERO_BARBARIAN) {
        panel(16, 26, 304, 214);
        ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, T(S_ABILITIES));
        for (int i = 0; i < 7; i++)
            ui_print(false, 26, 70 + i * 17, i == s_sub ? UI_STYLE_SELECTED : UI_STYLE_NORMAL, 0, 0, T((StrId)(S_BAB0 + i)));
        char head[48];
        snprintf(head, sizeof(head), "%s  (%s)", T((StrId)(S_BAB0 + s_sub)), barb_key(s_sub));
        ui_print(false, 134, 70, UI_STYLE_GOLD, 0, 0, head);
        draw_barb_desc(s_sub, 134, 90);
        return;
    }
    if (g_hero == HERO_HUNTRESS) {
        panel(16, 26, 304, 214);
        ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, T(S_ABILITIES));
        for (int i = 0; i < 8; i++)
            ui_print(false, 26, 70 + i * 17, i == s_sub ? UI_STYLE_SELECTED : UI_STYLE_NORMAL, 0, 0, T((StrId)(S_HAB0 + i)));
        char head[48];
        snprintf(head, sizeof(head), "%s  (%s)", T((StrId)(S_HAB0 + s_sub)), hunt_key(s_sub));
        ui_print(false, 134, 70, UI_STYLE_GOLD, 0, 0, head);
        draw_hunt_desc(s_sub, 134, 90);
        return;
    }
    panel(16, 26, 304, 214);
    ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, T(S_ABILITIES));
    for (int i = 0; i < 7; i++)
        ui_print(false, 26, 70 + i * 17, i == s_sub ? UI_STYLE_SELECTED : UI_STYLE_NORMAL, 0, 0, T(ABIL_NAME(i)));
    char head[48];
    snprintf(head, sizeof(head), "%s  (%s)", T(ABIL_NAME(s_sub)), abil_key(s_sub));
    ui_print(false, 134, 70, UI_STYLE_GOLD, 0, 0, head);
    draw_ability_desc(s_sub, 134, 90);
}

// Arte da carta (se ja existir na ROM) dentro da moldura da raridade; senao, um cartao liso.
static sprite_t *s_card_art[CARD_COUNT];
static sprite_t *s_card_frame[RARITY_COUNT];
static bool s_card_tried[CARD_COUNT], s_frame_tried[RARITY_COUNT];

// Carta com escala (sc) e largura horizontal extra (sx, para o efeito de "virar"). x,y = canto.
static void draw_card_ex(int id, float x, float y, float sc, float sx) {
    const CardDef *cd = cards_def(id);
    if (!cd || sc <= 0.01f || sx <= 0.01f) return;
    if (!s_card_tried[id]) {
        char path[48]; snprintf(path, sizeof(path), "rom:/card_%02d.rgba16.sprite", id);
        if (rom_file(path)) s_card_art[id] = sprite_load(path);
        s_card_tried[id] = true;
    }
    if (!s_frame_tried[cd->rarity]) {
        static const char *fr[RARITY_COUNT] = { "common", "rare", "legendary", "cursed" };
        char path[48]; snprintf(path, sizeof(path), "rom:/card_frame_%s.rgba16.sprite", fr[cd->rarity]);
        if (rom_file(path)) s_card_frame[cd->rarity] = sprite_load(path);
        s_frame_tried[cd->rarity] = true;
    }
    float w = 72.0f * sc * sx, h = 96.0f * sc;
    float x0 = x + (72.0f * sc - w) * 0.5f;          // "virar": encolhe no eixo x pelo centro
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(cards_rarity_color(cd->rarity));
    rdpq_fill_rectangle(x0, y, x0 + w, y + h);
    rdpq_set_prim_color(RGBA32(16, 10, 10, 255));
    rdpq_fill_rectangle(x0 + 3 * sc * sx, y + 3 * sc, x0 + w - 3 * sc * sx, y + h - 3 * sc);
    if (s_card_art[id]) {
        rdpq_set_mode_standard();
        rdpq_mode_alphacompare(1);
        rdpq_sprite_blit(s_card_art[id], x0 + 4 * sc * sx, y + 4 * sc,
                         &(rdpq_blitparms_t){ .scale_x = sc * sx, .scale_y = sc });   // arte 64x80
    }
    if (s_card_frame[cd->rarity]) {
        rdpq_set_mode_standard();
        rdpq_mode_alphacompare(1);
        rdpq_sprite_blit(s_card_frame[cd->rarity], x0, y,
                         &(rdpq_blitparms_t){ .scale_x = sc * sx, .scale_y = sc });   // moldura 72x96
    }
}

static void draw_card(int id, int x, int y) { draw_card_ex(id, (float)x, (float)y, 1.0f, 1.0f); }

static void draw_cards(void) {
    panel(16, 26, 304, 214);
    { char hb[32]; snprintf(hb, sizeof(hb), T(S_CARDS_N), cards_owned_count(), CARD_SLOTS); ui_print(true, 26, 46, UI_STYLE_GOLD, 0, 0, hb); }
    // 4 espacos fixos (um por tipo); o cursor anda por eles
    if (s_sub >= CARD_SLOTS) s_sub = CARD_SLOTS - 1;
    for (int t = 0; t < CARD_SLOTS; t++) {
        int id = cards_slot_card(t);
        char buf[48];
        snprintf(buf, sizeof(buf), "%s:", T((StrId)((g_hero == HERO_HUNTRESS ? S_HTYPE0 : g_hero == HERO_BARBARIAN ? S_BTYPE0 : S_TYPE0) + t)));
        ui_print(false, 26, 72 + t * 30, t == s_sub ? UI_STYLE_SELECTED : UI_STYLE_DISABLED, 0, 0, buf);
        ui_print(false, 34, 86 + t * 30, id >= 0 ? (t == s_sub ? UI_STYLE_GOLD : UI_STYLE_NORMAL) : UI_STYLE_DISABLED, 0, 0,
                 id >= 0 ? cards_def(id)->name : T(S_SLOT_EMPTY));
    }
    int id = cards_slot_card(s_sub);
    if (id < 0) {
        ui_print(false, 150, 90, UI_STYLE_DISABLED, 0, 0, T(S_NO_CARDS2));
        return;
    }
    const CardDef *cd = cards_def(id);
    draw_card(id, 150, 56);
    cd = cards_def(id);
    ui_print(false, 230, 70, UI_STYLE_GOLD, 0, 0, cd->name);
    ui_print(false, 230, 86, cd->rarity == RARITY_CURSED ? UI_STYLE_SELECTED : UI_STYLE_DISABLED, 0, 0, cards_rarity_name(cd->rarity));
    ui_print(false, 150, 170, UI_STYLE_NORMAL, 0, 0, cd->desc);
    if (cd->desc2[0]) ui_print(false, 150, 185, UI_STYLE_NORMAL, 0, 0, cd->desc2);
}

void menu_pause_draw(void) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 150));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_mode_blender(0);
    if (s_page == PAGE_STATUS)    { draw_status();    ui_print(false, 22, 230, UI_STYLE_DISABLED, 0, 0, T(S_B_BACK)); return; }
    if (s_page == PAGE_ABILITIES) { draw_abilities(); ui_print(false, 22, 230, UI_STYLE_DISABLED, 0, 0, T(S_B_BACK)); return; }
    if (s_page == PAGE_CARDS)     { draw_cards();     ui_print(false, 22, 230, UI_STYLE_DISABLED, 0, 0, T(S_B_BACK)); return; }
    ui_print(true, 22, 62, UI_STYLE_GOLD, 0, 0, T(S_PAUSED));
    const char *items[6];
    for (int i = 0; i < 6; i++) items[i] = T(PAUSE_IDS[i]);
    draw_items(items, PAUSE_ENABLED, 6, 22, 92, 22);
    if (s_msg_t > 0.0f) ui_print(false, 22, 230, UI_STYLE_GOLD, 0, 0, T(s_msg_id));
}

void menu_notice(bool saved) { s_msg_id = saved ? S_SAVED : S_SAVE_FAIL; s_msg_t = 2.0f; }

// ---- Oferta de carta: o jogo pausa e o jogador escolhe pegar (A) ou deixar (B) ----
static uint32_t s_offer_since = 0;
static int s_offer_last = -1;

MenuAction menu_offer_update(const joypad_inputs_t *in, const joypad_buttons_t *p) {
    (void)in;
    int id = cards_offer_pending();
    if (id < 0) return MENU_BACK;
    uint32_t now = get_ticks_ms();
    if (id != s_offer_last) { s_offer_last = id; s_offer_since = now; sound_play(SFX_SHAPESHIFT); }
    if (now - s_offer_since < 800) return MENU_NONE;      // revelacao + evita escolher sem querer
    if (p->a) { cards_offer_resolve(true);  sound_play(SFX_PICKUP); s_offer_last = -1; return MENU_CONFIRM; }
    if (p->b) { cards_offer_resolve(false); sound_play(SFX_SWING);  s_offer_last = -1; return MENU_CONFIRM; }
    return MENU_NONE;
}

static const char *type_name(int t) { return T((StrId)((g_hero == HERO_HUNTRESS ? S_HTYPE0 : g_hero == HERO_BARBARIAN ? S_BTYPE0 : S_TYPE0) + t)); }

void menu_offer_draw(void) {
    int id = cards_offer_pending();
    const CardDef *cd = cards_def(id);
    if (!cd) return;
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 170));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_mode_blender(0);
    CardRarity r = cd->rarity;
    int type = cards_type(id);
    int occ = cards_offer_occupant();
    bool ready = get_ticks_ms() - s_offer_since >= 800;
    char buf[64];
    ui_print(true, 0, 30, r == RARITY_CURSED ? UI_STYLE_SELECTED : UI_STYLE_GOLD, 1, SCREEN_WIDTH,
             T(r == RARITY_CURSED ? S_CARD_CURSED_FOUND : S_CARD_FOUND));
    // ---- revelacao: brilho da raridade, raios girando, faiscas, carta virando e crescendo ----
    float t = (float)(get_ticks_ms() - s_offer_since) * 0.001f;      // segundos desde que apareceu
    color_t rc = cards_rarity_color(r);
    float cx = 30 + 36, cy = 46 + 48;                                 // centro da carta
    {
        float e = t < 0.5f ? t / 0.5f : 1.0f;
        float pulse = 0.75f + 0.25f * sinf(t * 5.0f);
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        for (int k = 0; k < 5; k++) {                                 // halo em camadas
            float rr = (20.0f + k * 12.0f) * e * pulse;
            rdpq_set_prim_color(RGBA32(rc.r, rc.g, rc.b, (int)(46 - k * 8)));
            rdpq_fill_rectangle(cx - rr * 0.9f - 30, cy - rr * 1.15f - 40, cx + rr * 0.9f + 30, cy + rr * 1.15f + 40);
        }
        for (int k = 0; k < 12; k++) {                                // raios de luz girando
            float a = t * 0.9f + k * 0.5236f, len = (60.0f + 18.0f * sinf(t * 3 + k)) * e;
            for (int q = 3; q < 10; q++) {
                float d = len * q / 10.0f, px = cx + cosf(a) * d, py = cy + sinf(a) * d * 0.9f;
                int sz = 3 - q / 4;
                rdpq_set_prim_color(RGBA32(rc.r, rc.g, rc.b, 130 - q * 11));
                rdpq_fill_rectangle(px - sz, py - sz, px + sz + 1, py + sz + 1);
            }
        }
        for (int k = 0; k < 16; k++) {                                // faiscas subindo
            float ph = fmodf(t * 0.7f + k * 0.137f, 1.0f);
            float px = cx + sinf(k * 7.1f) * 50.0f + sinf(t * 2 + k) * 4.0f, py = cy + 50.0f - ph * 110.0f;
            int al = (int)(220 * (1.0f - ph));
            rdpq_set_prim_color(RGBA32(255, 230 - (r == RARITY_CURSED ? 180 : 0), 160 - (r == RARITY_CURSED ? 130 : 0), al));
            rdpq_fill_rectangle(px, py, px + 2, py + 2);
        }
        if (t < 0.35f) {                                              // clarao inicial
            rdpq_set_prim_color(RGBA32(255, 245, 220, (int)(170 * (1.0f - t / 0.35f))));
            rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        rdpq_mode_blender(0);
    }
    // carta: cresce com um pequeno "salto" e vira (largura 0 -> 1) no primeiro meio segundo
    float g = t < 0.45f ? t / 0.45f : 1.0f;
    float sc = 0.3f + 0.7f * g + (t < 0.6f ? 0.12f * sinf(g * 3.14159f) : 0.0f);
    float flip = t < 0.4f ? fabsf(cosf((1.0f - t / 0.4f) * 3.14159f * 1.5f)) : 1.0f;
    draw_card_ex(id, cx - 36 * sc, cy - 48 * sc, sc, flip < 0.05f ? 0.05f : flip);
    cd = cards_def(id);
    if (t < 0.45f) return;                                            // textos entram depois da carta
    ui_print(true, 116, 64, UI_STYLE_GOLD, 0, 0, cd->name);
    snprintf(buf, sizeof(buf), "%s  -  %s", cards_rarity_name(r), type_name(type));
    ui_print(false, 116, 80, r == RARITY_CURSED ? UI_STYLE_SELECTED : UI_STYLE_DISABLED, 0, 0, buf);
    ui_print(false, 116, 100, UI_STYLE_NORMAL, 0, 0, cd->desc);
    if (cd->desc2[0]) ui_print(false, 116, 115, UI_STYLE_NORMAL, 0, 0, cd->desc2);
    if (occ >= 0) {                                       // o espaco desse tipo ja tem carta: mostra quem sai
        const CardDef *o = cards_def(occ);
        snprintf(buf, sizeof(buf), T(S_SLOT_REPLACES), type_name(type));
        ui_print(false, 30, 158, UI_STYLE_SELECTED, 0, 0, buf);
        ui_print(false, 30, 174, UI_STYLE_GOLD, 0, 0, o->name);
        ui_print(false, 30, 189, UI_STYLE_DISABLED, 0, 0, o->desc);
        if (o->desc2[0]) ui_print(false, 30, 203, UI_STYLE_DISABLED, 0, 0, o->desc2);
        ui_print(false, 30, 230, ready ? UI_STYLE_NORMAL : UI_STYLE_DISABLED, 0, 0, T(S_SWAP_KEYS));
    } else {
        ui_print(true, 52, 196, ready ? UI_STYLE_NORMAL : UI_STYLE_DISABLED, 0, 0, T(S_CARD_TAKE));
        ui_print(true, 170, 196, ready ? UI_STYLE_NORMAL : UI_STYLE_DISABLED, 0, 0, T(S_CARD_LEAVE));
    }
}

// ---- Mascate demoniaco: troca essencia por pocao, elixir, carta selada ou continue ----
#define SHOP_ITEMS 4
static const int SHOP_PRICE[SHOP_ITEMS] = { 60, 45, 200, 400 };   // continue ficou em 400 (aprovado); o resto subiu
// Limite de compras POR FASE: pocoes (cura + elixir juntos) 10, carta 1, continue 1.
static int s_shop_serial = -1, s_bought_potions = 0, s_bought_card = 0, s_bought_cont = 0;
static int s_shop_cursor = 0;
static float s_shop_msg_t = 0.0f;
static StrId s_shop_msg = S_SHOP_POOR;
static uint32_t s_shop_since = 0;

void menu_shop_open(void) {
    if (s_shop_serial != world_load_serial()) {      // fase nova (ou recomecada): zera os limites
        s_shop_serial = world_load_serial();
        s_bought_potions = s_bought_card = s_bought_cont = 0;
    }
    s_shop_cursor = 0; s_shop_msg_t = 0.0f;
    s_shop_since = get_ticks_ms();
    sound_play(SFX_CHEST);
}

// O mascate ja vendeu o maximo desta fase?
static bool shop_sold_out(int i) {
    return i <= 1 ? s_bought_potions >= 10 : i == 2 ? s_bought_card >= 1 : s_bought_cont >= 1;
}
static bool shop_full(int i) {
    if (shop_sold_out(i)) return true;
    switch (i) {
        case 0: return g_druid.health_potions >= 5;
        case 1: return g_druid.spirit_potions >= 5;
        case 3: return g_druid.continues >= 3;
        default: return cards_offer_pending() >= 0;
    }
}

MenuAction menu_shop_update(const joypad_inputs_t *in, const joypad_buttons_t *p) {
    if (s_shop_msg_t > 0.0f) s_shop_msg_t -= 1.0f / 60.0f;
    int d = nav_vertical(in, p);
    if (d) { s_shop_cursor = (s_shop_cursor + d + SHOP_ITEMS) % SHOP_ITEMS; sound_play(SFX_SWING); }
    if (p->b || p->start) { sound_play(SFX_SWING); return MENU_BACK; }
    if (get_ticks_ms() - s_shop_since < 300) return MENU_NONE;   // o A que abriu nao compra nada
    if (p->a) {
        int i = s_shop_cursor;
        if (shop_sold_out(i)) { s_shop_msg = S_SHOP_SOLD; s_shop_msg_t = 1.6f; sound_play(SFX_PLAYER_HURT); return MENU_NONE; }
        if (shop_full(i)) { s_shop_msg = S_SHOP_FULL; s_shop_msg_t = 1.6f; sound_play(SFX_PLAYER_HURT); return MENU_NONE; }
        if (g_druid.essence < SHOP_PRICE[i]) { s_shop_msg = S_SHOP_POOR; s_shop_msg_t = 1.6f; sound_play(SFX_PLAYER_HURT); return MENU_NONE; }
        g_druid.essence -= SHOP_PRICE[i];
        s_shop_msg = S_SHOP_BOUGHT; s_shop_msg_t = 1.6f;
        sound_play(SFX_PICKUP);
        switch (i) {
            case 0: g_druid.health_potions++; s_bought_potions++; break;
            case 1: g_druid.spirit_potions++; s_bought_potions++; break;
            case 2: s_bought_card++;
                    cards_grant_weighted(80, 12, 2, 6);   // carta selada: rara e dificil
                    return MENU_CONFIRM;                  // fecha a loja e mostra a oferta
            default: g_druid.continues++; s_bought_cont++; break;
        }
    }
    return MENU_NONE;
}

void menu_shop_draw(void) {
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 140));
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_mode_blender(0);
    panel(16, 30, SCREEN_WIDTH - 16, 216);
    ui_print(true, 0, 52, UI_STYLE_GOLD, 1, SCREEN_WIDTH, T(S_SHOP_TITLE));
    ui_print(false, 0, 70, UI_STYLE_DISABLED, 1, SCREEN_WIDTH, T(S_SHOP_GREET));

    // brilho vermelho atras do item escolhido
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    int pulse = 40 + (int)(20.0f * sinf((float)get_ticks_ms() * 0.006f));
    rdpq_set_prim_color(RGBA32(170, 20, 10, pulse));
    rdpq_fill_rectangle(24, 84 + s_shop_cursor * 20, SCREEN_WIDTH - 24, 100 + s_shop_cursor * 20);
    rdpq_mode_blender(0);

    for (int i = 0; i < SHOP_ITEMS; i++) {
        bool sel = i == s_shop_cursor;
        bool can = !shop_full(i) && g_druid.essence >= SHOP_PRICE[i];
        int style = sel ? UI_STYLE_SELECTED : can ? UI_STYLE_NORMAL : UI_STYLE_DISABLED;
        int y = 96 + i * 20;
        char buf[64];
        snprintf(buf, sizeof(buf), "%s%s", sel ? "\xC2\xBB " : "", T((StrId)(S_SHOP_I0 + i)));
        ui_print(false, sel ? 30 : 40, y, style, 0, 0, buf);
        snprintf(buf, sizeof(buf), "%d", SHOP_PRICE[i]);
        ui_print(false, SCREEN_WIDTH - 34 - 60, y, can ? UI_STYLE_GOLD : UI_STYLE_DISABLED, 2, 60, buf);
    }

    ui_print(false, 0, 182, UI_STYLE_NORMAL, 1, SCREEN_WIDTH, T((StrId)(S_SHOP_D0 + s_shop_cursor)));
    char have[48];
    snprintf(have, sizeof(have), T(S_SHOP_HAVE), g_druid.essence);
    ui_print(false, 30, 204, UI_STYLE_GOLD, 0, 0, have);
    if (s_shop_msg_t > 0.0f)
        ui_print(false, SCREEN_WIDTH - 30 - 150, 204, s_shop_msg == S_SHOP_BOUGHT ? UI_STYLE_GOLD : UI_STYLE_SELECTED, 2, 150, T(s_shop_msg));
    ui_print(false, 0, 232, UI_STYLE_DISABLED, 1, SCREEN_WIDTH, T(S_SHOP_KEYS));
}
