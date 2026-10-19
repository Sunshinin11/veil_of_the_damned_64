#ifndef BARBARIAN_H
#define BARBARIAN_H
#include "common.h"

// Barbaro: A combo de 3 golpes (o 3o e pesado), B Salto Esmagador, C-esq Grito de Guerra,
// C-cima Redemoinho, C-dir Berserker. R esquiva e Z pocao sao comuns. A Furia (o "espirito")
// enche batendo e apanhando e esfria fora de combate.
void barb_buttons(joypad_inputs_t *in, joypad_buttons_t *pressed, float input_mag);
void barb_update(float dt);
void barb_on_hit_taken(void);            // apanhar tambem da furia
int  barb_damage_taken(int amount);      // berserker recebe mais dano
float barb_speed_mult(void);

int   barb_axe_damage(int step);         // 0,1 golpes rapidos; 2 golpe pesado
int   barb_leap_damage(void);
float barb_leap_cost(void);
float barb_shout_radius(void);
float barb_shout_stun(void);
int   barb_spin_damage(void);
float barb_spin_time(void);
float barb_berserk_time(void);
#define BARB_SHOUT_COST   20.0f
#define BARB_SPIN_COST    30.0f
#define BARB_BERSERK_COST 50.0f
#endif
