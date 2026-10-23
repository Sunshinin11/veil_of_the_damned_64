#include "audio.h"
#include "druid.h"
#include "enemies.h"
#include "world.h"
#include <wav64.h>
#include <mixer.h>

// WAV64 streams from ROM. Only the current music and active effects keep
// file handles, which is important on a base 4 MB N64.
#define AUDIO_RATE 22050
#define SFX_CHANNEL_FIRST 2
static float s_music_vol = 0.8f, s_sfx_vol = 0.8f;   // Opcoes: volume da musica e dos efeitos
#define SFX_CHANNEL_COUNT 5 // quatro efeitos + uma voz de grito protegida dos golpes
#define SPIN_CHANNEL (SFX_CHANNEL_FIRST + SFX_CHANNEL_COUNT)
#define SPIN_VOLUME 0.08f // cerca de11% do volume dos efeitos normais

typedef enum {
    MUSIC_TITLE, MUSIC_PATH, MUSIC_CRYPT, MUSIC_COMBAT,
    MUSIC_ALTAR, MUSIC_BOSS, MUSIC_VICTORY, MUSIC_DEFEAT,
    MUSIC_COUNT
} MusicTrack;

static const char *music_names[MUSIC_COUNT] = {
    "bgm_title", "bgm_path", "bgm_crypt", "bgm_combat",
    "bgm_altar", "bgm_boss", "bgm_victory", "bgm_defeat"
};

static const char *effect_names[SFX_COUNT] = {
    [SFX_SWING] = "sfx_druid_swing",
    [SFX_SPIRIT_BOLT_CAST] = "sfx_bolt_cast",
    [SFX_SPIRIT_BOLT_HIT] = "sfx_bolt_hit",
    [SFX_THORN_BURST] = "sfx_thorns",
    [SFX_WOLF_HOWL] = "sfx_wolf_howl",
    [SFX_WOLF_BITE] = "sfx_wolf_bite",
    [SFX_SHAPESHIFT] = "sfx_shapeshift",
    [SFX_POTION] = "sfx_potion",
    [SFX_PLAYER_HURT] = "sfx_player_hurt",
    [SFX_ENEMY_HURT] = "sfx_skeleton_hurt",
    [SFX_ENEMY_DIE] = "sfx_skeleton_die",
    [SFX_BOSS_SLAM] = "sfx_boss_slam",
    [SFX_BOSS_ENRAGE] = "sfx_boss_enrage",
    [SFX_VICTORY] = "sfx_gate",
    [SFX_BEAST_SWING] = "sfx_beast_swing",
    [SFX_SKELETON_SWING] = "sfx_skeleton_swing",
    [SFX_DEMON_SWING] = "sfx_demon_swing",
    [SFX_CULTIST_CAST] = "sfx_cultist_cast",
    [SFX_DARK_ORB_CAST] = "sfx_cultist_cast",
    [SFX_WOLF_SUMMON] = "sfx_wolf_summon",
    [SFX_DODGE] = "sfx_dodge",
    [SFX_SKELETON_HURT] = "sfx_skeleton_hurt",
    [SFX_DEMON_HURT] = "sfx_demon_hurt",
    [SFX_CULTIST_HURT] = "sfx_cultist_hurt",
    [SFX_BOSS_HURT] = "sfx_boss_hurt",
    [SFX_SKELETON_DIE] = "sfx_skeleton_die",
    [SFX_DEMON_DIE] = "sfx_demon_die",
    [SFX_CULTIST_DIE] = "sfx_cultist_die",
    [SFX_BOSS_DIE] = "sfx_boss_die",
    [SFX_CHEST] = "sfx_chest",
    [SFX_PICKUP] = "sfx_pickup",
    [SFX_UI_SELECT] = "sfx_ui_select",
    [SFX_UI_UNAVAILABLE] = "sfx_ui_unavailable",
    [SFX_BARB_SHOUT] = "sfx_barb_shout",
    [SFX_BARB_BERSERK] = "sfx_barb_berserk",
    [SFX_BARB_AXE] = "sfx_barb_axe",
};

static bool s_ready;
static GameState s_state;
static MusicTrack s_current_music;
static wav64_t s_music;
static bool s_music_open;
static wav64_t s_effects[SFX_CHANNEL_COUNT];
static bool s_effect_open[SFX_CHANNEL_COUNT];
static int s_next_effect;
static int s_combat_hold;
static wav64_t s_spin;
static bool s_spin_open;
static float s_spin_gain;

static void spin_audio_update(void) {
    bool active=s_state==GAME_STATE_PLAYING && g_hero==HERO_BARBARIAN && g_druid.hp>0 && g_druid.spin_timer>0.0f;
    if (active && !s_spin_open) {
        wav64_open(&s_spin,"rom:/sfx_barb_whirlwind.wav64");
        wav64_set_loop(&s_spin,true);
        wav64_play(&s_spin,SPIN_CHANNEL);
        s_spin_open=true; s_spin_gain=0.0f;
    }
    if (!s_spin_open) return;
    float target=active?SPIN_VOLUME:0.0f;
    s_spin_gain+=clampf(target-s_spin_gain,-0.025f,0.025f);
    mixer_ch_set_vol(SPIN_CHANNEL,s_spin_gain*s_sfx_vol,s_spin_gain*s_sfx_vol);
    if (!active && s_spin_gain<=0.0f) {
        mixer_ch_stop(SPIN_CHANNEL);wav64_close(&s_spin);s_spin_open=false;
    }
}

