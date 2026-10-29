#include "campaign_combat.h"
#include "gfx.h"
#include "lang.h"
#include "druid.h"
#include "review_hero_actions.h"
#include "cards.h"
#include "models.h"
#include "models_gen.h"
#include "audio.h"
#include "spells.h"
#include "enemies.h"
#include "world.h"
#include "hud.h"
#include "sprites.h"
#include "huntress.h"
#include "barbarian.h"

#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

Druid g_druid;
int g_hero = HERO_DRUID;

// ===== Modelo esqueletico do druida (Rota A: skinning animado do Tiny3D) =====
#define DRUID_MODEL_SCALE (1.0f / MESH_PREC)

static T3DModel   *s_druid_model = NULL;
static T3DSkeleton s_druid_skel;
static T3DAnim     s_anim[6];
static int         s_cur_anim = -1;
static const char *k_anim_names[6] = { "idle", "walk", "attack", "cast", "hurt", "death" };
static const bool  k_anim_loop[6]  = { true,   true,   false,    false,  false,  false  };
enum { ANIM_IDLE, ANIM_WALK, ANIM_ATTACK, ANIM_CAST, ANIM_HURT, ANIM_DEATH };

static void druid_anim_update(float dt);

void druid_model_init(void) {
    s_druid_model = t3d_model_load("rom:/druid.t3dm");
    s_druid_skel  = t3d_skeleton_create_buffered(s_druid_model, 3);
    for (int i = 0; i < 6; i++) {
        s_anim[i] = t3d_anim_create(s_druid_model, k_anim_names[i]);
        t3d_anim_attach(&s_anim[i], &s_druid_skel);
        t3d_anim_set_looping(&s_anim[i], k_anim_loop[i]);
        t3d_anim_set_playing(&s_anim[i], false);
    }
    s_cur_anim = -1;
    druid_anim_update(0.016f);
}

static int druid_anim_for_state(void) {
    if (g_druid.hp <= 0 || g_druid.state == DRUID_STATE_DEAD) return ANIM_DEATH;
    if (g_druid.state == DRUID_STATE_ATTACK) return ANIM_ATTACK;
    if (g_druid.state == DRUID_STATE_CAST)   return ANIM_CAST;
    if (g_druid.state == DRUID_STATE_DODGE)  return ANIM_WALK;
    if (g_druid.hurt_timer > 0.0f)           return ANIM_HURT;
    if (g_druid.state == DRUID_STATE_WALK)   return ANIM_WALK;
    return ANIM_IDLE;
}

static void druid_anim_update(float dt) {
    if (!s_druid_model) return;
    int want = druid_anim_for_state();
    if (want != s_cur_anim) {
        if (s_cur_anim >= 0) t3d_anim_set_playing(&s_anim[s_cur_anim], false);
        s_cur_anim = want;
        t3d_anim_set_playing(&s_anim[s_cur_anim], true);
        t3d_anim_set_time(&s_anim[s_cur_anim], 0.0f);
    }
    if (s_cur_anim == ANIM_WALK) {
        float sp = sqrtf(g_druid.vx * g_druid.vx + g_druid.vz * g_druid.vz);
        float r  = sp / (g_druid.is_beast ? 110.0f : 75.0f);
        t3d_anim_set_speed(&s_anim[s_cur_anim], 0.6f + r * 0.9f);
    } else {
        t3d_anim_set_speed(&s_anim[s_cur_anim], 1.0f);
    }
    t3d_anim_update(&s_anim[s_cur_anim], dt);
    t3d_skeleton_update(&s_druid_skel);
}

static void druid_build_matrix(void) {
    float d_s = DRUID_MODEL_SCALE;
    float d_scale[3] = { d_s, d_s, d_s };
    float d_rot[3]   = { 0.0f, g_druid.yaw - 1.5707963f, 0.0f };
    float d_pos[3]   = { g_druid.x, g_druid.y, g_druid.z };
    t3d_mat4fp_from_srt_euler(&g_druid.mat_fp, d_scale, d_rot, d_pos);
}

