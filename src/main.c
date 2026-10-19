#include "common.h"
#include "gfx.h"
#include "models.h"
#include "models_gen.h"
#include "audio.h"
#include "druid.h"
#include "spells.h"
#include "enemies.h"
#include "world.h"
#include "items.h"
#include "hud.h"
#include "sprites.h"
#include "menu.h"
#include "lang.h"
#include "save.h"
#include "cards.h"

static GameState s_game_state = GAME_STATE_INTRO;
static int s_start_area = 0;          // seletor de fase na tela inicial (teste)
static float s_state_timer = 0.0f;
static float s_fade_alpha = 0.0f;
static AreaId s_target_area = AREA_FORSAKEN_PATH;
static bool s_good_ending = false;
static bool s_area_loaded = false;

// Performance / log
static float s_perf_timer = 0.0f;

// Camera
static T3DViewport s_viewport;
static T3DVec3 s_cam_pos = {{ 0, 160.0f, -120.0f }};
static T3DVec3 s_cam_target = {{ 0, 10.0f, 0 }};
static const T3DVec3 s_cam_up = {{ 0, 1.0f, 0 }};

static void setup_camera(float dt) {
    // Camera isometrica elevada tipo ARPG, seguindo o druida (mais proxima).
    float target_x = g_druid.x;
    float target_y = g_druid.y + CAM_TARGET_Y;
    float target_z = g_druid.z;

    float desired_cam_x = target_x;
    float desired_cam_y = target_y + CAM_HEIGHT;
    float desired_cam_z = target_z - CAM_BACK;

    float follow_speed = 10.0f;
    float t = clampf(follow_speed * dt, 0.0f, 1.0f);
    s_cam_target.v[0] = lerpf(s_cam_target.v[0], target_x, t);
    s_cam_target.v[1] = lerpf(s_cam_target.v[1], target_y, t);
    s_cam_target.v[2] = lerpf(s_cam_target.v[2], target_z, t);

    s_cam_pos.v[0] = lerpf(s_cam_pos.v[0], desired_cam_x, t);
    s_cam_pos.v[1] = lerpf(s_cam_pos.v[1], desired_cam_y, t);
    s_cam_pos.v[2] = lerpf(s_cam_pos.v[2], desired_cam_z, t);

    t3d_viewport_look_at(&s_viewport, &s_cam_pos, &s_cam_target, &s_cam_up);
}

static void apply_area_lighting_and_fog(void) {
    AreaData *ad = &g_area_data[g_current_area];

    t3d_light_set_ambient(ad->ambient_light);
    t3d_light_set_directional(0, ad->dir_light_color, &ad->dir_light_vec);
    t3d_light_set_count(1);

    if (g_dbg_fog) {
        // Fog precisa dos comandos do lado rdpq TAMBEM (so t3d_fog_* nao basta).
        rdpq_set_fog_color(ad->fog_color);
        rdpq_mode_fog(RDPQ_FOG_STANDARD);
        t3d_fog_set_range(ad->fog_near + CAM_FOG_OFFSET, ad->fog_far + CAM_FOG_OFFSET);
        t3d_fog_set_enabled(true);
    } else {
        t3d_fog_set_enabled(false);
    }
}

static void draw_scene_3d(void) {
    t3d_viewport_attach(&s_viewport);

    // Combiner SHADE: sem isto o combiner padrao ignora a cor dos vertices
    // e toda a geometria sai preta/invisivel.
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);

    AreaData *ad = &g_area_data[g_current_area];
    t3d_screen_clear_color(ad->clear_color);
    t3d_screen_clear_depth();

    apply_area_lighting_and_fog();

    // Culling selecionavel pelo D-pad (padrao: nenhum).
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());

    world_draw();
    items_draw();
    enemies_draw();
    spells_draw();
    druid_draw();

    t3d_tri_sync();
}

