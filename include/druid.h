#ifndef DRUID_H
#define DRUID_H

#include "common.h"

typedef enum {
    DRUID_STATE_IDLE,
    DRUID_STATE_WALK,
    DRUID_STATE_ATTACK,
    DRUID_STATE_CAST,
    DRUID_STATE_DODGE,
    DRUID_STATE_HURT,
    DRUID_STATE_DEAD
} DruidState;

typedef struct {
    float x, y, z;
    float vx, vz;
    float yaw;
    float target_yaw;
    float radius;

    // Attributes
    int hp;
    int max_hp;
    float spirit;
    float max_spirit;
    int attack_damage;
    int level;
    int exp;
    int exp_next;
    int health_potions;
    int spirit_potions;
    int continues;          // vidas extras ("continue"), dropam de baus e chefes
    int essence;            // essencia demoniaca (moeda do mascate)

    // Cacadora
    bool  charging;         // segurando A (arco puxado)
    float charge;           // tempo puxando o arco
    float focus_timer;      // Foco do Cacador: critico garantido
    float camo_timer;       // Manto: invisivel para os monstros

    // Barbaro (o "espirito" vira Furia: enche batendo e apanhando)
    int   combo;            // golpe atual do combo (0, 1, 2 = pesado)
    bool  combo_queued;     // apertou A de novo durante o golpe
    float combo_reset;      // tempo para o combo voltar ao primeiro golpe
    float spin_timer;       // Redemoinho
    float spin_tick;
    float leap_timer;       // Salto Esmagador (no ar)
    float leap_x0, leap_z0, leap_x1, leap_z1;
    float shout_timer;      // bonus de dano do grito
    float berserk_timer;    // Berserker
    float fury_idle;        // tempo sem lutar (a furia esfria)
    float fx_shout, fx_impact;   // efeitos visuais (tempo restante)
    float fx_x, fx_z;

    // Shapeshift
    bool is_beast;
    float beast_timer;

    // Animation & State
    DruidState state;
    float state_timer;
    float walk_cycle;
    float attack_timer;
    float attack_duration;
    bool  attack_hit_done;
    float cast_timer;
    float dodge_timer;
    float transform_timer;
    float hurt_timer;
    float invuln_timer;

    // Attack hitbox
    bool  has_active_hitbox;
    float hitbox_x, hitbox_z;
    float hitbox_radius;
    int   hitbox_damage;

    // Render matrix
    T3DMat4FP mat_fp;
} Druid;

extern Druid g_druid;

void druid_init(void);
void druid_reset_at(float x, float z);
void druid_update(float dt, joypad_inputs_t *in, joypad_buttons_t *pressed);
void druid_take_damage(int amount, float from_x, float from_z);
void druid_heal(int amount);
void druid_restore_spirit(float amount);
void druid_gain_exp(int amount);
void druid_revive(void);   // usa um continue: volta com vida e espirito cheios, no mesmo lugar
void druid_use_potion(void);
void druid_toggle_shapeshift(void);
void druid_model_init(void);
void druid_draw(void);

// Atributos de cada classe: valor base e quanto ganha por nivel.
typedef struct {
    int   hp, hp_lv;          // vida
    int   sp, sp_lv;          // espirito / energia / furia
    int   dmg, dmg_lv;        // dano da arma
    float armor, armor_lv, armor_cap;   // reducao de dano da classe
    float crit, crit_lv, crit_cap;      // chance de critico da classe
} HeroStats;
const HeroStats *hero_stats(void);
float hero_armor(void);   // armadura da classe no nivel atual (sem cartas)
float hero_crit(void);    // critico da classe no nivel atual (sem cartas)

#endif // DRUID_H