void druid_init(void) {
    memset(&g_druid, 0, sizeof(Druid));
    g_druid.x = 0.0f;
    g_druid.y = 0.0f;
    g_druid.z = 0.0f;
    g_druid.radius = 7.0f;
    g_druid.yaw = 1.5707963f; // Facing forward (+Z)
    g_druid.target_yaw = g_druid.yaw;

    g_druid.level = 1;
    g_druid.max_hp = g_druid.hp = hero_stats()->hp;          // atributos da classe
    g_druid.max_spirit = g_druid.spirit = (float)hero_stats()->sp;
    g_druid.attack_damage = hero_stats()->dmg;

    g_druid.level = 1;
    g_druid.exp = 0;
    g_druid.exp_next = 100;

    g_druid.health_potions = 3;
    g_druid.spirit_potions = 2;
    g_druid.continues = 0;
    g_druid.essence = 0;
    cards_reset();   // novo jogo: sem cartas
    if (g_hero == HERO_BARBARIAN) g_druid.spirit = 0.0f;   // furia comeca vazia

    g_druid.is_beast = false;
    g_druid.is_stag = false;
    review_hero_reset();
    g_druid.beast_timer = 0.0f;
    g_druid.state = DRUID_STATE_IDLE;
    druid_build_matrix();
}

// Druida: conjurador (mais espirito, magias crescem com o nivel), corpo fragil.
// Cacadora: agil, menos vida, muito critico.  Barbaro: tanque, muita vida, armadura e dano de arma.
static const HeroStats HERO_STATS[HERO_COUNT] = {
    [HERO_DRUID]     = {  90, 12, 120, 18, 18, 4, 0.00f, .004f, .10f, .03f, .002f, .15f },
    [HERO_HUNTRESS]  = {  80, 10, 100, 12, 20, 5, 0.03f, .004f, .12f, .08f, .006f, .30f },
    [HERO_BARBARIAN] = { 140, 22, 100,  5, 26, 6, 0.10f, .008f, .30f, .03f, .002f, .15f },
    [HERO_MAGE]      = {  90, 12, 120, 18, 18, 4, 0.00f, .004f, .10f, .03f, .002f, .15f }, // temporary staff kit
    [HERO_ROGUE]     = {  80, 10, 100, 12, 20, 5, 0.03f, .004f, .12f, .08f, .006f, .30f }, // reserved; locked until own art
};
const HeroStats *hero_stats(void) { return &HERO_STATS[(g_hero >= 0 && g_hero < HERO_COUNT) ? g_hero : 0]; }
static int lv_steps(void) { return (g_druid.level > 1 ? g_druid.level : 1) - 1; }
float hero_armor(void) {
    const HeroStats *h = hero_stats();
    float a = h->armor + h->armor_lv * lv_steps();
    return a > h->armor_cap ? h->armor_cap : a;
}
float hero_crit(void) {
    const HeroStats *h = hero_stats();
    float c = h->crit + h->crit_lv * lv_steps();
    return c > h->crit_cap ? h->crit_cap : c;
}

void druid_reset_at(float x, float z) {
    review_hero_reset();
    g_druid.visual_action=0;g_druid.visual_timer=g_druid.visual_duration=0;
    if (g_hero == HERO_BARBARIAN) barb_cancel_actions();
    g_druid.x = x;
    g_druid.y = 0.0f;
    g_druid.z = z;
    g_druid.vx = 0.0f;
    g_druid.vz = 0.0f;
    g_druid.state = DRUID_STATE_IDLE;
    g_druid.state_timer = 0.0f;
    g_druid.attack_timer = 0.0f;
    g_druid.cast_timer = 0.0f;
    g_druid.hurt_timer = 0.0f;
    g_druid.invuln_timer = 0.0f;
    g_druid.has_active_hitbox = false;
    druid_build_matrix();
}

