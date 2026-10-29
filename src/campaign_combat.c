#include "campaign_combat.h"
#include "druid.h"
#include "world.h"
#include "audio.h"
#include "spells.h"

static CampaignFX effects[CAMPAIGN_FX_MAX];
static float readiness[MAX_ENEMIES],feral_rush[MAX_ENEMIES],cast_lock,ankle_bind;
static unsigned short generations[MAX_ENEMIES];
#define PI 3.14159265358979323846f

int campaign_enemy_role(const Enemy *e) {return e->type==ENEMY_TYPE_BOSS_MALAKOR?(e->boss_phase==2?4:3):(int)e->type;}
bool campaign_enemy_enabled(const Enemy *e) {
 int camp=enemy_campaign(e);return (camp>=1 && camp<=4 && e->variant!=EV_PRIEST) || e->variant==EV_MINIBOSS;
}
void campaign_combat_clear(void) {
 memset(effects,0,sizeof(effects));memset(readiness,0,sizeof(readiness));memset(feral_rush,0,sizeof(feral_rush));cast_lock=0;ankle_bind=0;
 for(int i=0;i<MAX_ENEMIES;i++)generations[i]++;
}
void campaign_enemy_spawned(int i) {
 if(i<0 || i>=MAX_ENEMIES)return;
 readiness[i]=0;feral_rush[i]=0;generations[i]++;
}
const CampaignFX *campaign_get_fx(void) {return effects;}
bool campaign_effect_active(int owner,int effect) {
 for(int i=0;i<CAMPAIGN_FX_MAX;i++)if(effects[i].life>0 && effects[i].owner==owner && effects[i].effect==effect)return true;
 return false;
}
static float angle_delta(float a,float b) {
 float x=fmodf(a-b+PI,2*PI);if(x<0)x+=2*PI;return x-PI;
}
bool campaign_fx_contains(const CampaignFX *f,float x,float z) {
 float dx=x-f->x,dz=z-f->z,d2=dx*dx+dz*dz;
 if(f->kind==CFX_LINE) {
  float along=clampf(dx*f->dx+dz*f->dz,0,f->length);
  dx-=f->dx*along;dz-=f->dz*along;return dx*dx+dz*dz<=f->radius*f->radius;
 }
 if(f->kind==CFX_RING) {
  float d=sqrtf(d2);
  return fabsf(d-f->length)<=f->radius && fabsf(angle_delta(atan2f(dz,dx),atan2f(f->dz,f->dx)))>.65f;
 }
 if(f->kind==CFX_SECTORS)
  return d2<=f->radius*f->radius && d2>14*14 && cosf(4*(atan2f(dz,dx)-atan2f(f->dz,f->dx)))>.45f;
 if(f->kind==CFX_CONE) {
  float d=sqrtf(d2);return d<=f->radius && (d<1 || (dx*f->dx+dz*f->dz)/d>.45f);
 }
 return d2<=f->radius*f->radius;
}
static bool spawn_fx(Enemy *e,int index,int effect,int kind,float x,float z,
                     float angle,float radius,float length,float delay,float active,int damage,float speed) {
 // Never create an invisible damaging attack when the bounded pool is full.
 for(int i=0;i<CAMPAIGN_FX_MAX;i++)if(effects[i].life<=0) {
  if(kind!=CFX_PROJECTILE)world_collide_circle(&x,&z,4);
  effects[i]=(CampaignFX){.x=x,.z=z,.dx=cosf(angle),.dz=sinf(angle),.speed=speed,
   .radius=radius,.length=length,.delay=delay,.windup=delay,.life=delay+active,.total=delay+active,
   .damage=damage,.owner=index,.generation=generations[index],.effect=(unsigned char)effect,
   .kind=(unsigned char)kind,.phase=(unsigned char)e->boss_phase,.campaign=(unsigned char)enemy_campaign(e)};
  return true;
 }
 return false;
}
static void damage_player(int amount,float x,float z) {
 if(amount<1 || g_druid.state==DRUID_STATE_DODGE || g_druid.leap_timer>0)return;
 int cap=(int)(g_druid.max_hp*.35f);if(cap<1)cap=1;
 if(amount>cap)amount=cap;
 druid_take_damage_capped(amount,x,z,cap);
}
void campaign_combat_tick(float dt) {
 if(dt<=0)return;
 cast_lock=fmaxf(0,cast_lock-dt);
 ankle_bind=fmaxf(0,ankle_bind-dt);
 for(int i=0;i<MAX_ENEMIES;i++){readiness[i]=fmaxf(0,readiness[i]-dt);feral_rush[i]=fmaxf(0,feral_rush[i]-dt);}
 Enemy *all=enemies_get_array();
 for(int i=0;i<CAMPAIGN_FX_MAX;i++) {
  CampaignFX *f=&effects[i];if(f->life<=0)continue;
  if(f->owner<0 || f->owner>=MAX_ENEMIES || !all[f->owner].active ||
     all[f->owner].state==ENEMY_STATE_DEAD || f->generation!=generations[f->owner] ||
     all[f->owner].boss_phase!=f->phase) {f->life=0;continue;}
  f->life-=dt;if(f->life<=0)continue;
  if(f->delay>0) {f->delay=fmaxf(0,f->delay-dt);continue;}
  if(f->kind==CFX_PROJECTILE) {
   if(f->effect==CFE_ESPADA_VOADORA && f->life<(f->total-f->windup)*.5f) {
    float dx=all[f->owner].x-f->x,dz=all[f->owner].z-f->z,d=sqrtf(dx*dx+dz*dz);
    if(d<8) {f->life=0;continue;}f->dx=dx/d;f->dz=dz/d;
   }
   float x=f->x+f->dx*f->speed*dt,z=f->z+f->dz*f->speed*dt,tx=x,tz=z;
   world_collide_circle(&tx,&tz,f->radius);
   if(vec2_dist_sq(x,z,tx,tz)>1) {f->life=0;continue;}f->x=x;f->z=z;
  }
  if(f->hit || f->kind==CFX_DECOY || g_druid.hp<=0 ||
     g_druid.state==DRUID_STATE_DODGE || g_druid.leap_timer>0 ||
     !campaign_fx_contains(f,g_druid.x,g_druid.z))continue;
  if(f->effect==CFE_COSTURA_MALDITA) {cast_lock=fmaxf(cast_lock,2.5f);f->hit=1;}
  if(f->effect==CFE_ESCARAVELHO)ankle_bind=fmaxf(ankle_bind,1.4f);
  if(f->effect==CFE_LAMENTO_MORTO) {g_druid.spirit=fmaxf(0,g_druid.spirit-22);f->hit=1;}
  if(f->damage>0) {
   int before=g_druid.hp;damage_player(f->damage,f->x,f->z);f->hit=1;
   Enemy *owner=&all[f->owner];
   if((owner->affix&AFX_VAMP) && g_druid.hp<before) {
    owner->hp+=(before-g_druid.hp)*3;if(owner->hp>owner->max_hp)owner->hp=owner->max_hp;
   }
  }
 }
}
static bool inside_effect(int id) {
 for(int i=0;i<CAMPAIGN_FX_MAX;i++)if(effects[i].life>0 && effects[i].delay<=0 &&
    effects[i].effect==id && campaign_fx_contains(&effects[i],g_druid.x,g_druid.z))return true;
 return false;
}
float campaign_player_speed(void) {return ankle_bind>0?.6f:inside_effect(CFE_VEU_ESPOROS)?.65f:1.0f;}
float campaign_player_grip(void) {return inside_effect(CFE_GEADA_SOLO)?.3f:1.0f;}
bool campaign_player_cast_locked(void) {return cast_lock>0;}
bool campaign_hit_decoys(float x,float z,float radius) {
 bool hit=false;
 for(int i=0;i<CAMPAIGN_FX_MAX;i++) {
  CampaignFX *f=&effects[i];
  if(f->life>0 && f->delay<=0 && f->kind==CFX_DECOY &&
     vec2_dist_sq(x,z,f->x,f->z)<(radius+f->radius)*(radius+f->radius)) {f->life=0;hit=true;}
 }
 return hit;
}
int campaign_incoming_damage(Enemy *e,int damage,float x,float z) {
 int camp=enemy_campaign(e);
 if(!campaign_enemy_enabled(e) || damage<=0)return damage;
 int role=campaign_enemy_role(e);
 if(camp==1 && role==1 && e->boss_move==1 &&
    (e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK)) {
  float dx=x-e->x,dz=z-e->z,d=sqrtf(dx*dx+dz*dz);
  if(d>1 && (dx*cosf(e->yaw)+dz*sinf(e->yaw))/d>.25f)damage=(damage*35+99)/100;
 }
 if(camp==3 && role==3 && e->boss_move==2) {
  if(e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK)e->lunge_hit=true;
  else if(e->state==ENEMY_STATE_RECOVERY)damage=damage*5/4;
 }
 return damage<1?1:damage;
}
static void melee(Enemy *e,float reach,float multiplier) {
 float dx=g_druid.x-e->x,dz=g_druid.z-e->z,d=sqrtf(dx*dx+dz*dz);
 if(d<=reach+g_druid.radius && (d<1 || (dx*cosf(e->yaw)+dz*sinf(e->yaw))/d>.2f))
 {
  int before=g_druid.hp;damage_player((int)(e->damage*multiplier),e->x,e->z);
  if((e->affix&AFX_VAMP) && g_druid.hp<before) {
   e->hp+=(before-g_druid.hp)*3;if(e->hp>e->max_hp)e->hp=e->max_hp;
  }
 }
}
static void execute(Enemy *e,int index) {
 int camp=enemy_campaign(e);
 int role=campaign_enemy_role(e),move=e->boss_move,D=e->damage;
 float boost=readiness[index]>0?1.15f:1;
 if(readiness[index]>0)D=D*115/100;
 float a=e->yaw,x=e->lunge_vx,z=e->lunge_vz;
 // Fodder has exactly one basic attack and never receives a borrowed skill.
 if(role==0) {melee(e,25,readiness[index]>0?1.15f:1);return;}
 if(camp==0) {
  if(move==0)melee(e,role==3?52:54,boost);
  else if(role==3)spells_cast_enemy_stone_wave(e->x+cosf(a)*30,1,e->z+sinf(a)*30,cosf(a),sinf(a),D*16/10);
  else feral_rush[index]=5;
 } else if(camp==1) {
  if(role==1) {if(move==0)melee(e,32,boost);return;} // bark defense has no hidden AoE
  if(role==2) {
   if(move==0)spawn_fx(e,index,CFE_DARDO_MICELIO,CFX_PROJECTILE,e->x,e->z,a,7,0,0,2.6f,D,95);
   else spawn_fx(e,index,CFE_VEU_ESPOROS,CFX_AREA,x,z,a,28,0,1.1f,3.5f,0,0);
  } else if(role==3) {
   if(move==0)melee(e,43,boost);
   else if(move==1)spawn_fx(e,index,CFE_GERMINACAO_ESPINHOS,CFX_AREA,x,z,a,22,0,1.3f,2.2f,D,0);
   else spawn_fx(e,index,CFE_GERMINACAO_ESPINHOS,CFX_RING,e->x,e->z,a,9,67,1.5f,2.2f,D,0);
  } else {
   if(move==0)melee(e,53,boost);
   else if(move==1)for(int k=-1;k<=1;k++)
    spawn_fx(e,index,CFE_MANDIBULA_RAIZ,CFX_AREA,x+k*26*cosf(a+PI/2),z+k*26*sinf(a+PI/2),a,15,0,1.0f+fabsf((float)k)*.25f,1.1f,D,0);
   else for(int k=0;k<3;k++) {
    float q=a+k*2*PI/3;
    spawn_fx(e,index,CFE_FLORACAO_NEGRA,CFX_AREA,e->x+cosf(q)*66,e->z+sinf(q)*66,q,24,0,1.5f,3,D*3/4,0);
   }
  }
 } else if(camp==2) {
  if(role==1) {
   if(move==0)melee(e,27,boost);
   else {Enemy *all=enemies_get_array();for(int k=0;k<MAX_ENEMIES;k++)
    if(all[k].active && all[k].hp>0 && all[k].state!=ENEMY_STATE_DEAD && vec2_dist_sq(e->x,e->z,all[k].x,all[k].z)<110*110)
     readiness[k]=4.0f;}
  } else if(role==2) {
   if(move==0)spawn_fx(e,index,CFE_ESTILHACO_GEADA,CFX_PROJECTILE,e->x,e->z,a,7,0,0,2.4f,D,110);
   else for(int k=-1;k<=1;k+=2)
    spawn_fx(e,index,CFE_REFLEXO_GELO,CFX_DECOY,e->x+k*30*cosf(a+PI/2),e->z+k*30*sinf(a+PI/2),a,8,0,.35f,5,0,0);
  } else if(role==3) {
   if(move==0)melee(e,44,boost);
   else if(move==1)spawn_fx(e,index,CFE_ESPADA_VOADORA,CFX_PROJECTILE,e->x,e->z,a,9,0,.1f,1.2f,D,140);
   else for(int k=-1;k<=1;k++)
    spawn_fx(e,index,CFE_RUNA_JULGAMENTO,CFX_AREA,x+k*34,z+(k==0?25:0),a,13,0,1.1f+fabsf((float)k)*.3f,1.0f,D,0);
  } else {
   if(move==0)melee(e,52,boost);
   else if(move==1)for(int k=-1;k<=1;k++)
    spawn_fx(e,index,CFE_ESTILHACO_GEADA,CFX_PROJECTILE,e->x,e->z,a+k*.28f,7,0,.3f,2.5f,D*3/4,120);
   else spawn_fx(e,index,CFE_GEADA_SOLO,CFX_AREA,x,z,a,65,0,1.6f,4.5f,0,0);
  }
 } else if(camp==3) {
  if(role==2){melee(e,move?35:27,move?boost*1.4f:boost);return;}
  if(role==1) {
   if(move==0)melee(e,34,boost);
   else for(int k=-1;k<=1;k++)
    spawn_fx(e,index,CFE_ESCARAVELHO,CFX_PROJECTILE,e->x,e->z,a+k*.22f,7,0,.2f,3,D*3/4,70);
  } else if(role==3) {
   if(move==0)melee(e,44,boost);
   else if(move==1)for(int k=-1;k<=1;k+=2)
    spawn_fx(e,index,CFE_CORRENTE_DINASTIA,CFX_LINE,e->x+k*22*cosf(a+PI/2),e->z+k*22*sinf(a+PI/2),a,8,130,1.2f,.6f,D,0);
   else if(e->lunge_hit)spawn_fx(e,index,CFE_CORRENTE_DINASTIA,CFX_PROJECTILE,e->x,e->z,a,8,0,.45f,1.8f,D,125);
  } else if(role==4) {
   if(move==0)melee(e,50,boost);
   else if(move==1)for(int k=0;k<4;k++)
    spawn_fx(e,index,CFE_RAIO_ECLIPSE,CFX_LINE,e->x,e->z,a+k*PI/2,7,145,1.5f,.65f,D,0);
   else spawn_fx(e,index,CFE_MANDALA_VAZIO,CFX_SECTORS,e->x,e->z,a,87,0,1.8f,.9f,D,0);
  }
 } else if(camp==4) {
  if(role==1) {
   if(move==0)melee(e,31,boost);
   else {
    CampaignFX cut={.x=e->x,.z=e->z,.dx=cosf(a),.dz=sinf(a),.kind=CFX_LINE,.radius=6+g_druid.radius,.length=48};
    if(campaign_fx_contains(&cut,g_druid.x,g_druid.z))damage_player(D*13/10,e->x,e->z);
   }
  } else if(role==2) {
   if(move==0)spawn_fx(e,index,CFE_AGULHA_LUTO,CFX_PROJECTILE,e->x,e->z,a,5,0,0,2.2f,D,125);
   else spawn_fx(e,index,CFE_COSTURA_MALDITA,CFX_AREA,x,z,a,23,0,1.25f,1.3f,0,0);
  } else if(role==3) {
   if(move==0)melee(e,40,boost);
   else if(move==1)for(int k=-1;k<=1;k++)
    spawn_fx(e,index,CFE_GOTA_RUBRA,CFX_PROJECTILE,e->x+k*14*cosf(a+PI/2),e->z+k*14*sinf(a+PI/2),a+k*.12f,7,0,.9f,2.5f,D*3/4,100);
   else spawn_fx(e,index,CFE_VEU_LAMINA,CFX_CONE,e->x,e->z,a,58,0,.65f,.6f,D,0);
  } else {
   if(move==0)melee(e,52,boost);
   else if(move==1)spawn_fx(e,index,CFE_LAMENTO_MORTO,CFX_CONE,e->x,e->z,a,108,0,1.2f,.65f,D/2,0);
   else for(int k=-1;k<=1;k++)
    spawn_fx(e,index,CFE_TRAPO_ERRANTE,CFX_PROJECTILE,e->x,e->z,a+k*.38f,11,0,.8f,2.8f,D*3/4,65);
  }
 }
}
bool campaign_enemy_update(Enemy *e,int index,float dt,float distance,float dx,float dz) {
 int camp=enemy_campaign(e);
 if(!campaign_enemy_enabled(e))return false;
 int role=campaign_enemy_role(e);bool boss=role>=3,ranged=role==2 && camp!=3;EnemyBalance profile=enemy_profile(e);
 if(e->morph_timer>0) {
  e->morph_timer=fmaxf(0,e->morph_timer-dt);e->vx=e->vz=0;
  if(e->morph_timer<=0) {e->state=ENEMY_STATE_CHASE;e->boss_move=0;}
  return true;
 }
 if(g_druid.hp<=0 || (!boss && g_druid.camo_timer>0)) {e->state=ENEMY_STATE_IDLE;e->vx=e->vz=0;return true;}
 switch(e->state) {
 case ENEMY_STATE_IDLE:
  e->vx=e->vz=0;if(distance<160)e->state=ENEMY_STATE_CHASE;break;
 case ENEMY_STATE_HURT:
 case ENEMY_STATE_CHASE: {
  e->state=ENEMY_STATE_CHASE;e->yaw=atan2f(dz,dx);e->anim_timer+=dt*10;
  float reach=boss?46:role==1?29:24;
  bool special=role>0 && e->boss_special_cooldown<=0 && distance<(camp==3 && role==2?35:boss?180:145);
  bool normal=e->attack_cooldown<=0 && distance<(ranged?145:reach);
  if(e->attack_cooldown<=0 && (special || normal)) {
   e->boss_move=special?(boss && e->variant!=EV_MINIBOSS?1+((e->combo_step++)&1):1):0;
   float windup=e->boss_move?(boss?1.3f:1.1f):enemy_attack_windup(profile.attack);
   if(camp==4 && role==1 && e->boss_move)windup=1.6f;
   if(role==1 && camp==1 && e->boss_move)windup=.6f;
   if(camp==3 && role==3 && e->boss_move==2)e->lunge_hit=false;
   if(windup<.30f)windup=.30f;
   e->telegraph_duration=windup;e->state_timer=windup;e->state=ENEMY_STATE_TELEGRAPH;
   e->lunge_vx=g_druid.x;e->lunge_vz=g_druid.z;e->vx=e->vz=0;
   if(special)e->boss_special_cooldown=boss?7.0f:6.0f;
   break;
  }
  float speed=enemy_move_speed(profile.movement)*(feral_rush[index]>0?1.5f:1);
  float movement=ranged?(distance<48?-1:distance>88?1:0):(distance>reach?1:0);
  e->vx+=dx*speed*e->speed_mult*movement*dt*8;e->vz+=dz*speed*e->speed_mult*movement*dt*8;
  break;
 }
 case ENEMY_STATE_TELEGRAPH:
  e->vx=e->vz=0;e->state_timer-=dt;
  if(e->state_timer>.3f && e->boss_move==0)e->yaw=atan2f(dz,dx);
  if(e->state_timer<=0) {
   e->state=ENEMY_STATE_ATTACK;e->state_timer=(camp==1 && role==1 && e->boss_move)?2.0f:.3f;
   sound_play(e->boss_move?SFX_CULTIST_CAST:role==0?SFX_SKELETON_SWING:SFX_DEMON_SWING);
   execute(e,index);
  }
  break;
 case ENEMY_STATE_ATTACK:
  e->vx=e->vz=0;e->state_timer-=dt;
  if(e->state_timer<=0) {
   e->state=ENEMY_STATE_RECOVERY;e->state_timer=enemy_attack_recovery(profile.attack);
   if(e->boss_move && boss && e->state_timer<.95f)e->state_timer=.95f;
   if(camp==2 && role==3 && e->boss_move==1)e->state_timer=1.5f;
   e->attack_cooldown=enemy_attack_cooldown(profile.attack);
  }
  break;
 case ENEMY_STATE_RECOVERY:
  e->vx=e->vz=0;e->state_timer-=dt;
  if(e->state_timer<=0)e->state=ENEMY_STATE_CHASE;
  break;
 default:break;
 }
 return true;
}

