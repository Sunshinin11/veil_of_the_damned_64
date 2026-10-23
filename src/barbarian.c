// Barbaro: heroi corpo a corpo da campanha do Norte. Usa a estrutura do jogador (g_druid);
// o "espirito" e a FURIA (comeca vazia, enche batendo e apanhando).
#include "barbarian.h"
#include "barbarian_attack.h"
#include "barbarian_skills.h"
#include "druid.h"
#include "cards.h"
#include "enemies.h"
#include "audio.h"
#include "hud.h"
#include "lang.h"
#include "world.h"

#define FURY_PER_HIT 9.0f

static float buff(void) {     // grito e berserker somam dano
    return (g_druid.shout_timer > 0.0f ? 0.25f : 0.0f) + (g_druid.berserk_timer > 0.0f ? BARB_BERSERK_DAMAGE + g_mods.beast_pct : 0.0f);
}
int barb_axe_damage(int step) {
    float base = (float)(g_druid.attack_damage + 6);
    if (step >= 2) base *= 1.8f + g_mods.heavy_pct;
    return (int)(base * (1.0f + g_mods.melee_pct + g_mods.all_pct + buff()));
}
int barb_leap_damage(void) { return (int)((g_druid.attack_damage * 1.6f + 10) * (1.0f + g_mods.leap_pct + g_mods.all_pct + buff())); }
float barb_leap_cost(void) { float c = 25.0f + g_mods.bolt_cost; return c < 8.0f ? 8.0f : c; }
float barb_shout_radius(void) { return 85.0f * (1.0f + g_mods.thorn_radius_pct); }
float barb_shout_stun(void) { return 1.8f + g_mods.root_time; }
int barb_spin_damage(void) { return (int)((g_druid.attack_damage * 0.7f + 4) * (1.0f + g_mods.wolf_pct + g_mods.all_pct + buff())); }
float barb_spin_time(void) { return 2.5f + g_mods.wolf_life; }
float barb_berserk_time(void) { return 12.0f + g_mods.beast_time; }
float barb_speed_mult(void) {
    float m = g_druid.rush_timer > 0.0f ? BARB_RUSH_MULT : 1.0f;
    if (g_druid.spin_timer > 0.0f) m *= 0.7f;      // girando anda mais devagar
    return m;
}

static const float cooldown_times[BARB_CD_COUNT] = {6.0f, 10.0f, 8.0f, 60.0f, 12.0f};
_Static_assert(sizeof(g_druid.barb_cooldowns)/sizeof(float)==BARB_CD_COUNT, "barbarian cooldown storage");
float barb_cooldown(BarbCooldown skill) { return skill>=0 && skill<BARB_CD_COUNT ? g_druid.barb_cooldowns[skill] : 0.0f; }
float barb_cooldown_time(BarbCooldown skill) { return skill>=0 && skill<BARB_CD_COUNT ? cooldown_times[skill] : 0.0f; }
static bool available(BarbCooldown skill) {
    float remaining=barb_cooldown(skill);
    if (remaining<=0.0f) return true;
    char msg[64]; snprintf(msg,sizeof(msg),T(S_B_COOLDOWN),(double)remaining);
    hud_show_message(msg,0.8f);
    return false;
}
static void recharge(BarbCooldown skill) { g_druid.barb_cooldowns[skill]=cooldown_times[skill]; }
void barb_cancel_actions(void) {
    g_druid.warcry_kind=0; g_druid.warcry_fired=false;
    g_druid.leap_timer=g_druid.spin_timer=0.0f;
    g_druid.spin_clock=0.0f; g_druid.leap_hit_done=false;
    g_druid.rush_timer=g_druid.berserk_timer=g_druid.shout_timer=0.0f;
    g_druid.combo_queued=0; g_druid.combo=0; g_druid.combo_reset=0.0f;
    g_druid.fx_shout=g_druid.fx_impact=0.0f;
}

static void start_warcry(unsigned char kind, float input_mag) {
    if (input_mag>0.15f) g_druid.yaw=g_druid.target_yaw;
    g_druid.target_yaw=g_druid.yaw;
    g_druid.vx=g_druid.vz=0.0f;
    g_druid.combo_queued=0;
    g_druid.warcry_kind=kind; g_druid.warcry_fired=false;
    g_druid.state=DRUID_STATE_CAST; g_druid.cast_timer=BARB_WARCRY_TIME;
    g_druid.fx_shout=0.0f; // a aura/urso ja esta nos quadros; sem circulo nos pes
}