static void druid_take_damage_limited(int amount, float from_x, float from_z, int maximum_loss) {
    if (VEIL_LEVEL_REVIEW && VEIL_REVIEW_GOD_MODE) return;
    if (g_druid.hp <= 0 || g_druid.invuln_timer > 0.0f) return;
    if(review_hero_in_smoke())amount=(amount+1)/2;

    // Armor damage reduction in beast form (-25%)
    if (g_druid.is_beast) {
        amount = (amount * 3) / 4;
        if (amount < 1) amount = 1;
    }

    if (g_hero == HERO_BARBARIAN) { amount = barb_damage_taken(amount); barb_on_hit_taken(); }
    {                                                   // armadura da classe + cartas (maldicao pode ser negativa)
        float armor = hero_armor() + g_mods.armor;
        if (armor > 0.75f) armor = 0.75f;
        if (armor != 0.0f) {
            amount = (int)((float)amount * (1.0f - armor) + 0.5f);
            if (amount < 1) amount = 1;
        }
    }
    // A percentage slam cannot become a one-shot through negative armor or
    // incoming-damage modifiers. Normal attacks keep their original behavior.
    if (maximum_loss>0 && amount>maximum_loss) amount=maximum_loss;
    g_druid.hp -= amount;
    g_druid.hurt_timer = 0.20f;
    g_druid.invuln_timer = 0.50f;
    sound_play(SFX_PLAYER_HURT);

    // Knockback
    float kdx = g_druid.x - from_x;
    float kdz = g_druid.z - from_z;
    float len = sqrtf(kdx * kdx + kdz * kdz);
    if (len > 0.001f) {
        g_druid.vx += (kdx / len) * 45.0f;
        g_druid.vz += (kdz / len) * 45.0f;
    }

    if (g_druid.hp <= 0) {
        g_druid.hp = 0;
        g_druid.state = DRUID_STATE_DEAD;
        g_druid.state_timer = 0.0f;
        hud_show_message(T(S_FELL), 3.0f);
    }
}

void druid_take_damage(int amount, float from_x, float from_z) {
    druid_take_damage_limited(amount,from_x,from_z,0);
}

void druid_take_damage_capped(int amount, float from_x, float from_z, int maximum_loss) {
    if (amount<=0 || maximum_loss<=0) return;
    druid_take_damage_limited(amount,from_x,from_z,maximum_loss);
}

void druid_heal(int amount) {
    g_druid.hp += amount;
    if (g_druid.hp > g_druid.max_hp) {
        g_druid.hp = g_druid.max_hp;
    }
}

void druid_restore_spirit(float amount) {
    g_druid.spirit += amount;
    if (g_druid.spirit > g_druid.max_spirit) {
        g_druid.spirit = g_druid.max_spirit;
    }
}

void druid_gain_exp(int amount) {
    g_druid.exp += amount;
    if (g_druid.exp >= g_druid.exp_next) {
        g_druid.level++;
        g_druid.exp -= g_druid.exp_next;
        g_druid.exp_next = (int)(g_druid.exp_next * 1.6f);

        // Ganhos de nivel da classe (vida/espirito saem da tabela em cards_refresh_stats)
        g_druid.attack_damage += hero_stats()->dmg_lv;
        cards_refresh_stats();   // vida/espirito maximos = classe + nivel + cartas
        g_druid.hp += g_druid.max_hp * 35 / 100;   // subir de nivel cura so 35% (nao e mais cura total)
        if (g_druid.hp > g_druid.max_hp) g_druid.hp = g_druid.max_hp;
        if (g_hero != HERO_BARBARIAN) g_druid.spirit = g_druid.max_spirit;   // furia nao enche ao subir de nivel

        sound_play(SFX_VICTORY);
        { char lb[48]; snprintf(lb, sizeof(lb), T(S_LVUP_S), g_druid.level); hud_show_banner(T(S_LVUP_T), lb, 2.5f); }
    }
}

void druid_revive(void) {
    if (g_druid.continues <= 0) return;
    g_druid.visual_action=0;g_druid.visual_timer=g_druid.visual_duration=0;
    if (g_hero == HERO_BARBARIAN) barb_cancel_actions();
    g_druid.continues--;
    if (g_seeds > 0) {                     // morrer custa as sementes
        g_seeds = 0;
        cards_refresh_stats();
        hud_show_banner(T(S_SEED_LOST_T), T(S_SEED_LOST_S), 3.0f);
    }
    g_druid.hp = g_druid.max_hp;
    g_druid.spirit = g_druid.max_spirit;
    g_druid.state = DRUID_STATE_IDLE;
    g_druid.state_timer = 0.0f;
    g_druid.hurt_timer = 0.0f;
    g_druid.invuln_timer = 3.0f;          // 3 s sem levar dano para sair do cerco
    g_druid.vx = g_druid.vz = 0.0f;
}

