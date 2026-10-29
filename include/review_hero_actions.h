#ifndef REVIEW_HERO_ACTIONS_H
#define REVIEW_HERO_ACTIONS_H
#include "common.h"

enum { HERO_VIS_SMOKE=16, HERO_VIS_SPIN, HERO_VIS_KNIFE,
       HERO_VIS_DRAIN, HERO_VIS_BARRIER, HERO_VIS_DARKNESS, HERO_VIS_WELL };
enum { HERO_FX_SMOKE, HERO_FX_DRAIN, HERO_FX_BARRIER, HERO_FX_DARKNESS,
       HERO_FX_CLAWS, HERO_FX_WOLF, HERO_FX_STAG, HERO_FX_WELL };
typedef struct { float x,z,life,duration,tick; unsigned char kind; } HeroEffect;
#define MAX_HERO_EFFECTS 6
void review_hero_update(float dt);
void review_hero_buttons(joypad_buttons_t *pressed,float input_mag);
void review_hero_reset(void);
void review_druid_stag(void);
void review_druid_buff(void);
const HeroEffect *review_hero_effects(void);
bool review_hero_in_smoke(void);
#endif
