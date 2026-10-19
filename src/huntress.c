// Cacadora: heroina de longe. Usa a mesma estrutura do jogador (g_druid) e o "espirito" como energia.
#include "huntress.h"
#include "druid.h"
#include "cards.h"
#include "spells.h"
#include "audio.h"
#include "hud.h"
#include "lang.h"
#include "save.h"

#define SHOT_TIME     0.16f   // flecha sai no toque; animacao curta de retorno
#define VOLLEY_COST   18.0f

int hunt_arrow_damage(void) { return (int)((g_druid.attack_damage + 4) * (1.0f + g_mods.melee_pct + g_mods.all_pct)); }
int hunt_volley_count(void) { int n = 3 + g_mods.bolt_extra; return n > 7 ? 7 : n; }
float hunt_volley_cost(void) { float c = VOLLEY_COST + g_mods.bolt_cost; return c < 6.0f ? 6.0f : c; }
int hunt_roots_damage(void) { return (int)((20 + g_druid.level * 5) * (1.0f + g_mods.thorn_pct + g_mods.all_pct)); }
float hunt_roots_time(void) { return 3.0f + g_mods.root_time; }
int hunt_hawk_count(void) { int n = 1 + g_mods.wolf_extra; return n > 3 ? 3 : n; }
int hunt_hawk_damage(void) { return (int)((14 + g_druid.level * 4) * (1.0f + g_mods.wolf_pct + g_mods.all_pct)); }
float hunt_hawk_life(void) { return 16.0f + g_mods.wolf_life; }
float hunt_focus_time(void) { return 6.0f + g_mods.beast_time * 0.5f; }
float hunt_camo_time(void) { return 5.0f + g_mods.camo_time; }
float hunt_camo_cost(void) { float c = 30.0f + g_mods.camo_cost; return c < 10.0f ? 10.0f : c; }

static bool spend(float cost) {
    if (g_druid.spirit >= cost) { g_druid.spirit -= cost; return true; }
    char mb[48]; snprintf(mb, sizeof(mb), T(S_NO_ENERGY), (int)cost); hud_show_message(mb, 1.0f);
    return false;
}

// Dano de uma flecha com foco (critico garantido) e o primeiro tiro saindo da camuflagem (x3).
static int arrow_damage(float mul) {
    float pct = g_mods.melee_pct;
    int d = cards_roll_damage((int)((g_druid.attack_damage + 4) * mul), pct);
    if (g_druid.focus_timer > 0.0f) d = (int)(d * (1.8f + g_mods.beast_pct));
    if (g_druid.camo_timer > 0.0f) {          // ataque surpresa: sai do manto
        d *= 3;
        g_druid.camo_timer = 0.0f;
        hud_show_message(T(S_H_AMBUSH), 1.2f);
    }
    return d;
}

static void fire(float yaw, float mul, bool charged) {
    float jit = g_mods.aim_jitter > 0.0f ? ((float)(rand() % 200) / 100.0f - 1.0f) * g_mods.aim_jitter : 0.0f;
    float a = yaw + jit;
    float dx = cosf(a), dz = sinf(a);
    spells_cast_arrow(g_druid.x + dx * 8.0f, g_druid.z + dz * 8.0f, dx, dz, arrow_damage(mul),
                      charged || g_mods.bolt_pierce, charged ? 430.0f : 340.0f);
}

void hunt_buttons(joypad_inputs_t *in, joypad_buttons_t *pressed, float input_mag) {
    (void)in;
    bool busy = g_druid.state == DRUID_STATE_ATTACK || g_druid.state == DRUID_STATE_DODGE;
    // A: ataque basico imediato, sem parar a corrida.
    if (pressed->a && !busy) {
        g_druid.state = DRUID_STATE_ATTACK;
        if (input_mag > 0.15f) g_druid.yaw = g_druid.target_yaw; else g_druid.target_yaw = g_druid.yaw;
        g_druid.attack_duration = SHOT_TIME * (g_druid.focus_timer > 0.0f ? 0.7f : 1.0f);
        g_druid.attack_timer = g_druid.attack_duration;
        g_druid.attack_hit_done = true;
        g_druid.charging = false;
        g_druid.charge = 0.0f;
        fire(g_druid.yaw, 1.0f, false);
        sound_play(SFX_SWING);
        return;
    }
    // B: rajada em leque
    if (pressed->b && !busy) {
        if (spend(hunt_volley_cost())) {
            int n = hunt_volley_count();
            for (int k = 0; k < n; k++) {
                float a = g_druid.yaw + ((float)k - (float)(n - 1) * 0.5f) * 0.20f;
                fire(a, 0.75f + g_mods.bolt_pct, false);
            }
            sound_play(SFX_SWING);
            g_druid.state = DRUID_STATE_CAST;
            g_druid.cast_timer = 0.30f;
        }
    }
    // C-esq: armadilha de raizes um pouco a frente
    if (pressed->c_left && !busy) {
        if (spend(HUNT_ROOTS_COST)) {
            float d = 34.0f;
            spells_place_roots(g_druid.x + cosf(g_druid.yaw) * d, g_druid.z + sinf(g_druid.yaw) * d);
            g_druid.state = DRUID_STATE_CAST;
            g_druid.cast_timer = 0.30f;
            hud_show_message(T(S_H_ROOTS), 1.2f);
        }
    }
    // C-cima: gaviao
    if (pressed->c_up && !busy) {
        if (spend(HUNT_HAWK_COST)) {
            spells_cast_hawks(g_druid.x, g_druid.z);
            g_druid.state = DRUID_STATE_CAST;
            g_druid.cast_timer = 0.30f;
            hud_show_banner(T(S_H_HAWK_T), T(S_H_HAWK_S), 2.0f);
        }
    }
    // C-dir: Foco do Cacador
    if (pressed->c_right && !busy && g_druid.focus_timer <= 0.0f) {
        if (spend(HUNT_FOCUS_COST)) {
            g_druid.focus_timer = hunt_focus_time();
            sound_play(SFX_SHAPESHIFT);
            hud_show_banner(T(S_H_FOCUS_T), T(S_H_FOCUS_S), 2.0f);
        }
    }
    // L: Manto de Camuflagem
    if (pressed->l && !busy && g_druid.camo_timer <= 0.0f) {
        if (spend(hunt_camo_cost())) {
            g_druid.camo_timer = hunt_camo_time();
            sound_play(SFX_DODGE);
            hud_show_banner(T(S_H_CAMO_T), T(S_H_CAMO_S), 2.0f);
        }
    }
}

void hunt_attack_update(float dt, joypad_inputs_t *in, float input_mag) {
    (void)in; (void)input_mag;
    if (g_druid.state != DRUID_STATE_ATTACK) { g_druid.charging = false; return; }
    g_druid.attack_timer -= dt;
    if (g_druid.attack_timer <= 0.0f) { g_druid.state = DRUID_STATE_IDLE; g_druid.charge = 0.0f; }
}
