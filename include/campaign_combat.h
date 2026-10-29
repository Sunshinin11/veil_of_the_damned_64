#ifndef CAMPAIGN_COMBAT_H
#define CAMPAIGN_COMBAT_H
#include "enemies.h"
#include "campaign_art_types.h"
#define CAMPAIGN_FX_MAX 16
enum { CFX_PROJECTILE,CFX_AREA,CFX_LINE,CFX_RING,CFX_SECTORS,CFX_CONE,CFX_DECOY };
typedef struct {
 float x,z,dx,dz,speed,radius,length,delay,life,total,windup;
 int damage,owner;
 unsigned short generation;
 unsigned char effect,kind,phase,hit,campaign;
} CampaignFX;
bool campaign_enemy_enabled(const Enemy *e);
int campaign_enemy_role(const Enemy *e);
void campaign_combat_clear(void);
void campaign_enemy_spawned(int index);
void campaign_combat_tick(float dt);
bool campaign_enemy_update(Enemy *e,int index,float dt,float distance,float dx,float dz);
const CampaignFX *campaign_get_fx(void);
bool campaign_effect_active(int owner,int effect);
bool campaign_fx_contains(const CampaignFX *fx,float x,float z);
float campaign_player_speed(void);
float campaign_player_grip(void);
bool campaign_player_cast_locked(void);
int campaign_incoming_damage(Enemy *e,int amount,float source_x,float source_z);
bool campaign_hit_decoys(float x,float z,float radius);
#endif