static void release_warcry(void) {
    g_druid.warcry_fired=true;
    if (g_druid.warcry_kind==1) {
        float r=barb_shout_radius();
        enemies_root_at(g_druid.x,g_druid.z,r,barb_shout_stun());
        if (g_mods.shout_dmg) enemies_damage_at(g_druid.x,g_druid.z,r,barb_axe_damage(0),0.0f,0.0f);
        if (g_mods.shout_heal>0.0f) druid_heal((int)(g_druid.max_hp*g_mods.shout_heal));
        g_druid.shout_timer=6.0f;
        sound_play(SFX_BARB_SHOUT);
        hud_show_banner(T(S_B_SHOUT_T),T(S_B_SHOUT_S),1.6f);
    } else {
        g_druid.berserk_timer=barb_berserk_time();
        sound_play(SFX_BARB_BERSERK);
        hud_show_banner(T(S_B_BERSERK_T),T(S_B_BERSERK_S),2.0f);
    }
}

static void gain(float f) {
    g_druid.spirit += f * (1.0f + g_mods.regen_pct * 0.5f);
    if (g_druid.spirit > g_druid.max_spirit) g_druid.spirit = g_druid.max_spirit;
    g_druid.fury_idle = 0.0f;
}
void barb_on_hit_taken(void) { gain(6.0f); }
int barb_damage_taken(int amount) {
    if (g_druid.berserk_timer > 0.0f) amount = amount * 6 / 5;   // berserker: +20% de dano recebido
    return amount;
}

static bool spend(float cost) {
    if (g_druid.spirit >= cost) { g_druid.spirit -= cost; return true; }
    char mb[48]; snprintf(mb, sizeof(mb), T(S_NO_FURY), (int)cost); hud_show_message(mb, 1.0f);
    return false;
}

// Golpe na frente (ou em volta). Devolve se acertou alguem.
static bool strike(float dist, float radius, int dmg, float knock) {
    float hx = g_druid.x + cosf(g_druid.yaw) * dist, hz = g_druid.z + sinf(g_druid.yaw) * dist;
    bool hit = enemies_damage_at(hx, hz, radius, cards_roll_damage(dmg, 0.0f), cosf(g_druid.yaw) * knock, sinf(g_druid.yaw) * knock);
    if (hit) {
        gain(FURY_PER_HIT);
        float ls = g_mods.lifesteal + (g_druid.berserk_timer > 0.0f ? 0.10f : 0.0f);
        if (ls > 0.0f) druid_heal((int)(dmg * ls + 0.5f));
    }
    return hit;
}

static void start_swing(float input_mag) {
    g_druid.state = DRUID_STATE_ATTACK;
    g_druid.vx = g_druid.vz = 0.0f;
    g_druid.walk_cycle = 0.0f;
    if (input_mag > 0.15f) g_druid.yaw = g_druid.target_yaw; else g_druid.target_yaw = g_druid.yaw;
    g_druid.attack_duration = (g_druid.combo >= 2 ? BARB_HEAVY_TIME : BARB_SWING_TIME) * (g_druid.berserk_timer > 0.0f ? BARB_BERSERK_ATTACK_TIME : 1.0f);
    g_druid.attack_timer = g_druid.attack_duration;
    g_druid.attack_hit_done = false;
    g_druid.combo_reset = 0.0f; // a janela entre cliques nunca vence no meio de um golpe
    sound_play(g_druid.combo >= 2 ? SFX_BEAST_SWING : SFX_BARB_AXE);
}