void druid_use_potion(void) {
    if (g_druid.hp <= 0) return;

    if (g_druid.health_potions > 0 && g_druid.hp < g_druid.max_hp) {
        g_druid.health_potions--;
        int heal = (int)(45.0f * (1.0f + g_mods.potion_pct));
        druid_heal(heal);
        sound_play(SFX_POTION);
    } else if (g_druid.spirit_potions > 0 && g_druid.spirit < g_druid.max_spirit) {
        g_druid.spirit_potions--;
        druid_restore_spirit(50.0f);
        sound_play(SFX_POTION);
    }
}

void druid_toggle_shapeshift(void) {
    if (g_druid.hp <= 0) return;

    if (!g_druid.is_beast) {
        if (g_druid.spirit < 25.0f) {
            { char mb[48]; snprintf(mb, sizeof(mb), T(S_NO_MANA), 25); hud_show_message(mb, 1.2f); }
            return;
        }
        g_druid.spirit -= 25.0f;
        g_druid.is_beast = true;
        g_druid.transform_timer = 0.32f;
        g_druid.beast_timer = 20.0f + g_mods.beast_time;
        sound_play(SFX_SHAPESHIFT);
        hud_show_banner(T(S_BEAST_T), T(S_BEAST_S), 2.0f);
        debugf("[DRUID] Shapeshifted into WEREWOLF! Dmg: %d, Speed: 110\n", g_druid.attack_damage + 12);
    } else {
        // Revert to human
        g_druid.is_beast = false;
        g_druid.is_stag = false;
        g_druid.transform_timer = 0.32f;
        g_druid.beast_timer = 0.0f;
        sound_play(SFX_SHAPESHIFT);
        hud_show_message(T(S_HUMAN), 1.5f);
        debugf("[DRUID] Reverted to HUMAN form.\n");
    }
}

