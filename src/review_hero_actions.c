// Connect accepted animations/effects to the existing player and projectile pools.
#include "review_hero_actions.h"
#include "druid.h"
#include "hero_rules.h"
#include "spells.h"
#include "enemies.h"
#include "world.h"
#include "audio.h"
#include "hud.h"
static HeroEffect fx[MAX_HERO_EFFECTS];
static int next_combo;
static float combo_left;
const HeroEffect *review_hero_effects(void){return fx;}
void review_hero_reset(void){memset(fx,0,sizeof(fx));next_combo=0;combo_left=0;}
static HeroEffect *effect(int kind,float seconds){
    int slot=0;
    for(int i=0;i<MAX_HERO_EFFECTS;i++)if(fx[i].life<=0 || fx[i].kind==kind){slot=i;break;}
    fx[slot]=(HeroEffect){g_druid.x,g_druid.z,seconds,seconds,0,(unsigned char)kind};
    return &fx[slot];
}
bool review_hero_in_smoke(void){
    if(g_hero!=HERO_ROGUE)return false;
    for(int i=0;i<MAX_HERO_EFFECTS;i++)if(fx[i].life>0 && fx[i].kind==HERO_FX_SMOKE &&
        vec2_dist_sq(g_druid.x,g_druid.z,fx[i].x,fx[i].z)<42*42)return true;
    return false;
}
static bool spend(float amount){if(g_druid.spirit<amount)return false;g_druid.spirit-=amount;return true;}
static void cast(int visual,float seconds){
    g_druid.visual_action=visual;g_druid.visual_duration=g_druid.visual_timer=seconds;
    g_druid.state=DRUID_STATE_CAST;g_druid.cast_timer=seconds;
}
void review_druid_stag(void){
    if(g_druid.is_stag){g_druid.is_stag=false;g_druid.is_beast=false;return;}
    if(!spend(25))return;
    g_druid.is_beast=g_druid.is_stag=true;g_druid.beast_timer=20;
    g_druid.transform_timer=.72f;sound_play(SFX_SHAPESHIFT);
    hud_show_message("Forma de cervo",1.5f);
}
void review_druid_buff(void){
    if(g_druid.focus_timer>0 || !spend(25))return;
    g_druid.focus_timer=6;
    effect(g_druid.is_stag?HERO_FX_STAG:g_druid.is_beast?HERO_FX_WOLF:HERO_FX_CLAWS,1.0f);
}
void review_hero_update(float dt){
    combo_left=fmaxf(0,combo_left-dt);if(combo_left<=0)next_combo=0;
    for(int i=0;i<MAX_HERO_EFFECTS;i++)if(fx[i].life>0){
        HeroEffect *f=&fx[i];f->life=fmaxf(0,f->life-dt);f->tick-=dt;
        if(f->kind==HERO_FX_DRAIN || f->kind==HERO_FX_DARKNESS){f->x=g_druid.x;f->z=g_druid.z;}
        if(f->kind==HERO_FX_BARRIER && f->tick<=0){enemies_root_at(f->x,f->z,48,.18f);f->tick=.35f;}
        if(f->kind==HERO_FX_WELL && f->tick<=0 && f->tick>-10 && f->life>0){
            // One impact after the casting pose, never one damage hit per frame.
            enemies_damage_at(f->x,f->z,32,g_druid.attack_damage*2,0,0);
            sound_play(SFX_SPIRIT_BOLT_HIT);f->tick=-1000;
        }
    }
}
void review_hero_buttons(joypad_buttons_t *p,float input_mag){
    if(g_druid.state==DRUID_STATE_ATTACK || g_druid.state==DRUID_STATE_DODGE || g_druid.state==DRUID_STATE_CAST)return;
    if(input_mag>.15f)g_druid.yaw=g_druid.target_yaw;
    float dx=cosf(g_druid.yaw),dz=sinf(g_druid.yaw);
    if(p->a){
        g_druid.state=DRUID_STATE_ATTACK;g_druid.attack_hit_done=false;
        g_druid.combo=next_combo;
        if(g_hero==HERO_ROGUE){next_combo=(next_combo+1)%3;combo_left=1.5f;g_druid.camo_timer=0;}
        g_druid.attack_timer=g_druid.attack_duration=g_hero==HERO_ROGUE?.48f:.60f;
        sound_play(SFX_SWING);return;
    }
    if(g_hero==HERO_ROGUE){
        if(p->b && spend(18)){effect(HERO_FX_SMOKE,5);cast(HERO_VIS_SMOKE,.6f);}
        else if(p->c_left && spend(20)){g_druid.camo_timer=5;hud_show_message("Invisibilidade",1);}
        else if(p->c_up && spend(25)){
            g_druid.x+=dx*36;g_druid.z+=dz*36;world_collide_circle(&g_druid.x,&g_druid.z,g_druid.radius);
            enemies_damage_at(g_druid.x,g_druid.z,34,g_druid.attack_damage*2,dx,dz);cast(HERO_VIS_SPIN,.65f);
        } else if(p->c_right && spend(15)){
            spells_cast_knife(g_druid.x+dx*8,g_druid.z+dz*8,dx,dz,g_druid.attack_damage*2);cast(HERO_VIS_KNIFE,.55f);
        }
    }else if(g_hero==HERO_MAGE){
        // Approved casting poses are selected by the visual action.
        if(p->b && spend(15)){
            // Keep the basic ranged orb separate from the ground-targeted well.
            float tx=g_druid.x+dx*64,tz=g_druid.z+dz*64,best=128*128;
            const Enemy *all=enemies_get_array();
            for(int i=0;i<MAX_ENEMIES;i++){
                const Enemy *e=&all[i];if(!e->active || e->hp<=0 || e->state==ENEMY_STATE_DEAD)continue;
                float ex=e->x-g_druid.x,ez=e->z-g_druid.z,d=ex*ex+ez*ez;
                if(d<best && ex*dx+ez*dz>=0){best=d;tx=e->x;tz=e->z;}
            }
            // Stop at terrain instead of casting through an intervening wall.
            float wx=g_druid.x,wz=g_druid.z;
            for(int step=1;step<=16;step++){
                float x=g_druid.x+(tx-g_druid.x)*step/16,z=g_druid.z+(tz-g_druid.z)*step/16;
                if(world_collide_circle(&x,&z,1))break;
                wx=x;wz=z;
            }
            HeroEffect *well=effect(HERO_FX_WELL,1.52f);well->x=wx;well->z=wz;well->tick=.5f;
            cast(HERO_VIS_WELL,.5f);
        }else if(p->c_left && spend(25)){
            Enemy *all=enemies_get_array();int drain=0;
            for(int i=0;i<MAX_ENEMIES;i++)if(all[i].active && all[i].hp>0 && all[i].state!=ENEMY_STATE_DEAD &&
                vec2_dist_sq(all[i].x,all[i].z,g_druid.x,g_druid.z)<80*80)drain++;
            enemies_damage_at(g_druid.x,g_druid.z,80,g_druid.attack_damage/2,0,0);
            druid_heal(drain*3);effect(HERO_FX_DRAIN,1.02f);cast(HERO_VIS_DRAIN,.6f);
        }else if(p->c_up && spend(30)){effect(HERO_FX_BARRIER,4);cast(HERO_VIS_BARRIER,.6f);}
        else if(p->c_right && spend(35)){g_druid.focus_timer=6;effect(HERO_FX_DARKNESS,6);cast(HERO_VIS_DARKNESS,.7f);}
    }
}
