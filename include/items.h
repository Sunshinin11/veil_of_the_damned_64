#ifndef ITEMS_H
#define ITEMS_H

#include "common.h"

#define MAX_DROPS 64
#define MAX_CHESTS 12

typedef enum {
    DROP_HEALTH_POTION,
    DROP_SPIRIT_VIAL,
    DROP_SOUL_ORB,
    DROP_CONTINUE,     // vida extra ("continue"): usa o sprite da reliquia
    DROP_ESSENCE       // essencia demoniaca: moeda do mascate
} DropType;

typedef struct {
    bool active;
    DropType type;
    float x, y, z;
    float bounce_timer;
    float life;
    T3DMat4FP mat_fp;
} ItemDrop;

typedef struct {
    bool active;
    bool opened;
    float open_timer;
    float x, z;
    float yaw;
    bool small;        // bauzinho: abre direto, saque menor, sem carta
    bool locked;       // bau grande selado: so abre depois do evento (world.c)
    T3DMat4FP mat_fp;
} Chest;

void items_init(void);
void items_clear(void);
void items_spawn_drop(DropType type, float x, float z);
void items_add_chest(float x, float z, float yaw);
int  items_add_chest_ex(float x, float z, bool small, bool locked);   // indice ou -1
void items_unlock_chest(int i);
void items_update(float dt);
void items_draw(void);
const ItemDrop* items_get_drops(void);
const Chest* items_get_chests(void);

#endif // ITEMS_H