void barb_buttons(joypad_inputs_t *in, joypad_buttons_t *pressed, float input_mag) {
    (void)in;
    // R e imediato e nao interrompe o golpe/grito; nao concede salto nem invulnerabilidade.
    if (pressed->r && !g_mods.no_dodge && available(BARB_CD_RUSH)) {
        g_druid.rush_timer=BARB_RUSH_TIME;
        recharge(BARB_CD_RUSH);
        hud_show_message(T(S_B_RUSH),1.2f);
    }
    bool leaping = g_druid.leap_timer > 0.0f, spinning = g_druid.spin_timer > 0.0f;
    bool busy = g_druid.state == DRUID_STATE_DODGE || g_druid.state==DRUID_STATE_CAST || leaping || spinning;
    if (pressed->a && !busy) {
        if (g_druid.state == DRUID_STATE_ATTACK) {
            if (g_druid.combo_queued < 2) g_druid.combo_queued++;
        } else {
            g_druid.combo_queued = 0;
            start_swing(input_mag);
        }
    }
    if (busy || g_druid.state == DRUID_STATE_ATTACK) return;
    // B: Salto Esmagador (no inimigo mais proximo a frente, ou 80 unidades adiante)
    if (pressed->b && available(BARB_CD_LEAP) && spend(barb_leap_cost())) {
        recharge(BARB_CD_LEAP);
        float tx = g_druid.x + cosf(g_druid.yaw) * 80.0f, tz = g_druid.z + sinf(g_druid.yaw) * 80.0f;
        int t = enemies_find_closest(g_druid.x + cosf(g_druid.yaw) * 60.0f, g_druid.z + sinf(g_druid.yaw) * 60.0f, 70.0f);
        if (t >= 0) { Enemy *e = &enemies_get_array()[t]; tx = e->x; tz = e->z; }
        g_druid.leap_x0 = g_druid.x; g_druid.leap_z0 = g_druid.z;
        g_druid.leap_x1 = tx; g_druid.leap_z1 = tz;
        g_druid.leap_timer = BARB_LEAP_TIME;
        g_druid.leap_hit_done = false;
        g_druid.vx = g_druid.vz = 0.0f;
        g_druid.invuln_timer = BARB_LEAP_IMPACT + 0.05f;
        g_druid.yaw = g_druid.target_yaw = atan2f(tz - g_druid.z, tx - g_druid.x);
        sound_play(SFX_DODGE);
        return;
    }
    // C-esq: Grito de Guerra (atordoa em volta e da +25% de dano por 6 s)
    if (pressed->c_left && available(BARB_CD_SHOUT) && spend(BARB_SHOUT_COST)) {
        recharge(BARB_CD_SHOUT);
        start_warcry(1,input_mag);
        return;
    }
    // C-cima: Redemoinho
    if (pressed->c_up && available(BARB_CD_SPIN) && spend(BARB_SPIN_COST)) {
        recharge(BARB_CD_SPIN);
        g_druid.spin_timer = barb_spin_time();
        g_druid.spin_tick = 0.0f;
        g_druid.spin_clock = 0.0f; // o audio continuo e controlado por sound_update
        return;
    }
    // C-dir: Berserker
    if (pressed->c_right && available(BARB_CD_BERSERK) && g_druid.berserk_timer <= 0.0f && spend(BARB_BERSERK_COST)) {
        recharge(BARB_CD_BERSERK);
        start_warcry(2,input_mag);
    }
}

