#ifndef SPELLS_H
#define SPELLS_H

#include "common.h"

#define MAX_PROJECTILES 16
#define MAX_THORNS 12
#define MAX_WOLVES 5   // 2 base + cartas

typedef enum {
    PROJ_SPIRIT_BOLT,
    PROJ_DARK_BLOOD_ORB,
    PROJ_ARROW              // flecha da cacadora
} ProjectileType;

#define MAX_ROOTS 4
#define MAX_HAWKS 3
// Armadilha de raizes: arma no chao; quando um monstro chega perto, prende todos em volta.
typedef struct { bool active, sprung; float x, z, life, anim; } RootTrap;
// Gaviao: voa ate o inimigo mais proximo e ataca em mergulhos.
typedef struct { bool active; float x, y, z, yaw, life, max_life, cd, anim; int damage; } Hawk;

typedef struct {
    bool active;
    ProjectileType type;
    float x, y, z;
    float vx, vz;
    float radius;
    int damage;
    float life;
    bool is_enemy;
    bool pierce;       // carta: atravessa inimigos
    float hit_cd;      // intervalo entre acertos ao atravessar
    float yaw;         // direcao (flecha desenhada girada)
    T3DMat4FP mat_fp;
} Projectile;

typedef struct {
    bool active;
    float x, z;
    float scale_y;
    float life;
    float max_life;
    float radius;
    int damage;
    bool hit_done;
    T3DMat4FP mat_fp;
} ThornSpike;

typedef enum {
    WOLF_IDLE,
    WOLF_RUN,
    WOLF_ATTACK
} WolfState;

typedef struct {
    bool active;
    float x, y, z;
    float vx, vz;
    float yaw;
    float life;
    float max_life;  // vida inicial (a animacao de invocacao usa os primeiros 0,5 s)
    int target_enemy_idx;
    WolfState state;
    float attack_cooldown;
    int damage;
    float anim_timer;
    float stuck_t;   // tempo empurrando contra um obstaculo sem sair do lugar
    float ignore_t;  // tempo ignorando alvos (depois de ficar preso)
    T3DMat4FP mat_fp;
} SpiritWolf;

void spells_init(void);
void spells_update(float dt);
void spells_draw(void);

// Casting
bool spells_cast_spirit_bolt(float x, float z, float dir_x, float dir_z);
bool spells_cast_thorn_burst(float center_x, float center_z);
bool spells_cast_spirit_wolves(float x, float z);
bool spells_cast_arrow(float x, float z, float dir_x, float dir_z, int damage, bool pierce, float speed);
bool spells_place_roots(float x, float z);
bool spells_cast_hawks(float x, float z);
const RootTrap* spells_get_roots(void);
const Hawk* spells_get_hawks(void);
bool spells_cast_enemy_dark_orb(float x, float y, float z, float dir_x, float dir_z, int damage);

// Query
const Projectile* spells_get_projectiles(void);
const ThornSpike* spells_get_thorns(void);
const SpiritWolf* spells_get_wolves(void);
void spells_clear_all(void);

#endif // SPELLS_H