static void music_start(MusicTrack track) {
    if (s_music_open && s_current_music == track) return;
    if (s_music_open) {
        mixer_ch_stop(0);
        wav64_close(&s_music);
        s_music_open = false;
    }
    char path[80];
    snprintf(path, sizeof(path), "rom:/%s.wav64", music_names[track]);
    wav64_open(&s_music, path);
    wav64_set_loop(&s_music, track != MUSIC_VICTORY && track != MUSIC_DEFEAT);
    wav64_play(&s_music, 0);
    mixer_ch_set_vol(0, .42f * s_music_vol, .42f * s_music_vol);
    s_music_open = true;
    s_current_music = track;
}

void sound_init(void) {
    audio_init(AUDIO_RATE, AUDIO_DEFAULT_LATENCY);
    mixer_init(SPIN_CHANNEL + 1);
    mixer_set_vol(.8f);
    s_ready = true;
    s_state = GAME_STATE_INTRO;
    s_music_open = false;
    s_next_effect = 0;
    s_combat_hold = 0;
    s_spin_open=false; s_spin_gain=0.0f;
    memset(s_effect_open, 0, sizeof(s_effect_open));
    debugf("[AUDIO] WAV64 streaming mixer ready at %d Hz.\n", AUDIO_RATE);
}

void sound_play(SoundEffect sfx) {
    if (!s_ready || sfx < 0 || sfx >= SFX_COUNT || !effect_names[sfx]) return;
    bool voice=sfx==SFX_BARB_SHOUT || sfx==SFX_BARB_BERSERK;
    int slot = voice ? SFX_CHANNEL_COUNT-1 : s_next_effect++ % (SFX_CHANNEL_COUNT-1);
    int ch = SFX_CHANNEL_FIRST + slot;
    if (s_effect_open[slot]) {
        mixer_ch_stop(ch);
        wav64_close(&s_effects[slot]);
        s_effect_open[slot] = false;
    }
    char path[80];
    snprintf(path, sizeof(path), "rom:/%s.wav64", effect_names[sfx]);
    wav64_open(&s_effects[slot], path);
    wav64_play(&s_effects[slot], ch);
    mixer_ch_set_vol(ch, .75f * s_sfx_vol, .75f * s_sfx_vol);
    s_effect_open[slot] = true;
}

void sound_set_volumes(float music, float sfx) { s_music_vol = music; s_sfx_vol = sfx; }

void sound_set_context(GameState state) { s_state = state; }

// Trilha por campanha: [fase 1, fase 2, fase 3, combate, chefe]. So 8 musicas no jogo,
// entao a Floresta Morta embaralha as mesmas (a 01 do titulo vira musica de fase).
static const MusicTrack s_camp_music[3][5] = {
    { MUSIC_PATH,  MUSIC_CRYPT, MUSIC_ALTAR, MUSIC_COMBAT, MUSIC_BOSS },     // Condenados
    { MUSIC_CRYPT, MUSIC_TITLE, MUSIC_PATH,  MUSIC_ALTAR,  MUSIC_COMBAT },   // Floresta Morta
    { MUSIC_TITLE, MUSIC_PATH,  MUSIC_CRYPT, MUSIC_BOSS,   MUSIC_ALTAR },    // Norte do Barbaro
};

static MusicTrack desired_music(void) {
    const MusicTrack *cm = s_camp_music[(g_campaign >= 0 && g_campaign < 3) ? g_campaign : 0];
    if (s_state == GAME_STATE_INTRO || s_state == GAME_STATE_CHAR_SELECT) return MUSIC_TITLE;
    if (s_state == GAME_STATE_PLAYER_DEAD) return MUSIC_DEFEAT;
    if (s_state == GAME_STATE_VICTORY) return MUSIC_VICTORY;
    if (g_current_area == AREA_BLACK_ALTAR) {
        Enemy *boss = enemies_get_boss();
        if (boss && boss->hp > 0 &&
            vec2_dist_sq(boss->x, boss->z, g_druid.x, g_druid.z) < 150.0f * 150.0f)
            return cm[4];
        return cm[2];
    }
    Enemy *enemies = enemies_get_array();
    bool threatened = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (e->active && e->hp > 0 &&
            (e->state == ENEMY_STATE_CHASE || e->state == ENEMY_STATE_TELEGRAPH ||
             e->state == ENEMY_STATE_ATTACK) &&
            vec2_dist_sq(e->x, e->z, g_druid.x, g_druid.z) < 95.0f * 95.0f) {
            threatened = true;
            break;
        }
    }
    if (threatened) s_combat_hold = 120;
    else if (s_combat_hold > 0) s_combat_hold--;
    if (s_combat_hold > 0) return cm[3];
    return g_current_area == AREA_HOLLOW_CRYPT ? cm[1] : cm[0];
}

void sound_pump(void) {
    if (s_ready) mixer_try_play();
}

void sound_update(void) {
    if (!s_ready) return;
    mixer_ch_set_vol(0, .42f * s_music_vol, .42f * s_music_vol);   // volume da musica (opcoes)
    music_start(desired_music());
    spin_audio_update();
    for (int i = 0; i < SFX_CHANNEL_COUNT; i++) {
        if (s_effect_open[i] && !mixer_ch_playing(SFX_CHANNEL_FIRST + i)) {
            wav64_close(&s_effects[i]);
            s_effect_open[i] = false;
        }
    }
    sound_pump();
}

void sound_close(void) {
    if (!s_ready) return;
    if (s_spin_open) {mixer_ch_stop(SPIN_CHANNEL);wav64_close(&s_spin);s_spin_open=false;}
    for (int i = 0; i < SFX_CHANNEL_COUNT; i++) {
        if (s_effect_open[i]) {
            mixer_ch_stop(SFX_CHANNEL_FIRST + i);
            wav64_close(&s_effects[i]);
        }
    }
    if (s_music_open) {
        mixer_ch_stop(0);
        wav64_close(&s_music);
    }
    mixer_close();
    audio_close();
    s_ready = false;
}