void barb_update(float dt) {
    for (int i=0;i<BARB_CD_COUNT;i++) g_druid.barb_cooldowns[i]=fmaxf(0.0f,g_druid.barb_cooldowns[i]-dt);
    g_druid.rush_timer=fmaxf(0.0f,g_druid.rush_timer-dt);
    // furia: esfria depois de 4 s sem lutar
    g_druid.fury_idle += dt;
    if (g_druid.fury_idle > 4.0f && g_druid.spirit > 0.0f) {
        g_druid.spirit -= 4.0f * dt;
        if (g_druid.spirit < 0.0f) g_druid.spirit = 0.0f;
    }
    if (g_druid.shout_timer > 0.0f) g_druid.shout_timer -= dt;
    if (g_druid.fx_shout > 0.0f) g_druid.fx_shout -= dt;
    if (g_druid.fx_impact > 0.0f) g_druid.fx_impact -= dt;
    if (g_druid.berserk_timer > 0.0f) {
        g_druid.berserk_timer -= dt;
        if (g_druid.berserk_timer <= 0.0f) hud_show_message(T(S_B_CALM), 1.5f);
    }
    if (g_druid.state != DRUID_STATE_ATTACK && g_druid.combo_reset > 0.0f) {
        g_druid.combo_reset -= dt;
        if (g_druid.combo_reset <= 0.0f) g_druid.combo = 0;
    }

    if (g_druid.leap_timer <= 0.0f) g_druid.y = 0.0f;
    // Os dois gritos compartilham 12 quadros (80 ms), som/efeito no pico do urso.
    if (g_druid.state==DRUID_STATE_CAST && g_druid.warcry_kind) {
        g_druid.vx=g_druid.vz=0.0f;
        float remaining=fmaxf(0.0f,g_druid.cast_timer-dt);
        if (!g_druid.warcry_fired && BARB_WARCRY_TIME-remaining>=BARB_WARCRY_PEAK) release_warcry();
        g_druid.cast_timer=remaining;
        if (remaining<=0.0f) { g_druid.warcry_kind=0; g_druid.state=DRUID_STATE_IDLE; }
        return;
    }

    // Salto aprovado: preparo, voo, impacto no quadro9 e recuperacao.
    if (g_druid.leap_timer > 0.0f) {
        g_druid.leap_timer = fmaxf(0.0f,g_druid.leap_timer-dt);
        float elapsed = BARB_LEAP_TIME-g_druid.leap_timer;
        float t = barb_leap_travel(elapsed);
        g_druid.x = g_druid.leap_x0 + (g_druid.leap_x1 - g_druid.leap_x0) * t;
        g_druid.z = g_druid.leap_z0 + (g_druid.leap_z1 - g_druid.leap_z0) * t;
        // A elevacao ja esta desenhada nos PNGs: nao somar outro salto nem subir a camera.
        g_druid.y = 0.0f;
        g_druid.vx = g_druid.vz = 0.0f;
        world_collide_circle(&g_druid.x, &g_druid.z, g_druid.radius);
        if (!g_druid.leap_hit_done && elapsed >= BARB_LEAP_IMPACT) {
            g_druid.leap_hit_done = true;
            float r = 52.0f * (1.0f + g_mods.leap_radius_pct);
            if (enemies_damage_at(g_druid.x, g_druid.z, r, cards_roll_damage(barb_leap_damage(), 0.0f), 0.0f, 0.0f)) gain(FURY_PER_HIT);
            enemies_root_at(g_druid.x, g_druid.z, r, 0.6f);
            g_druid.fx_impact = 0.0f; // terra/pedras ja estao nos quadros aprovados
            sound_play(SFX_BOSS_SLAM);
        }
        return;
    }
    // Redemoinho: dano em volta a cada 0,22 s
    if (g_druid.spin_timer > 0.0f) {
        g_druid.spin_timer = fmaxf(0.0f,g_druid.spin_timer-dt);
        g_druid.spin_clock += dt;
        if (g_druid.spin_timer <= 0.0f) return;
        g_druid.spin_tick -= dt;
        if (g_druid.spin_tick <= 0.0f) {
            g_druid.spin_tick = 0.22f;
            if (enemies_damage_at(g_druid.x, g_druid.z, 30.0f, cards_roll_damage(barb_spin_damage(), 0.0f), 0.0f, 0.0f)) gain(3.0f);
        }
        return;
    }
    // Combo
    if (g_druid.state == DRUID_STATE_ATTACK) {
        g_druid.attack_timer -= dt;
        bool heavy = g_druid.combo >= 2;
        if (!g_druid.attack_hit_done && g_druid.attack_timer <= g_druid.attack_duration * (1.0f - barb_combo_hit_phase(heavy))) {
            g_druid.attack_hit_done = true;
            strike(heavy ? 18.0f : 15.0f, heavy ? 26.0f : 20.0f, barb_axe_damage(g_druid.combo), heavy ? 2.4f : 1.0f);
            if (heavy) sound_play(SFX_BOSS_SLAM); // terra/pedras ja fazem parte dos quadros do golpe
        }
        if (g_druid.attack_timer <= 0.0f) {
            int next = g_druid.combo + 1;
            if (g_druid.combo_queued) {
                g_druid.combo_queued--;
                g_druid.combo = next > 2 ? 0 : next;
                start_swing(0.0f);
            }
            else {
                g_druid.state = DRUID_STATE_IDLE;
                g_druid.combo = next > 2 ? 0 : next;
                g_druid.combo_reset = 0.6f;            // apertar A logo continua o combo
            }
        }
    }
}
