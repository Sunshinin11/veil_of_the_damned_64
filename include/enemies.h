#ifndef ENEMIES_H
#define ENEMIES_H

#include "common.h"

#define MAX_ENEMIES 56

typedef enum {
    ENEMY_TYPE_SKELETON,
    ENEMY_TYPE_DEMON,
    ENEMY_TYPE_CULTIST,
    ENEMY_TYPE_BOSS_MALAKOR
} EnemyType;

typedef enum {
    ENEMY_STATE_IDLE,
    ENEMY_STATE_CHASE,
    ENEMY_STATE_TELEGRAPH,
    ENEMY_STATE_ATTACK,
    ENEMY_STATE_RECOVERY,
    ENEMY_STATE_HURT,
    ENEMY_STATE_DEAD
} EnemyState;

typedef struct {
    bool active;
    EnemyType type;
    EnemyState state;

    float x, y, z;
    float vx, vz;
    float yaw;
    float radius;

    int hp;
    int max_hp;
    int damage;
    int exp_reward;
    int pack;           // grupo de origem (acorda junto)
    bool elite;         // guardiao: mais vida, dano e velocidade
    float speed_mult;

    // AI timers
    float state_timer;
    float attack_cooldown;
    float telegraph_duration;
    float hurt_timer;
    float anim_timer;

    // Boss specific
    int boss_phase;
    float boss_special_cooldown;
    int   boss_move;        // golpe em preparacao (chefe)
    int   combo_step;       // golpes restantes no combo (chefe e guardioes)
    float morph_timer;      // transformacao Malakor -> Varkhul (invulneravel)
    float lunge_vx, lunge_vz;
    bool  lunge_hit;
    bool  summoned;         // fase 2 ja chamou reforcos
    // Cacadora
    float root_timer;       // preso pelas raizes (nao anda)
    float poison_t, poison_dps, poison_acc;
    uint8_t affix;          // guardioes no Pesadelo/Inferno: AFX_FAST | AFX_VAMP | AFX_EXPLODE

    // Hitbox to damage player
    bool  has_active_hitbox;
    float hitbox_x, hitbox_z;
    float hitbox_radius;
    int   hitbox_damage;

    T3DMat4FP mat_fp;
} Enemy;

// Multiplicador de forca dos monstros da area (1.0 = mapa 1). Definido por world_load_area().
extern float g_enemy_power;
// Agressividade da area (1.0 = mapa 1; menor = ataques mais rapidos e frequentes).
extern float g_enemy_aggro;
// Veneno a aplicar no proximo enemies_damage_at (flechas com carta de veneno). 0 = nenhum.
extern float g_hit_poison;
void enemies_root_at(float x, float z, float r, float time);
enum { AFX_FAST = 1, AFX_VAMP = 2, AFX_EXPLODE = 4 };
void enemies_set_affixes(int idx, int count);   // sorteia 'count' habilidades especiais

void enemies_init(void);
void enemies_clear(void);
int  enemies_spawn(EnemyType type, float x, float z);
int  enemies_spawn_ex(EnemyType type, float x, float z, int pack, bool elite);
void enemies_force_chase(int idx);
int  enemies_count_elites(void);
void enemies_update(float dt);
void enemies_draw(void);

bool enemies_damage_at(float x, float z, float radius, int damage, float knockback_x, float knockback_z);
int  enemies_find_closest(float x, float z, float max_dist);
Enemy* enemies_get_array(void);
int  enemies_get_active_count(void);
Enemy* enemies_get_boss(void);

#endif // ENEMIES_H
