#ifndef HUNTRESS_H
#define HUNTRESS_H
#include "common.h"

// Cacadora: arco (A: tiro / segurar = tiro carregado), B: rajada, C-esq: raizes, C-cima: gaviao,
// C-dir: Foco do Cacador, L: Manto de Camuflagem. R (rolar) e Z (pocao) sao comuns aos herois.
void hunt_buttons(joypad_inputs_t *in, joypad_buttons_t *pressed, float input_mag);
enum { HUNT_VISUAL_NONE, HUNT_VISUAL_VOLLEY, HUNT_VISUAL_FOCUS, HUNT_VISUAL_TRAP, HUNT_VISUAL_PIERCE };
void hunt_attack_update(float dt, joypad_inputs_t *in, float input_mag);

// Numeros atuais (nivel + cartas), usados pela pausa e pelo HUD
int   hunt_arrow_damage(void);
int   hunt_volley_count(void);
float hunt_volley_cost(void);
int   hunt_roots_damage(void);
float hunt_roots_time(void);
int   hunt_hawk_count(void);
int   hunt_hawk_damage(void);
float hunt_hawk_life(void);
float hunt_focus_time(void);
float hunt_camo_time(void);
float hunt_camo_cost(void);
#define HUNT_ROOTS_COST 25.0f
#define HUNT_HAWK_COST  35.0f
#define HUNT_FOCUS_COST 30.0f
#endif