int main(void) {
    // 1. Hardware Initialization
    debug_init_isviewer();
    debug_init_usblog();
    debugf("\n========================================\n");
    debugf("   VEIL OF THE DAMNED (NINTENDO 64)\n");
    debugf("   Dark Fantasy Gothic Action RPG\n");
    debugf("========================================\n\n");

    joypad_init();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    asset_init_compression(2);
    dfs_init(DFS_DEFAULT_LOCATION);
    t3d_init((T3DInitParams){ .matrixStackSize = 8 });

    // Fonte embutida para o texto (rdpq_text).
    rdpq_text_register_font(FONT_BUILTIN_DEBUG_MONO, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO));
    ui_fonts_init();   // fontes goticas (Pirata One)

    // 2. Viewport
    s_viewport = t3d_viewport_create_buffered(3);
    t3d_viewport_set_perspective(&s_viewport, CAM_FOV, 320.0f / 240.0f, CAM_NEAR, CAM_FAR);

    // 3. Subsistemas
    gfx_init();
    sound_init();
    models_init();
    models_gen_init();
    druid_init();
    sprites_init();
    if (!sprites_druid_ready()) druid_model_init();
    spells_init();
    enemies_init();
    items_init();
    hud_init();
    world_init();
    menu_init();
    save_init();       // EEPROM: opcoes salvas (idioma, volume...) e o save da partida

    if (is_memory_expanded()) {
        debugf("[GAME] Expansion Pak detectado (8 MB).\n");
    } else {
        debugf("[GAME] Memoria base (4 MB).\n");
    }
    debugf("[GAME] Subsistemas iniciados. Entrando no loop.\n");

    s_game_state = GAME_STATE_INTRO;
    s_state_timer = 0.0f;

    uint64_t last_time = get_ticks();

    // 4. Game Loop
    for (;;) {
        uint64_t current_time = get_ticks();
        float dt = (float)(current_time - last_time) / (float)TICKS_PER_SECOND;
        last_time = current_time;
        dt = clampf(dt, 0.005f, 0.05f); // guarda de simulacao

        // Log de desempenho (FPS REAL do display, nao do dt limitado).
        s_perf_timer += dt;
        if (s_perf_timer >= 1.0f) {
            s_perf_timer = 0.0f;
            debugf("[PERF] Area %d | FPS %.1f | HP %d | Inimigos %d\n",
                   (int)g_current_area, display_get_fps(), g_druid.hp, enemies_get_active_count());
        }

        // Input
        joypad_poll();
        joypad_inputs_t in = joypad_get_inputs(JOYPAD_PORT_1);
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        // D-pad esquerda: minimapa (os atalhos de depuracao de render foram removidos na versao final)
        if (s_game_state == GAME_STATE_PLAYING) {
            if (pressed.d_left)  world_minimap_toggle();   // Z ja e a pocao
        }

        sound_set_context(s_game_state);
        sound_update();

        // ---- Logica por estado ----
        switch (s_game_state) {
            case GAME_STATE_INTRO: {          // tela inicial: Novo Jogo / Carregar / Opcoes
                MenuAction act = menu_title_update(&in, &pressed, &s_start_area);
                if (act == MENU_NEW_GAME) {
                    s_game_state = GAME_STATE_CHAR_SELECT;
                } else if (act == MENU_LOAD) {          // carregar: recomeca no inicio da fase salva
                    int area = 0;
                    if (save_load_game(&area)) {
                        s_start_area = area;
                        world_load_area((AreaId)area);
                        setup_camera(0.1f);
                        s_game_state = GAME_STATE_PLAYING;
                    }
                }
                break;
            }

            case GAME_STATE_CHAR_SELECT: {
                MenuAction act = menu_char_update(&in, &pressed, &s_start_area);
                if (act == MENU_BACK) {
                    menu_reset_cursor();
                    s_game_state = GAME_STATE_INTRO;
                } else if (act == MENU_CONFIRM) {
                    g_hero = menu_selected_hero();
                    g_campaign = menu_selected_campaign();
                    g_difficulty = menu_selected_difficulty();
                    g_seeds = 0;
                    int stage = (s_start_area / AREA_COUNT == g_campaign) ? s_start_area % AREA_COUNT : 0;
                    druid_init();
                    world_load_area((AreaId)stage);
                    setup_camera(0.1f);
                    s_game_state = GAME_STATE_PLAYING;
                    
                }
                break;
            }

            case GAME_STATE_CARD_OFFER: {         // carta encontrada: escolher pegar ou deixar
                MenuAction act = menu_offer_update(&in, &pressed);
                if (act == MENU_BACK || (act == MENU_CONFIRM && cards_offer_pending() < 0))
                    s_game_state = GAME_STATE_PLAYING;
                break;
            }

            case GAME_STATE_SHOP: {              // mascate: comprar com essencia
                MenuAction act = menu_shop_update(&in, &pressed);
                if (act == MENU_BACK || act == MENU_CONFIRM) s_game_state = GAME_STATE_PLAYING;
                break;
            }

            case GAME_STATE_PAUSED: {
                MenuAction act = menu_pause_update(&in, &pressed);
                if (act == MENU_RESUME) {
                    s_game_state = GAME_STATE_PLAYING;
                } else if (act == MENU_SAVE) {
                    menu_notice(save_write_game((int)g_current_area));
                } else if (act == MENU_QUIT) {
                    menu_reset_cursor();
                    s_game_state = GAME_STATE_INTRO;
                }
                break;
            }

            case GAME_STATE_PLAYING: {
                if (pressed.start) {               // START = pausa
                    menu_reset_cursor();
                    s_game_state = GAME_STATE_PAUSED;
                    break;
                }
                if (cards_offer_pending() >= 0) {  // carta de bau/chefe: pausa para escolher
                    s_game_state = GAME_STATE_CARD_OFFER;
                    break;
                }
                // Opcoes > Controles: invertido troca A (golpe) e B (raio)
                joypad_buttons_t act_btn = pressed;
                if (g_opt.controls) { act_btn.a = pressed.b; act_btn.b = pressed.a; }
                if (act_btn.a && world_near_vendor() && g_druid.hp > 0) {   // perto do mascate: A negocia
                    menu_shop_open();
                    s_game_state = GAME_STATE_SHOP;
                    break;
                }
                druid_update(dt, &in, &act_btn);
                spells_update(dt);
                enemies_update(dt);
                items_update(dt);
                world_update(dt);
                hud_update(dt);

                if (g_druid.hp <= 0 && g_druid.state == DRUID_STATE_DEAD) {
                    s_game_state = GAME_STATE_PLAYER_DEAD;
                    s_state_timer = 0.0f;
                }

                if (world_check_exit(g_druid.x, g_druid.z)) {
                    if (g_current_area < AREA_COUNT - 1) {
                        s_target_area = g_current_area + 1;
                        s_game_state = GAME_STATE_AREA_TRANSITION;
                        s_state_timer = 0.0f;
                        sound_play(SFX_VICTORY);
                    }
                }

                // Altar: so ha vitoria depois que os 3 selos invocaram Malakor e ele caiu.
                if (g_current_area == AREA_BLACK_ALTAR && world_boss_summoned()) {
                    static float boss_dead_t = 0.0f;
                    Enemy *boss = enemies_get_boss();
                    if (!boss || boss->hp <= 0) boss_dead_t += dt; else boss_dead_t = 0.0f;
                    if (boss_dead_t > 3.0f) {
                        boss_dead_t = 0.0f;
                        // Final bom da Floresta: as 3 sementes purificam Sylvara e libertam o ultimo aprendiz
                        s_good_ending = g_campaign == 1 && g_difficulty >= DIFF_NIGHTMARE && g_seeds >= 3;
                        if (s_good_ending) progress_unlock_hero(HERO_DRUID);
                        progress_beat(g_campaign, g_difficulty);
                        s_game_state = GAME_STATE_VICTORY;
                        s_state_timer = 0.0f;
                    }
                }
                break;
            }

            case GAME_STATE_AREA_TRANSITION: {
                s_state_timer += dt;
                if (s_state_timer < 0.5f) {
                    s_fade_alpha = s_state_timer / 0.5f;
                } else if (!s_area_loaded) {
                    s_area_loaded = true;
                    world_load_area(s_target_area);
                    setup_camera(0.1f);
                } else if (s_state_timer < 1.0f) {
                    s_fade_alpha = 1.0f - (s_state_timer - 0.5f) / 0.5f;
                } else {
                    s_fade_alpha = 0.0f;
                    s_area_loaded = false;
                    s_game_state = GAME_STATE_PLAYING;
                }
                break;
            }

            case GAME_STATE_PLAYER_DEAD: {
                s_state_timer += dt;
                if (s_state_timer > 1.5f && (pressed.start || pressed.a)) {
                    if (g_druid.continues > 0) {          // usa um continue: volta no mesmo lugar
                        druid_revive();
                        s_game_state = GAME_STATE_PLAYING;
                        hud_show_banner(T(S_CONT_T), T(S_CONT_S), 2.0f);
                    } else {                              // sem continues: fim de jogo
                        menu_reset_cursor();
                        s_game_state = GAME_STATE_INTRO;
                    }
                }
                break;
            }

            case GAME_STATE_VICTORY: {
                s_state_timer += dt;
                if (s_state_timer > 3.0f && (pressed.start || pressed.a)) {
                    menu_reset_cursor();
                    s_game_state = GAME_STATE_INTRO;   // volta para a tela inicial
                }
                break;
            }
        }

        // Camera segue sempre (mostra a cena atras das telas).
        setup_camera(dt);

        // ---- Render ----
        rdpq_attach(display_get(), display_get_zbuf());
        t3d_frame_start();

        bool in_menu = (s_game_state == GAME_STATE_INTRO || s_game_state == GAME_STATE_CHAR_SELECT);
        if (!in_menu) {
            draw_scene_3d();
            // Projected 2D entities share the same isometric camera as the 3D world.
            sprites_draw_world(&s_viewport);
            world_draw_screen_filter();   // Altar Negro: filtro vermelho de perigo
        }

        // HUD sprites and dynamic values + fade
        if (s_game_state == GAME_STATE_PLAYING ||
            s_game_state == GAME_STATE_AREA_TRANSITION ||
            s_game_state == GAME_STATE_PLAYER_DEAD ||
            s_game_state == GAME_STATE_PAUSED ||
            s_game_state == GAME_STATE_CARD_OFFER ||
            s_game_state == GAME_STATE_SHOP) {
            hud_draw();
        }

        if (s_game_state == GAME_STATE_AREA_TRANSITION && s_fade_alpha > 0.01f) {
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            uint8_t a = (uint8_t)(s_fade_alpha * 255.0f);
            rdpq_set_prim_color(RGBA32(0, 0, 0, a));
            rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }

        // ---- Texto (rdpq_text), antes do detach_show ----
        if (s_game_state == GAME_STATE_INTRO) {
            menu_title_draw(s_start_area);
        } else if (s_game_state == GAME_STATE_CHAR_SELECT) {
            menu_char_draw(s_start_area);
        } else if (s_game_state == GAME_STATE_PAUSED) {
            hud_draw_text();
            menu_pause_draw();
        } else if (s_game_state == GAME_STATE_CARD_OFFER) {
            hud_draw_text();
            menu_offer_draw();
        } else if (s_game_state == GAME_STATE_SHOP) {
            menu_shop_draw();
        } else if (s_game_state == GAME_STATE_PLAYING ||
                   s_game_state == GAME_STATE_AREA_TRANSITION) {
            hud_draw_text();
        } else if (s_game_state == GAME_STATE_PLAYER_DEAD) {
            hud_draw_text();
            ui_print(true, 0, 96, UI_STYLE_SELECTED, 1, SCREEN_WIDTH, T(S_YOU_DIED));
            if (g_druid.continues > 0) {
                char cbuf[48];
                snprintf(cbuf, sizeof(cbuf), T(S_CONT_LEFT), g_druid.continues);
                ui_text_center(124, cbuf);
                if (s_state_timer > 1.5f) ui_text_center(150, T(S_PRESS_CONT));
            } else {
                ui_text_center(124, T(S_NO_CONT));
                if (s_state_timer > 1.5f) ui_text_center(150, T(S_PRESS_OVER));
            }
        } else if (s_game_state == GAME_STATE_VICTORY) {
            ui_print(true, 0, 52, UI_STYLE_GOLD, 1, SCREEN_WIDTH, T(S_WIN1));
            if (s_good_ending) {
                ui_text_center(84, T(S_WIN2G));
                ui_text_center(104, T(S_WIN3G));
                ui_text_center(124, T(S_WIN4G));
            } else {
                ui_text_center(84, T(g_campaign == 2 ? S_WIN2N : g_campaign == 1 ? S_WIN2F : S_WIN2));
                ui_text_center(104, T(g_campaign == 2 ? S_WIN3N : g_campaign == 1 ? S_WIN3F : S_WIN3));
                ui_text_center(124, T(g_campaign == 2 ? S_WIN4N : g_campaign == 1 ? S_WIN4F : S_WIN4));
            }
            char lvl_buf[48];
            snprintf(lvl_buf, sizeof(lvl_buf), T(S_WIN5), g_druid.level);
            ui_text_center(156, lvl_buf);
            ui_text_center(212, T(S_PRESS_START));
        }

        // Linha de debug opcional (D-pad cima)
        if (g_dbg_hud) {
            const char *cull_name = (g_dbg_cull == 0) ? "NENHUM" : (g_dbg_cull == 1) ? "BACK" : "FRONT";
            ui_textf(6, 30, "FPS:%.1f CULL:%s FOG:%d", display_get_fps(), cull_name, (int)g_dbg_fog);
        }

        rdpq_detach_show();
        sound_pump();
    }

    t3d_viewport_destroy(&s_viewport);
    t3d_destroy();
    sound_close();
    rdpq_close();
    display_close();
    return 0;
}