void druid_update(float dt, joypad_inputs_t *in, joypad_buttons_t *pressed) {
    review_hero_update(dt);
    if(g_druid.visual_timer>0)g_druid.visual_timer=fmaxf(0,g_druid.visual_timer-dt);
    // Witch stitching blocks casts only: movement, basic attacks, potion and dodge remain available.
    joypad_buttons_t unlocked_buttons;
    if(pressed && campaign_player_cast_locked()) {
        unlocked_buttons=*pressed;
        unlocked_buttons.b=unlocked_buttons.c_left=unlocked_buttons.c_up=unlocked_buttons.c_right=0;
        pressed=&unlocked_buttons;
    }
    if (g_druid.hp <= 0) {
        g_druid.state = DRUID_STATE_DEAD;
        g_druid.state_timer += dt;
        druid_build_matrix();
        druid_anim_update(dt);
        return;
    }

    // Timers
    if (g_druid.hurt_timer > 0.0f) g_druid.hurt_timer -= dt;
    if (g_druid.invuln_timer > 0.0f) g_druid.invuln_timer -= dt;
    if (g_druid.transform_timer > 0.0f) g_druid.transform_timer -= dt;

    // Passive Spirit regeneration & Beast form drain
    if (g_druid.is_beast) {
        g_druid.beast_timer -= dt;
        g_druid.spirit -= 3.5f * dt;
        druid_heal((int)(3.0f * dt)); // Health regeneration in beast form

        if (g_druid.beast_timer <= 0.0f || g_druid.spirit <= 0.0f) {
            g_druid.spirit = 0.0f;
            druid_toggle_shapeshift();
        }
    } else {
        float rg = 1.0f + g_mods.regen_pct;
        float base = g_hero == HERO_HUNTRESS ? 13.0f : 10.0f;   // cacadora: energia volta mais rapido
        if (g_hero != HERO_BARBARIAN && rg > 0.0f) druid_restore_spirit(base * rg * dt);   // barbaro: furia so lutando
    }
    if (g_druid.focus_timer > 0.0f) g_druid.focus_timer -= dt;
    if (g_druid.camo_timer > 0.0f) g_druid.camo_timer -= dt;

    // Input processing
    float move_x = 0.0f;
    float move_z = 0.0f;

    if (in) {
        // Analog stick input with center deadzone
        int sx = in->stick_x;
        int sy = in->stick_y;
        if (abs(sx) > 8) move_x = (float)sx / 80.0f;
        if (abs(sy) > 8) move_z = (float)sy / 80.0f;
    }

    // Clamp input magnitude
    float input_mag = sqrtf(move_x * move_x + move_z * move_z);
    if (input_mag > 1.0f) {
        move_x /= input_mag;
        move_z /= input_mag;
        input_mag = 1.0f;
    }

    // Speed setting
    float speed = (g_druid.is_stag ? 80.0f : g_druid.is_beast ? 110.0f : g_hero == HERO_HUNTRESS ? 82.0f : g_hero == HERO_BARBARIAN ? 78.0f : 75.0f) * (1.0f + g_mods.speed_pct);
    if (g_hero == HERO_BARBARIAN) speed *= barb_speed_mult();
    if(g_druid.state!=DRUID_STATE_DODGE)speed*=campaign_player_speed();
    if (g_druid.state == DRUID_STATE_DODGE) speed *= 1.6f;
    else speed *= world_ground_speed(g_druid.x, g_druid.z);   // agua do pantano
    float accel = 12.0f * world_ground_grip(g_druid.x, g_druid.z)*campaign_player_grip();   // gelo liso: escorrega

    // A camera olha na direcao +Z (olho atras, em -Z), entao a tela fica espelhada
    // em relacao a uma camera -Z padrao: inverte os dois eixos p/ o analogico bater.
    float target_vx = -move_x * speed; //  direita no analogico  -> direita na tela
    float target_vz =  move_z * speed; //  cima no analogico     -> cima na tela (+Z, longe)
    if(g_druid.state==DRUID_STATE_DODGE) {
        target_vx=g_hero==HERO_ROGUE?0:cosf(g_druid.yaw)*speed;
        target_vz=g_hero==HERO_ROGUE?0:sinf(g_druid.yaw)*speed;
    }
    if(g_hero==HERO_HUNTRESS && g_druid.state==DRUID_STATE_ATTACK) {
        float recoil=70*sinf(3.14159265f*(1-g_druid.attack_timer/fmaxf(.01f,g_druid.attack_duration)));
        target_vx-=cosf(g_druid.yaw)*recoil;target_vz-=sinf(g_druid.yaw)*recoil;
    }
    float aim_vx = target_vx, aim_vz = target_vz;
    // Golpe corpo a corpo: o druida PARA para golpear e so volta a andar quando a animacao termina.
    if (g_druid.state == DRUID_STATE_ATTACK && g_hero != HERO_HUNTRESS) {
        target_vx = 0.0f; target_vz = 0.0f;
    }
    if (g_hero == HERO_BARBARIAN && (g_druid.state == DRUID_STATE_CAST || g_druid.leap_timer > 0.0f)) {
        target_vx = target_vz = 0.0f;
        g_druid.vx = g_druid.vz = 0.0f;
    }

    g_druid.vx = lerpf(g_druid.vx, target_vx, clampf(accel * dt, 0.0f, 1.0f));
    g_druid.vz = lerpf(g_druid.vz, target_vz, clampf(accel * dt, 0.0f, 1.0f));

    // Update position
    g_druid.x += g_druid.vx * dt;
    g_druid.z += g_druid.vz * dt;

    // Collide with world bounds & obstacles
    world_collide_circle(&g_druid.x, &g_druid.z, g_druid.radius);

    // Update facing angle when moving
    if (input_mag > 0.15f) {
        g_druid.target_yaw = atan2f(aim_vz, aim_vx); // direcao do INPUT, mesmo durante o ataque
        if (g_druid.state != DRUID_STATE_ATTACK && g_druid.state != DRUID_STATE_CAST &&
            g_druid.state != DRUID_STATE_DODGE) {
            g_druid.state = DRUID_STATE_WALK;
        }
    } else {
        if (g_druid.state != DRUID_STATE_ATTACK && g_druid.state != DRUID_STATE_CAST &&
            g_druid.state != DRUID_STATE_DODGE) {
            g_druid.state = DRUID_STATE_IDLE;
        }
    }

    // Keep the step cycle moving while velocity eases to zero after release;
    // otherwise the final walking pose freezes for a few visible frames.
    float move_speed = sqrtf(g_druid.vx * g_druid.vx + g_druid.vz * g_druid.vz);
    if (move_speed > 10.0f) {
        float max_speed = g_druid.is_beast ? 110.0f : 75.0f;
        float step_rate = g_druid.is_beast ? 10.0f : 8.0f;
        g_druid.walk_cycle += dt * step_rate * clampf(move_speed / max_speed, 0.0f, 1.0f);
    }

    // Smooth yaw rotation (travada durante o golpe)
    if ((g_druid.state == DRUID_STATE_ATTACK && g_hero != HERO_HUNTRESS) ||
        (g_hero == HERO_BARBARIAN && (g_druid.state == DRUID_STATE_CAST || g_druid.leap_timer > 0.0f))) g_druid.target_yaw = g_druid.yaw;
    float dyaw = g_druid.target_yaw - g_druid.yaw;
    while (dyaw > 3.14159f) dyaw -= 6.28318f;
    while (dyaw < -3.14159f) dyaw += 6.28318f;
    g_druid.yaw += dyaw * clampf(18.0f * dt, 0.0f, 1.0f);

    // Action buttons
    if (pressed) {
        // R: short dodge, shared by Druid and Werewolf.
        if (g_hero != HERO_BARBARIAN && pressed->r && g_druid.state != DRUID_STATE_DODGE && !g_mods.no_dodge && g_druid.leap_timer <= 0.0f) {
            g_druid.state = DRUID_STATE_DODGE;
            g_druid.dodge_timer = 0.30f;
            g_druid.invuln_timer = g_druid.dodge_timer;
            float dash_x = input_mag > .15f ? -move_x / input_mag : cosf(g_druid.yaw);
            float dash_z = input_mag > .15f ?  move_z / input_mag : sinf(g_druid.yaw);
            float dash = speed;
            g_druid.vx = dash_x * dash;
            g_druid.vz = dash_z * dash;
            g_druid.yaw=g_druid.target_yaw=atan2f(dash_z,dash_x);
            if(g_hero==HERO_ROGUE) {
                g_druid.x+=dash_x*42;g_druid.z+=dash_z*42;
                world_collide_circle(&g_druid.x,&g_druid.z,g_druid.radius);
                g_druid.vx=g_druid.vz=0;
            }
            sound_play(SFX_DODGE);
        }
        if (g_hero == HERO_HUNTRESS) {
            hunt_buttons(in, pressed, input_mag);
        } else if (g_hero == HERO_BARBARIAN) {
            barb_buttons(in, pressed, input_mag);
        } else if (g_hero == HERO_ROGUE || g_hero == HERO_MAGE) {
            review_hero_buttons(pressed,input_mag);
        } else {
        // A Button: Basic Melee Attack
        if (pressed->a && g_druid.state != DRUID_STATE_ATTACK &&
            g_druid.state != DRUID_STATE_DODGE) {
            g_druid.state = DRUID_STATE_ATTACK;
            g_druid.vx = 0.0f; g_druid.vz = 0.0f;              // para na hora (sem deslizar)
            g_druid.walk_cycle = 0.0f;
            if (input_mag > 0.15f) g_druid.yaw = g_druid.target_yaw;   // golpeia para onde o analogico aponta
            else g_druid.target_yaw = g_druid.yaw;
            // Mais curto e com o acerto mais cedo: antes o dano so saia ~0,24 s depois do botao.
            g_druid.attack_duration = g_druid.is_beast ? 0.36f : 0.45f;
            if(g_druid.focus_timer>0 && !g_druid.is_beast)g_druid.attack_duration*=.7f;
            g_druid.attack_timer = g_druid.attack_duration;
            g_druid.attack_hit_done = false;
            sound_play(g_druid.is_beast ? SFX_BEAST_SWING : SFX_SWING);
        }

        // B Button: Spirit Bolt
        if (pressed->b && g_druid.state != DRUID_STATE_ATTACK &&
            g_druid.state != DRUID_STATE_DODGE) {
            float bolt_cost = 15.0f + g_mods.bolt_cost;
            if (bolt_cost < 5.0f) bolt_cost = 5.0f;
            if (g_druid.spirit >= bolt_cost) {
                float dir_x = cosf(g_druid.yaw);
                float dir_z = sinf(g_druid.yaw);
                if (spells_cast_spirit_bolt(g_druid.x + dir_x * 8.0f, g_druid.z + dir_z * 8.0f, dir_x, dir_z)) {
                    g_druid.spirit -= bolt_cost;
                    g_druid.state = DRUID_STATE_CAST;
                    g_druid.cast_timer = 0.25f;
                }
            } else {
                { char mb[48]; snprintf(mb, sizeof(mb), T(S_NO_MANA), (int)bolt_cost); hud_show_message(mb, 1.0f); }
            }
        }

        // C-Left: Thorn Burst
        if (pressed->c_left && g_druid.state != DRUID_STATE_DODGE &&
            g_druid.state != DRUID_STATE_ATTACK) {
            review_druid_stag();
        }

        // C-Up: Spirit Wolves
        if (pressed->c_up && g_druid.state != DRUID_STATE_DODGE &&
            g_druid.state != DRUID_STATE_ATTACK) {
            if (g_druid.spirit >= 40.0f) {
                if (spells_cast_spirit_wolves(g_druid.x, g_druid.z)) {
                    g_druid.spirit -= 40.0f;
                    g_druid.state = DRUID_STATE_CAST;
                    g_druid.cast_timer = 0.30f;
                    hud_show_banner(T(S_WOLVES_T), T(S_WOLVES_S), 2.0f);
                }
            } else {
                { char mb[48]; snprintf(mb, sizeof(mb), T(S_NO_MANA), 40); hud_show_message(mb, 1.0f); }
            }
        }

        // C-Right: Shapeshift
        if (pressed->c_right && g_druid.state != DRUID_STATE_DODGE &&
            g_druid.state != DRUID_STATE_ATTACK) {
            if(g_druid.is_stag){g_druid.is_stag=false;g_druid.is_beast=false;}
            druid_toggle_shapeshift();
        }
        if(pressed->l)review_druid_buff();

        }   // fim dos botoes do druida

        // Z or C-Down: Potion
        if (pressed->z || pressed->c_down) {
            druid_use_potion();
        }
    }

    // Attack state update & Hitbox execution
    if (g_hero == HERO_HUNTRESS) {
        hunt_attack_update(dt, in, input_mag);
    } else if (g_hero == HERO_BARBARIAN) {
        barb_update(dt);
    } else if (g_druid.state == DRUID_STATE_ATTACK) {
        g_druid.attack_timer -= dt;

        // Active hit window around midway through swing
        if (!g_druid.attack_hit_done && g_druid.attack_timer < (g_druid.attack_duration * 0.72f)) {
            g_druid.attack_hit_done = true;
            float h_dist = g_druid.is_beast ? 18.0f : 15.0f;
            float hx = g_druid.x + cosf(g_druid.yaw) * h_dist;
            float hz = g_druid.z + sinf(g_druid.yaw) * h_dist;
            float h_rad = g_druid.is_beast ? 20.0f : 16.0f;
            int dmg = cards_roll_damage(g_druid.attack_damage + (g_druid.is_beast ? 14 : 0),
                                        g_mods.melee_pct + (g_druid.is_beast ? g_mods.beast_pct : 0.0f));
            if(g_hero==HERO_ROGUE && g_druid.combo==2)dmg*=2;
            if(g_hero==HERO_MAGE && g_druid.focus_timer>0)dmg=dmg*3/2;

            // Damage enemies in melee arc
            bool hit = false;
            if(g_hero==HERO_MAGE)spells_cast_mage_orb(hx,hz,cosf(g_druid.yaw),sinf(g_druid.yaw),dmg);
            else hit=enemies_damage_at(hx, hz, h_rad, dmg, cosf(g_druid.yaw), sinf(g_druid.yaw));
            if (hit && g_mods.lifesteal > 0.0f) {          // cartas de roubo de vida
                int heal = (int)(dmg * g_mods.lifesteal + 0.5f);
                if (heal > 0) druid_heal(heal);
            }
            if(hit && g_hero==HERO_DRUID && g_druid.is_beast && !g_druid.is_stag && g_druid.focus_timer>0)druid_heal(dmg/10);
        }

        if (g_druid.attack_timer <= 0.0f) {
            g_druid.state = DRUID_STATE_IDLE;
        }
    }

    // Cast state update
    if (g_druid.state == DRUID_STATE_CAST && g_hero != HERO_BARBARIAN) {
        g_druid.cast_timer -= dt;
        if (g_druid.cast_timer <= 0.0f) {
            g_druid.state = DRUID_STATE_IDLE;
        }
    }

    if (g_druid.state == DRUID_STATE_DODGE) {
        g_druid.dodge_timer -= dt;
        if (g_druid.dodge_timer <= 0.0f) g_druid.state = DRUID_STATE_IDLE;
    }

    // Matriz raiz (posicao + direcao) e avanco da animacao esqueletica
    druid_build_matrix();
    druid_anim_update(dt);
}

static void druid_push_part(const float pos[3], const float rot[3]) {
    gfx_push_part((float[]){pos[0]*MESH_PREC,pos[1]*MESH_PREC,pos[2]*MESH_PREC},rot);
}

void druid_draw(void) {
    // O humano usa geometria 3D real; as formas transformadas mantêm a arte atual.
    if (g_hero != HERO_DRUID || g_druid.is_beast || g_druid.is_stag) return;
    gfx_matrix_push(&g_druid.mat_fp); // raiz: posicao + direcao

    // cor por vertice (regioes do corpo), sem textura -> combiner SHADE, sem luz
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | T3D_FLAG_NO_LIGHT | gfx_cull_flag());
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);

    float speed = sqrtf(g_druid.vx*g_druid.vx + g_druid.vz*g_druid.vz);
    float stride = fminf(1.0f, speed / 55.0f);
    float phase = g_druid.walk_cycle * 1.5707963f;
    float swing = sinf(phase);
    float crouch = stride * 1.25f;

    // Tronco inclinado, baixo e com braços abertos: corrida animalesca.
    druid_push_part((float[]){0,-crouch,0},(float[]){-stride*.16f,0,0});
    models_draw_mesh(&mesh_druid3d_body);
    druid_push_part((float[]){0,23.0f,0},(float[]){-stride*.10f,0,0});
    models_draw_mesh(&mesh_druid3d_head);
    t3d_matrix_pop(1);
    t3d_matrix_pop(1);

    for (int side=-1;side<=1;side+=2) {
        float leg = swing * (float)side * stride;
        float arm = -leg;
        druid_push_part((float[]){side*5.0f,21.7f,0},
                      (float[]){-.28f-arm*.55f,0,side*(.33f+stride*.18f)});
        models_draw_mesh(&mesh_druid3d_upper_arm);
        druid_push_part((float[]){0,-6.2f,0},(float[]){-.30f+arm*.40f,0,side*.08f});
        models_draw_mesh(&mesh_druid3d_lower_arm);
        t3d_matrix_pop(2);

        // Joelhos alternam de verdade; a canela dobra no voo e estende no apoio.
        druid_push_part((float[]){side*2.0f,13.9f,0},
                      (float[]){leg*.82f,0,side*.04f});
        models_draw_mesh(&mesh_druid3d_upper_leg);
        float knee = stride*(.15f+fmaxf(0.0f,-leg)*1.05f);
        druid_push_part((float[]){0,-6.8f,0},(float[]){knee,0,0});
        models_draw_mesh(&mesh_druid3d_lower_leg);
        t3d_matrix_pop(2);
    }
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
    t3d_matrix_pop(1);
}
