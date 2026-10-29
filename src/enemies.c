#include "gfx.h"
#include "enemies.h"
#include "cards.h"
#include "lang.h"
#include "sprites.h"
#include "models.h"
#include "audio.h"
#include "druid.h"
#include "spells.h"
#include "world.h"
#include "items.h"
#include "hud.h"
#include "demon_combat.h"
#include "cultist_combat.h"
#include "malakor_combat.h"
#include "varkhul_combat.h"
#include "campaign_combat.h"

static Enemy s_enemies[MAX_ENEMIES];
static int s_boss_idx = -1;
static EnemyHealEffect s_heal_fx[MAX_ENEMY_HEAL_EFFECTS];
static VarkhulCombatFX s_varkhul_fx;
const VarkhulCombatFX *enemies_get_varkhul_fx(void) {return &s_varkhul_fx;}

void enemies_init(void) {
    enemies_clear();
}

void enemies_clear(void) {
    campaign_combat_clear();
    memset(&s_varkhul_fx,0,sizeof(s_varkhul_fx));
    for (int i = 0; i < MAX_ENEMIES; i++) {
        s_enemies[i].active = false;
    }
    s_boss_idx = -1;
    memset(s_heal_fx,0,sizeof(s_heal_fx));
}

float g_enemy_power = 1.0f;
float g_enemy_aggro = 1.0f;
float g_hit_poison = 0.0f;

// Dificuldade: vida e dano dos monstros (Normal, Pesadelo, Inferno)
static const float DIFF_HP[DIFF_COUNT]  = { 1.0f, 1.6f, 2.5f };
static const float DIFF_DMG[DIFF_COUNT] = { 1.0f, 1.4f, 1.9f };
static const int DIFF_ELITE_MULT[DIFF_COUNT] = { 2, 3, 4 };

static int cemetery_boss_review_hp(int normal_hp) {
    if (VEIL_LEVEL_REVIEW && VEIL_REVIEW_WEAK_CEMETERY_BOSS && g_campaign==0)
        return VEIL_REVIEW_CEMETERY_BOSS_HP;
    return normal_hp;
}

void enemies_set_affixes(int idx, int count) {
    if (idx < 0 || idx >= MAX_ENEMIES) return;
    Enemy *e = &s_enemies[idx];
    for (int k = 0; k < count && k < 3; k++) {
        uint8_t pick;
        do { pick = (uint8_t)(1u << (rand() % 3)); } while (e->affix & pick);
        e->affix |= pick;
        if (pick == AFX_FAST) { e->speed_mult *= 1.08f; } // stays within its approved movement category
    }
}

void enemies_root_at(float x, float z, float r, float time) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &s_enemies[i];
        if (!e->active || e->state == ENEMY_STATE_DEAD) continue;
        if (vec2_dist_sq(x, z, e->x, e->z) > (r + e->radius) * (r + e->radius)) continue;
        e->root_timer = e->type == ENEMY_TYPE_BOSS_MALAKOR ? time * 0.35f : time;   // chefe se solta rapido
    }
}

int enemies_spawn(EnemyType type, float x, float z) {
    return enemies_spawn_ex(type, x, z, 0, false);
}

// Acorda todos os integrantes parados do mesmo grupo.
static void alert_pack(int pack) {
    if (pack <= 0) return;
    for (int j = 0; j < MAX_ENEMIES; j++) {
        Enemy *o = &s_enemies[j];
        if (o->active && o->pack == pack && o->state == ENEMY_STATE_IDLE) {
            o->state = ENEMY_STATE_CHASE;
        }
    }
}

void enemies_force_chase(int idx) {
    if (idx < 0 || idx >= MAX_ENEMIES || !s_enemies[idx].active) return;
    s_enemies[idx].state = ENEMY_STATE_CHASE;
}

int enemies_count_elites(void) {
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (s_enemies[i].active && s_enemies[i].elite && s_enemies[i].state != ENEMY_STATE_DEAD) n++;
    }
    return n;
}

int enemy_campaign(const Enemy *e) {return e->origin_campaign<5?e->origin_campaign:0;}
EnemyBalance enemy_profile(const Enemy *e) {
    int role=e->variant==EV_PRIEST?5:e->type==ENEMY_TYPE_BOSS_MALAKOR?(e->boss_phase==2?4:3):(int)e->type;
    EnemyBalance profile=enemy_balance(enemy_campaign(e),role);
    if(e->variant==EV_COURIER){profile.movement=EB_FAST;profile.attack=EB_NORMAL;}
    return profile;
}
static EnemyType pack_balanced_type(EnemyType type,int pack,bool elite) {
    if(type==ENEMY_TYPE_BOSS_MALAKOR)return type;
    int strong=0,support=0,basic=0,total_strong=0,total_support=0;
    for(int i=0;i<MAX_ENEMIES;i++) {
        const Enemy *o=&s_enemies[i];
        if(!o->active || o->hp<=0 || o->state==ENEMY_STATE_DEAD || o->variant!=EV_STANDARD)continue;
        if(o->type==ENEMY_TYPE_DEMON){total_strong++;if(o->pack==pack)strong++;}
        else if(o->type==ENEMY_TYPE_CULTIST){total_support++;if(o->pack==pack)support++;}
        else if(o->type==ENEMY_TYPE_SKELETON && o->pack==pack)basic++;
    }
    // A lone elite leader may be placed before its basic escort. Later members
    // restore the majority; ordinary packs cannot start with repeated heavies.
    bool leader=elite && basic+strong+support==0;
    bool majority=!leader && strong+support+1>=basic;
    if(type==ENEMY_TYPE_DEMON && (majority || strong>=PACK_STRONG_LIMIT || total_strong>=ACTIVE_STRONG_LIMIT))return ENEMY_TYPE_SKELETON;
    if(type==ENEMY_TYPE_CULTIST && (majority || support>=PACK_SUPPORT_LIMIT || total_support>=ACTIVE_SUPPORT_LIMIT))return ENEMY_TYPE_SKELETON;
    return type;
}
static int spawn_config(EnemyType type,float x,float z,int pack,bool elite,int variant,int origin,int phase) {
    if(VEIL_LEVEL_REVIEW && VEIL_REVIEW_HIDE_MONSTERS && type!=ENEMY_TYPE_BOSS_MALAKOR)return -1;
    if(variant==EV_STANDARD)type=pack_balanced_type(type,pack,elite);
    for(int i=0;i<MAX_ENEMIES;i++)if(!s_enemies[i].active) {
        for(int f=0;f<MAX_ENEMY_HEAL_EFFECTS;f++)if(s_heal_fx[f].target==i)s_heal_fx[f].life=0;
        Enemy *e=&s_enemies[i];memset(e,0,sizeof(*e));campaign_enemy_spawned(i);
        e->active=true;e->type=type;e->origin_campaign=origin;e->variant=variant;
        e->boss_phase=type==ENEMY_TYPE_BOSS_MALAKOR?phase:0;
        e->state=ENEMY_STATE_IDLE;e->x=x;e->z=z;e->pack=pack;e->yaw=(rand()%628)/100.0f;
        EnemyBalance b=enemy_profile(e);e->hp=e->max_hp=b.hp;e->damage=b.damage;
        e->radius=type==ENEMY_TYPE_BOSS_MALAKOR?26:type==ENEMY_TYPE_DEMON?10:type==ENEMY_TYPE_CULTIST?6.5f:7;
        e->exp_reward=type==ENEMY_TYPE_BOSS_MALAKOR?350:type==ENEMY_TYPE_DEMON?65:type==ENEMY_TYPE_CULTIST?45:30;
        e->telegraph_duration=enemy_attack_windup(b.attack);e->speed_mult=1;
        e->boss_special_cooldown=type==ENEMY_TYPE_BOSS_MALAKOR?4:3.5f+(i%3)*.7f;
        if(type==ENEMY_TYPE_CULTIST){e->combo_step=-1;e->lunge_vx=3;}
        if(type!=ENEMY_TYPE_BOSS_MALAKOR && g_enemy_power>1.001f) {
            e->max_hp=(int)(e->max_hp*g_enemy_power);
            e->damage=(int)(e->damage*(1+(g_enemy_power-1)*.8f));
            e->exp_reward=(int)(e->exp_reward*(1+(g_enemy_power-1)*.5f));
        }
        int d=g_difficulty<0?0:g_difficulty>=DIFF_COUNT?DIFF_COUNT-1:g_difficulty;
        e->max_hp=(int)(e->max_hp*DIFF_HP[d]);e->damage=(int)(e->damage*DIFF_DMG[d]);
        if(variant==EV_MINIBOSS){e->max_hp=e->max_hp*30/100;e->damage=e->damage*60/100;e->radius*=.70f;}
        else if(variant==EV_THRONE || variant==EV_PRIEST){e->max_hp*=variant==EV_PRIEST?5:4;e->damage=e->damage*125/100;e->exp_reward*=3;}
        else if(variant==EV_CHEST){e->max_hp*=4;e->damage=e->damage*120/100;e->exp_reward*=3;}
        else if(variant==EV_CHAMPION){e->max_hp*=3;e->damage=e->damage*130/100;e->exp_reward*=3;}
        else if(variant==EV_COURIER)e->max_hp=e->max_hp*3/2;
        e->elite=elite && variant==EV_STANDARD;
        if(e->elite){int m=DIFF_ELITE_MULT[d];e->max_hp*=m;e->damage*=m;e->radius*=1.2f;e->exp_reward*=4;if(d>0)enemies_set_affixes(i,d);}
        if(variant==EV_PRIEST)e->boss_special_cooldown=3600;
        if(type==ENEMY_TYPE_BOSS_MALAKOR && variant!=EV_MINIBOSS){e->max_hp=cemetery_boss_review_hp(e->max_hp);s_boss_idx=i;}
        e->hp=e->max_hp;return i;
    }
    return -1;
}
int enemies_spawn_ex(EnemyType type,float x,float z,int pack,bool elite) {return spawn_config(type,x,z,pack,elite,EV_STANDARD,g_campaign,1);}
int enemies_spawn_variant(EnemyType type,float x,float z,int pack,int variant) {return spawn_config(type,x,z,pack,false,variant,g_campaign,1);}
int enemies_spawn_miniboss(int campaign,int phase,float x,float z,int pack) {
    if(campaign<0 || campaign>=5 || (phase!=1 && phase!=2))return -1;
    return spawn_config(ENEMY_TYPE_BOSS_MALAKOR,x,z,pack,false,EV_MINIBOSS,campaign,phase);
}


bool enemies_damage_at(float x, float z, float radius, int damage, float knockback_x, float knockback_z) {
    bool hit_any = world_event_damage_targets(x,z,radius,damage);
    campaign_hit_decoys(x,z,radius);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &s_enemies[i];
        if (!e->active || e->state == ENEMY_STATE_DEAD) continue;

        float d2 = vec2_dist_sq(x, z, e->x, e->z);
        float combined_r = radius + e->radius;

        if (d2 <= (combined_r * combined_r)) {
            hit_any = true;
            if (e->type == ENEMY_TYPE_BOSS_MALAKOR && e->morph_timer > 0.0f) continue;   // transformando
            if (g_campaign==0 && e->type==ENEMY_TYPE_BOSS_MALAKOR && e->boss_phase==2 && e->y>0.0f) continue;
            e->hp -= campaign_incoming_damage(e,damage,x,z);
            if(e->hp<=0 && (e->type!=ENEMY_TYPE_BOSS_MALAKOR || e->variant==EV_MINIBOSS))world_event_enemy_defeated(i);
            if (g_hit_poison > 0.0f) { e->poison_t = 3.0f; e->poison_dps = g_hit_poison; }
            if (e->type == ENEMY_TYPE_BOSS_MALAKOR && e->variant!=EV_MINIBOSS && e->boss_phase == 1 && e->hp <= 0) {
                // Malakor cai... e se ergue como Varkhul, com uma barra de vida nova.
                e->boss_phase = 2;
                e->max_hp = cemetery_boss_review_hp((int)(enemy_profile(e).hp * DIFF_HP[g_difficulty]));
                e->hp = e->max_hp;
                e->damage = (int)(enemy_profile(e).damage * DIFF_DMG[g_difficulty]);
                e->morph_timer = 1.8f;
                e->state = ENEMY_STATE_RECOVERY;
                e->state_timer = 1.8f;
                e->combo_step = 0;
                e->boss_special_cooldown = 2.5f;
                if (g_campaign==0) {
                    memset(&s_varkhul_fx,0,sizeof(s_varkhul_fx));
                    e->boss_move=0; e->y=0; e->vx=e->vz=0;
                }
                sound_play(SFX_BOSS_ENRAGE);
                hud_show_banner(T(g_campaign==3?S_BOSS_RAGE_D:g_campaign==4?S_BOSS_RAGE_V:g_campaign == 2 ? S_BOSS_RAGE_TN : g_campaign == 1 ? S_BOSS_RAGE_TF : S_BOSS_RAGE_T), campaign_boss_name(g_campaign,true), 2.5f);
                continue;
            }
            e->hurt_timer = 0.20f;
            static const SoundEffect hurt_sfx[] = {
                SFX_SKELETON_HURT, SFX_DEMON_HURT, SFX_CULTIST_HURT, SFX_BOSS_HURT
            };
            sound_play(hurt_sfx[e->type]);

            // Knockback (Boss resists heavy knockback)
            float kb_factor = e->type==ENEMY_TYPE_BOSS_MALAKOR?0.2f:
                              e->type==ENEMY_TYPE_DEMON?0.30f:1.0f;
            e->vx += knockback_x * 40.0f * kb_factor;
            e->vz += knockback_z * 40.0f * kb_factor;

            if (e->hp <= 0) {
                e->hp = 0;
                e->state = ENEMY_STATE_DEAD;
                e->state_timer = 1.5f; // Death fade timer
                static const SoundEffect death_sfx[] = {
                    SFX_SKELETON_DIE, SFX_DEMON_DIE, SFX_CULTIST_DIE, SFX_BOSS_DIE
                };
                sound_play(death_sfx[e->type]);
                druid_gain_exp(e->exp_reward);

                const char *type_names[] = { "Skeleton", "Demon", "Cultist", "BOSS MALAKOR" };
                debugf("[ENEMY] %s defeated! Exp: +%d\n", type_names[e->type], e->exp_reward);

                // Loot: hordas grandes => drops raros (as pocoes voltam a ter valor).
                if (e->type != ENEMY_TYPE_BOSS_MALAKOR) {
                    int roll = rand() % 100;
                    if (roll < 60) items_spawn_drop(DROP_SOUL_ORB, e->x, e->z);
                    static const int ESS_CH[DIFF_COUNT] = { 30, 45, 60 }, POT_CH[DIFF_COUNT] = { 5, 4, 3 };
                    if (rand() % 100 < ESS_CH[g_difficulty]) items_spawn_drop(DROP_ESSENCE, e->x + 4.0f, e->z - 4.0f);   // 3 de essencia
                    roll = rand() % 100;
                    if (roll < POT_CH[g_difficulty])          items_spawn_drop(DROP_HEALTH_POTION, e->x + 5.0f, e->z);
                    else if (roll < POT_CH[g_difficulty] + 6) items_spawn_drop(DROP_SPIRIT_VIAL, e->x - 5.0f, e->z);
                }

                // Guardiao (elite = chefe da area): 2 orbes, 1 pocao, 50% carta com raridade melhor
                // que a do bau e 25% de chance de um continue.
                if (e->elite) {
                    items_spawn_drop(DROP_HEALTH_POTION, e->x + 7.0f, e->z + 3.0f);
                    items_spawn_drop(DROP_SOUL_ORB, e->x - 7.0f, e->z + 3.0f);
                    items_spawn_drop(DROP_SOUL_ORB, e->x, e->z - 8.0f);
                    if (rand() % 100 < 8) items_spawn_drop(DROP_CONTINUE, e->x, e->z + 12.0f);
                    for (int k = 0; k < 5; k++) items_spawn_drop(DROP_ESSENCE, e->x + (k - 2) * 8.0f, e->z - 10.0f);
                    if (g_difficulty == DIFF_HELL)           cards_grant_weighted(35, 38, 18, 9);   // Inferno: lendarias de verdade
                    else if (g_difficulty == DIFF_NIGHTMARE) cards_grant_weighted(48, 36, 8, 8);
                    else                                      cards_grant_weighted(60, 28, 5, 7);   // guardiao: carta garantida
                    if (e->affix & AFX_EXPLODE) {               // explosivo: estoura num anel de orbes ao morrer
                        for (int a = 0; a < 8; a++) spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, cosf(a * 0.785f), sinf(a * 0.785f), e->damage / 2);
                    }
                }

                if (e->type == ENEMY_TYPE_BOSS_MALAKOR && e->variant!=EV_MINIBOSS) {
                    // Chefe final: carta garantida, quase sempre rara ou lendaria.
                    cards_grant_weighted(0, 40, 50, 10);
                    hud_show_banner(T(g_campaign==3?S_BOSS_DEAD_D:g_campaign==4?S_BOSS_DEAD_V:g_campaign == 2 ? S_BOSS_DEAD_TN : g_campaign == 1 ? S_BOSS_DEAD_TF : S_BOSS_DEAD_T), T(S_BOSS_DEAD_S), 3.5f);
                }
            } else {
                // If damaged while idle, alert to player immediately
                if (e->state == ENEMY_STATE_IDLE) {
                    e->state = ENEMY_STATE_CHASE;
                    alert_pack(e->pack);
                }
            }
        }
    }

    return hit_any;
}

int enemies_find_closest(float x, float z, float max_dist) {
    int best_idx = -1;
    float best_dist_sq = max_dist * max_dist;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &s_enemies[i];
        if (!e->active || e->state == ENEMY_STATE_DEAD) continue;

        float d2 = vec2_dist_sq(x, z, e->x, e->z);
        if (d2 < best_dist_sq) {
            best_dist_sq = d2;
            best_idx = i;
        }
    }

    return best_idx;
}

// ---- Chefe final (estilo souls): golpes telegrafados, combos, investida e recuperacoes punitivas ----
enum { BM_SLAM, BM_COMBO, BM_LUNGE, BM_NOVA, BM_SWEEP, BM_GRAB };
static const float DIFF_TF[DIFF_COUNT] = { 1.0f, 0.9f, 0.8f };   // preparacoes do chefe mais curtas

static void boss_ring(Enemy *e, int n, float offset, int dmg) {
    for (int a = 0; a < n; a++) {
        float ang = offset + (float)a * (6.2831853f / (float)n);
        spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, cosf(ang), sinf(ang), dmg);
    }
}

static void boss_pick(Enemy *e, float dist) {
    bool p2 = e->boss_phase == 2;
    float tf = (p2 ? 0.78f : 1.0f) * DIFF_TF[g_difficulty];   // fase 2: preparacoes mais curtas
    e->state = ENEMY_STATE_TELEGRAPH;
    e->combo_step = 0;
    if (e->boss_special_cooldown <= 0.0f) {
        e->boss_move = BM_NOVA; e->state_timer = 0.95f * tf;
        e->boss_special_cooldown = p2 ? 6.5f : 8.5f;
    } else if (p2 && g_campaign == 1 && g_seeds > 0 && dist < 75.0f && rand() % 100 < 28) {
        e->boss_move = BM_GRAB; e->state_timer = 1.0f * tf;      // Mae-Raiz: tenta agarrar (e devorar uma semente)
    } else if (dist > 80.0f) {
        e->boss_move = BM_LUNGE; e->state_timer = 0.75f * tf;
    } else {
        int r = rand() % 100;
        if (p2 && r < 35)            { e->boss_move = BM_SWEEP; e->state_timer = 0.65f * tf; }
        else if (r < (p2 ? 75 : 60)) { e->boss_move = BM_COMBO; e->combo_step = p2 ? 4 : 3; e->state_timer = 0.6f * tf; }
        else                         { e->boss_move = BM_SLAM;  e->state_timer = 0.9f * tf; }
    }
}

// Malakor's approved three-move first form. No dash, combo lunge or orb rings.
static void boss_hit_player(int damage,float x,float z) {
    int cap=g_druid.max_hp*35/100;if(cap<1)cap=1;
    druid_take_damage_capped(damage>cap?cap:damage,x,z,cap);
}
static void malakor_begin(Enemy *e, int move, float windup) {
    e->boss_move=move;
    e->telegraph_duration=windup;
    e->state_timer=e->telegraph_duration;
    e->state=ENEMY_STATE_TELEGRAPH;
    e->vx=e->vz=0.0f;
}
static void malakor_pull_player(Enemy *e, float dt, float dist, float dx, float dz) {
    if (g_druid.hp<=0 || g_druid.state==DRUID_STATE_DODGE || g_druid.leap_timer>0.0f ||
        dist>MALAKOR_PULL_RANGE) return;
    float stop=e->radius+g_druid.radius+3.0f;
    if (dist<=stop) return;
    float step=fminf(MALAKOR_PULL_SPEED*dt,dist-stop);
    g_druid.x-=dx*step;g_druid.z-=dz*step;
    // Input stays active; walls and escape skills continue to work.
    world_collide_circle(&g_druid.x,&g_druid.z,g_druid.radius);
}
static void malakor_update(Enemy *e, float dt, float dist, float dx, float dz) {
    if (e->lunge_vx>0.0f)e->lunge_vx=fmaxf(0.0f,e->lunge_vx-dt); // suction cooldown, no dash
    if (g_druid.hp<=0) {e->state=ENEMY_STATE_IDLE;e->vx=e->vz=0.0f;return;}
    switch(e->state) {
        case ENEMY_STATE_IDLE:
            e->vx=e->vz=0.0f;
            if(dist<170.0f)e->state=ENEMY_STATE_CHASE;
            break;
        case ENEMY_STATE_HURT:
        case ENEMY_STATE_CHASE:
            e->state=ENEMY_STATE_CHASE;
            e->yaw=atan2f(dz,dx);
            e->anim_timer+=dt*10.0f;
            if(e->attack_cooldown<=0.0f) {
                if(e->boss_special_cooldown<=0.0f) {
                    if(e->combo_step==1 && e->lunge_vx<=0.0f && dist<=MALAKOR_PULL_RANGE) {
                        malakor_begin(e,MALAKOR_MOVE_PULL,MALAKOR_PULL_WINDUP);
                        e->lunge_vx=MALAKOR_PULL_COOLDOWN;e->combo_step=0;
                    } else {
                        malakor_begin(e,MALAKOR_MOVE_GROUND,MALAKOR_GROUND_WINDUP);
                        e->combo_step=1;
                    }
                    e->boss_special_cooldown=MALAKOR_SPECIAL_COOLDOWN;
                    break;
                }
                if(dist<MALAKOR_NORMAL_RANGE) {
                    malakor_begin(e,MALAKOR_MOVE_NORMAL,MALAKOR_NORMAL_WINDUP);
                    break;
                }
            }
            if(dist>32.0f) {e->vx+=dx*enemy_move_speed(enemy_profile(e).movement)*dt*8.0f;e->vz+=dz*enemy_move_speed(enemy_profile(e).movement)*dt*8.0f;}
            break;
        case ENEMY_STATE_TELEGRAPH:
            e->vx=e->vz=0.0f;
            e->state_timer-=dt;
            if(e->state_timer>MALAKOR_AIM_LOCK)e->yaw=atan2f(dz,dx);
            if(e->state_timer>0.0f)break;
            e->state=ENEMY_STATE_ATTACK;
            if(e->boss_move==MALAKOR_MOVE_NORMAL) {
                e->state_timer=MALAKOR_NORMAL_ACTIVE;
                sound_play(SFX_DEMON_SWING);
                float front=cosf(e->yaw)*dx+sinf(e->yaw)*dz;
                if(dist<MALAKOR_NORMAL_RANGE && front>.25f)boss_hit_player(e->damage,e->x,e->z);
            } else if(e->boss_move==MALAKOR_MOVE_GROUND) {
                e->state_timer=MALAKOR_GROUND_ACTIVE;
                sound_play(SFX_BOSS_SLAM);
                float x=cosf(e->yaw),z=sinf(e->yaw);
                float offset=e->radius+4.0f;
                // One traveling rock wave; no extra radial hit, no double damage.
                spells_cast_enemy_stone_wave(e->x+x*offset,1.0f,e->z+z*offset,x,z,e->damage*16/10);
            } else {
                e->state_timer=MALAKOR_PULL_ACTIVE;
                sound_play(SFX_BOSS_ENRAGE);
            }
            break;
        case ENEMY_STATE_ATTACK:
            e->vx=e->vz=0.0f;
            if(e->boss_move==MALAKOR_MOVE_PULL)malakor_pull_player(e,dt,dist,dx,dz);
            e->state_timer-=dt;
            if(e->state_timer>0.0f)break;
            e->state=ENEMY_STATE_RECOVERY;
            e->state_timer=e->boss_move==MALAKOR_MOVE_NORMAL?MALAKOR_NORMAL_RECOVERY:
                e->boss_move==MALAKOR_MOVE_GROUND?MALAKOR_GROUND_RECOVERY:MALAKOR_PULL_RECOVERY;
            e->attack_cooldown=enemy_attack_cooldown(enemy_profile(e).attack);
            break;
        case ENEMY_STATE_RECOVERY:
            e->vx=e->vz=0.0f;e->state_timer-=dt;
            if(e->state_timer<=0.0f)e->state=ENEMY_STATE_CHASE;
            break;
        default:break;
    }
}

// Campaign-zero second form: scythe, telegraphed aerial impact, timed rush.
static void varkhul_begin(Enemy *e, int move, float time) {
    e->boss_move=move; e->state=ENEMY_STATE_TELEGRAPH;
    e->state_timer=e->telegraph_duration=time; e->vx=e->vz=0;
}
static void varkhul_update(Enemy *e,float dt,float dist,float dx,float dz) {
    s_varkhul_fx.rush=fmaxf(0,s_varkhul_fx.rush-dt);
    if (e->morph_timer>0) {
        e->morph_timer=fmaxf(0,e->morph_timer-dt);
        e->vx=e->vz=0; e->y=0;
        if(e->morph_timer<=0) {e->state=ENEMY_STATE_CHASE;e->attack_cooldown=.5f;}
        return;
    }
    if (g_druid.hp<=0) {
        e->state=ENEMY_STATE_IDLE;e->vx=e->vz=0;e->y=0;s_varkhul_fx.rush=0;return;
    }
    switch(e->state) {
    case ENEMY_STATE_IDLE:
    case ENEMY_STATE_HURT:
    case ENEMY_STATE_CHASE:
        e->state=ENEMY_STATE_CHASE;e->y=0;e->yaw=atan2f(dz,dx);
        e->anim_timer+=dt*10*(s_varkhul_fx.rush>0?VARKHUL_RUSH_MULT:1);
        if(e->attack_cooldown<=0) {
            if(e->boss_special_cooldown<=0) {
                if(e->combo_step==0) {
                    s_varkhul_fx.x=g_druid.x;s_varkhul_fx.z=g_druid.z;
                    world_collide_circle(&s_varkhul_fx.x,&s_varkhul_fx.z,e->radius);
                    varkhul_begin(e,VARKHUL_DROP,VARKHUL_WARNING);
                    e->combo_step=1;e->boss_special_cooldown=8.0f;
                } else {
                    varkhul_begin(e,VARKHUL_RUSH,.45f);
                    e->combo_step=0;e->boss_special_cooldown=10.0f;
                }
                break;
            }
            if(dist<VARKHUL_SCYTHE_RANGE) {
                varkhul_begin(e,VARKHUL_SCYTHE,VARKHUL_SCYTHE_WINDUP);
                break;
            }
        }
        if(dist>34) {
            float speed=enemy_move_speed(enemy_profile(e).movement)*(s_varkhul_fx.rush>0?VARKHUL_RUSH_MULT:1);
            e->vx+=dx*speed*dt*8;e->vz+=dz*speed*dt*8;
        }
        break;
    case ENEMY_STATE_TELEGRAPH:
        e->vx=e->vz=0;e->state_timer-=dt;
        if(e->boss_move==VARKHUL_DROP) {
            if(e->state_timer<VARKHUL_WARNING-.35f) {
                e->x=s_varkhul_fx.x;e->z=s_varkhul_fx.z;e->y=VARKHUL_DROP_HEIGHT;
            }
        } else if(e->state_timer>.25f)e->yaw=atan2f(dz,dx);
        if(e->state_timer>0)break;
        if(e->boss_move==VARKHUL_RUSH) {
            s_varkhul_fx.rush=VARKHUL_RUSH_TIME;
            e->state=ENEMY_STATE_CHASE;e->attack_cooldown=.25f;
            sound_play(SFX_BOSS_ENRAGE);break;
        }
        e->state=ENEMY_STATE_ATTACK;
        if(e->boss_move==VARKHUL_DROP) {e->state_timer=VARKHUL_FALL;break;}
        e->state_timer=VARKHUL_SCYTHE_ACTIVE;
        sound_play(SFX_DEMON_SWING);
        if(dist<VARKHUL_SCYTHE_RANGE && cosf(e->yaw)*dx+sinf(e->yaw)*dz>-.1f)
            boss_hit_player(e->damage,e->x,e->z);
        break;
    case ENEMY_STATE_ATTACK:
        e->vx=e->vz=0;e->state_timer-=dt;
        if(e->boss_move==VARKHUL_DROP) {
            e->y=VARKHUL_DROP_HEIGHT*fmaxf(0,e->state_timer)/VARKHUL_FALL;
            if(e->state_timer>0)break;
            e->y=0;s_varkhul_fx.impact=VARKHUL_IMPACT_TIME;
            sound_play(SFX_BOSS_SLAM);
            if(vec2_dist(e->x,e->z,g_druid.x,g_druid.z)<=VARKHUL_DROP_RADIUS+g_druid.radius) {
                int damage=e->damage*16/10,cap=g_druid.max_hp*35/100;
                if(damage>cap)damage=cap;
                if(damage<1)damage=1;
                druid_take_damage_capped(damage,e->x,e->z,cap);
            }
        } else if(e->state_timer>0)break;
        e->state=ENEMY_STATE_RECOVERY;
        e->state_timer=e->boss_move==VARKHUL_DROP?VARKHUL_DROP_RECOVERY:VARKHUL_SCYTHE_RECOVERY;
        e->attack_cooldown=.45f;
        break;
    case ENEMY_STATE_RECOVERY:
        e->vx=e->vz=0;e->y=0;e->state_timer-=dt;
        if(e->state_timer<=0)e->state=ENEMY_STATE_CHASE;
        break;
    default:break;
    }
}

static void boss_update(Enemy *e, float dt, float dist, float dx, float dz) {
    if (g_campaign==0 && e->boss_phase==1) {malakor_update(e,dt,dist,dx,dz);return;}
    if (g_campaign==0 && e->boss_phase==2) {varkhul_update(e,dt,dist,dx,dz);return;}
    bool p2 = e->boss_phase == 2;
    float tf = (p2 ? 0.78f : 1.0f) * DIFF_TF[g_difficulty];
    int D = e->damage;
    if (e->morph_timer > 0.0f) {                   // transformando: parado e invulneravel
        e->morph_timer -= dt;
        e->vx = e->vz = 0.0f;
        if (e->morph_timer <= 0.0f) { e->state = ENEMY_STATE_CHASE; boss_ring(e, 12, 0.0f, 24); }
        return;
    }
    switch (e->state) {
        case ENEMY_STATE_IDLE:
            if (dist < 170.0f && g_druid.hp > 0) e->state = ENEMY_STATE_CHASE;
            break;
        case ENEMY_STATE_HURT:
        case ENEMY_STATE_CHASE: {
            e->state = ENEMY_STATE_CHASE;
            if (g_druid.hp <= 0) { e->state = ENEMY_STATE_IDLE; break; }
            e->yaw = atan2f(dz, dx);
            e->anim_timer += dt * 10.0f;
            if (p2 && !e->summoned && e->hp < e->max_hp / 2) {   // metade da fase 2: chama os cultistas
                e->summoned = true;
                for (int k = 0; k < 4; k++) {
                    float a = (float)k * 1.5707963f;
                    int idx = enemies_spawn_ex(ENEMY_TYPE_CULTIST, e->x + cosf(a) * 70.0f, e->z + sinf(a) * 70.0f, 900, false);
                    if (idx >= 0) enemies_force_chase(idx);
                }
                sound_play(SFX_CULTIST_CAST);
            }
            float speed = p2 ? 72.0f : 48.0f;
            if (e->attack_cooldown <= 0.0f && (dist < 40.0f || dist > 80.0f || e->boss_special_cooldown <= 0.0f)) {
                boss_pick(e, dist);
                break;
            }
            if (dist > 32.0f) {
                e->vx += dx * speed * dt * 8.0f;
                e->vz += dz * speed * dt * 8.0f;
            }
            break;
        }
        case ENEMY_STATE_TELEGRAPH: {
            e->state_timer -= dt;
            if (e->boss_move != BM_LUNGE || e->state_timer > 0.25f) e->yaw = atan2f(dz, dx);   // trava a mira no fim
            if (e->state_timer > 0.0f) break;
            e->state = ENEMY_STATE_ATTACK;
            e->state_timer = 0.25f;
            float fx = cosf(e->yaw), fz = sinf(e->yaw);
            switch (e->boss_move) {
                case BM_SLAM:
                    sound_play(SFX_BOSS_SLAM);
                    if (dist < 44.0f) druid_take_damage(D * 16 / 10, e->x, e->z);
                    if (p2) boss_ring(e, 10, 0.3f, 22);
                    break;
                case BM_GRAB:                           // garras de raiz: se pegar, devora uma semente
                    sound_play(SFX_BOSS_SLAM);
                    if (dist < 48.0f && g_druid.invuln_timer <= 0.0f) {
                        druid_take_damage(D, e->x, e->z);
                        if (g_seeds > 0) {
                            g_seeds--;
                            cards_refresh_stats();
                            hud_show_banner(T(S_SEED_STOLEN_T), T(S_SEED_STOLEN_S), 2.5f);
                        }
                    }
                    break;
                case BM_SWEEP:                          // foice: arco largo
                    sound_play(SFX_BOSS_SLAM);
                    if (dist < 54.0f) druid_take_damage(D * 13 / 10, e->x, e->z);
                    break;
                case BM_COMBO: {
                    bool last = e->combo_step <= 1;
                    sound_play(last ? SFX_BOSS_SLAM : SFX_DEMON_SWING);
                    e->vx += fx * 110.0f; e->vz += fz * 110.0f;   // avanca a cada golpe
                    if (dist < (last ? 44.0f : 36.0f)) druid_take_damage(last ? D * 14 / 10 : D, e->x, e->z);
                    if (last && p2) boss_ring(e, 8, 0.0f, 22);
                    break;
                }
                case BM_LUNGE:                          // investida em linha reta
                    sound_play(SFX_DEMON_SWING);
                    e->lunge_vx = fx * 270.0f; e->lunge_vz = fz * 270.0f;
                    e->lunge_hit = false;
                    e->state_timer = 0.45f;
                    break;
                default:                                // nova: aneis de orbes (Varkhul solta dois)
                    sound_play(SFX_BOSS_ENRAGE);
                    boss_ring(e, p2 ? 14 : 10, 0.0f, 22);
                    if (p2) { spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, dx, dz, 26); e->combo_step = 1; }
                    break;
            }
            break;
        }
        case ENEMY_STATE_ATTACK: {
            e->state_timer -= dt;
            if (e->boss_move == BM_LUNGE) {
                e->vx = e->lunge_vx; e->vz = e->lunge_vz;
                if (!e->lunge_hit && dist < e->radius + 12.0f) { e->lunge_hit = true; druid_take_damage(D * 14 / 10, e->x, e->z); }
            }
            if (e->state_timer > 0.0f) break;
            if (e->boss_move == BM_COMBO && e->combo_step > 1) {    // proximo golpe do combo
                e->combo_step--;
                e->state = ENEMY_STATE_TELEGRAPH;
                // o ultimo golpe vem atrasado: quem rola cedo demais apanha
                e->state_timer = (e->combo_step == 1 ? 0.8f : 0.42f) * tf;
                break;
            }
            if (e->boss_move == BM_NOVA && e->combo_step == 1) {    // Varkhul: segundo anel defasado
                e->combo_step = 0;
                boss_ring(e, 14, 0.224f, 22);
            }
            if (e->boss_move == BM_LUNGE && p2) boss_ring(e, 8, 0.0f, 22);
            e->state = ENEMY_STATE_RECOVERY;
            // janela de punicao depois de cada golpe pesado
            e->state_timer = (e->boss_move == BM_COMBO ? 1.1f : e->boss_move == BM_SLAM ? 1.2f :
                              e->boss_move == BM_LUNGE ? 1.0f : 0.8f) * (p2 ? 0.8f : 1.0f);
            e->attack_cooldown = p2 ? 0.35f : 0.6f;
            break;
        }
        case ENEMY_STATE_RECOVERY:
            e->state_timer -= dt;
            if (e->state_timer <= 0.0f) e->state = ENEMY_STATE_CHASE;
            break;
        default: break;
    }
}

static void demon_hit_player(Enemy *e, int amount, int cap) {
    int before=g_druid.hp;
    druid_take_damage_capped(amount,e->x,e->z,cap);
    if ((e->affix&AFX_VAMP) && g_druid.hp<before) {
        e->hp+=(before-g_druid.hp)*3;
        if (e->hp>e->max_hp) e->hp=e->max_hp;
    }
}

// True only when a Brute attack was processed. Ordinary navigation/detection
// stays in the existing FSM. Both attacks stop movement; a slam locks facing.
static bool demon_update(Enemy *e, float dt, float distance) {
    if (e->state==ENEMY_STATE_CHASE) {
        if (g_druid.hp<=0 || g_druid.camo_timer>0.0f || e->attack_cooldown>0.0f) return false;
        bool slam=e->boss_special_cooldown<=0.0f && distance<=DEMON_SLAM_TRIGGER_RANGE;
        if (!slam && distance>22.0f) return false;
        e->boss_move=slam?DEMON_MOVE_SLAM:DEMON_MOVE_NORMAL;
        e->yaw=atan2f(g_druid.z-e->z,g_druid.x-e->x);
        e->state=ENEMY_STATE_TELEGRAPH;
        e->state_timer=slam?DEMON_SLAM_WINDUP:fmaxf(e->telegraph_duration,DEMON_NORMAL_WINDUP_MIN);
        if (slam) e->boss_special_cooldown=DEMON_SLAM_COOLDOWN;
        e->vx=e->vz=0.0f;
        return true;
    }
    if (e->state!=ENEMY_STATE_TELEGRAPH && e->state!=ENEMY_STATE_ATTACK && e->state!=ENEMY_STATE_RECOVERY)
        return false;
    bool slam=e->boss_move==DEMON_MOVE_SLAM;
    e->vx=e->vz=0.0f;
    e->state_timer-=dt;
    if (e->state==ENEMY_STATE_TELEGRAPH) {
        if (!slam) e->yaw=atan2f(g_druid.z-e->z,g_druid.x-e->x);
        if (e->state_timer>0.0f) return true;
        e->state=ENEMY_STATE_ATTACK;
        e->state_timer=slam?DEMON_SLAM_ACTIVE:DEMON_NORMAL_ACTIVE;
        // Exactly one damage event, on the transition to the impact pose.
        sound_play(slam?SFX_BOSS_SLAM:SFX_DEMON_SWING);
        if (slam && distance<=DEMON_SLAM_RADIUS) {
            int damage=demon_percent_damage(g_druid.max_hp,DEMON_SLAM_PERCENT);
            demon_hit_player(e,damage,damage);
        } else if (!slam && distance<DEMON_NORMAL_REACH) {
            int cap=demon_percent_damage(g_druid.max_hp,DEMON_NORMAL_PERCENT_CAP);
            int damage=e->damage<cap?e->damage:cap;
            demon_hit_player(e,damage,cap);
        }
    } else if (e->state==ENEMY_STATE_ATTACK && e->state_timer<=0.0f) {
        // No elite rapid follow-up: this enemy is deliberately slow.
        e->combo_step=0;
        e->state=ENEMY_STATE_RECOVERY;
        e->state_timer=slam?DEMON_SLAM_RECOVERY:DEMON_NORMAL_RECOVERY;
        e->attack_cooldown=DEMON_NORMAL_COOLDOWN;
    } else if (e->state==ENEMY_STATE_RECOVERY && e->state_timer<=0.0f) {
        e->state=ENEMY_STATE_CHASE;
        e->boss_move=DEMON_MOVE_NORMAL;
    }
    return true;
}

static bool cultist_ally_valid(const Enemy *caster, const Enemy *target) {
    return target!=caster && target->active && target->hp>0 && target->max_hp>0 &&
        target->state!=ENEMY_STATE_DEAD &&
        target->hp<target->max_hp &&
        vec2_dist_sq(caster->x,caster->z,target->x,target->z)<=CULTIST_HEAL_RANGE*CULTIST_HEAL_RANGE;
}

static bool cultist_ally_visible(const Enemy *caster, const Enemy *target) {
    float hw,hh,tile,height;int cols,rows;
    if (!world_wall_info(&hw,&hh,&tile,&height,&cols,&rows) || tile<=0.0f) return true;
    float dx=target->x-caster->x,dz=target->z-caster->z;
    int steps=(int)ceilf(sqrtf(dx*dx+dz*dz)/(tile*.4f));
    if (steps<1) steps=1;
    for (int i=1;i<steps;i++) {
        float t=(float)i/(float)steps;
        int col=(int)floorf((caster->x+dx*t+hw)/tile);
        int row=(int)floorf((hh-caster->z-dz*t)/tile);
        if (col<0 || row<0 || col>=cols || row>=rows || world_wall_tile(col,row)) return false;
    }
    return true;
}

static int cultist_find_ally(const Enemy *caster) {
    int best=-1;float lowest=CULTIST_HEAL_THRESHOLD,nearest=1.0e30f;
    for (int i=0;i<MAX_ENEMIES;i++) {
        Enemy *target=&s_enemies[i];
        if (!cultist_ally_valid(caster,target)) continue;
        float fraction=(float)target->hp/(float)target->max_hp;
        float distance=vec2_dist_sq(caster->x,caster->z,target->x,target->z);
        if (fraction>lowest || (fraction==lowest && distance>=nearest)) continue;
        if (!cultist_ally_visible(caster,target)) continue;
        best=i;lowest=fraction;nearest=distance;
    }
    return best;
}

static bool cultist_heal_target(Enemy *caster) {
    int index=caster->combo_step;
    if (index<0 || index>=MAX_ENEMIES) return false;
    Enemy *target=&s_enemies[index];
    if (!cultist_ally_valid(caster,target) || !cultist_ally_visible(caster,target)) return false;
    int amount=(target->max_hp/100)*CULTIST_HEAL_PERCENT+
               ((target->max_hp%100)*CULTIST_HEAL_PERCENT)/100;
    if (amount<1) amount=1;
    int missing=target->max_hp-target->hp;
    target->hp+=amount<missing?amount:missing;
    int slot=0;
    for (int i=0;i<MAX_ENEMY_HEAL_EFFECTS;i++) {
        if (s_heal_fx[i].life<=0.0f || s_heal_fx[i].target==index) {slot=i;break;}
        if (s_heal_fx[i].life<s_heal_fx[slot].life) slot=i;
    }
    s_heal_fx[slot]=(EnemyHealEffect){CULTIST_HEAL_EFFECT_LIFETIME,index};
    sound_play(SFX_CULTIST_CAST);
    return true;
}

static bool cultist_update(Enemy *e, float dt, float distance) {
    if (e->lunge_vx>0.0f) e->lunge_vx-=dt;
    if (e->lunge_vz>0.0f) e->lunge_vz-=dt;
    if (e->state==ENEMY_STATE_CHASE) {
        if (g_druid.hp<=0 || g_druid.camo_timer>0.0f || e->attack_cooldown>0.0f || distance>=150.0f)
            return false;
        int target=-1;
        if (e->boss_special_cooldown<=0.0f) {
            if (e->lunge_vz>0.0f) return false; // Wait for ally scan before any offense.
            e->lunge_vz=CULTIST_SCAN_INTERVAL;
            target=cultist_find_ally(e);
        }
        bool heal=target>=0;
        if (!heal && (e->lunge_vx>0.0f || distance>=CULTIST_SKULL_RANGE)) return false;
        e->boss_move=heal?CULTIST_MOVE_HEAL:CULTIST_MOVE_SKULL;
        e->combo_step=target;
        if (heal) e->boss_special_cooldown=CULTIST_HEAL_COOLDOWN;
        else e->lunge_vx=CULTIST_SKULL_COOLDOWN;
        e->yaw=heal?atan2f(s_enemies[target].z-e->z,s_enemies[target].x-e->x):
                    atan2f(g_druid.z-e->z,g_druid.x-e->x);
        e->state=ENEMY_STATE_TELEGRAPH;e->state_timer=CULTIST_CAST_WINDUP;
        e->vx=e->vz=0.0f;
        return true;
    }
    if (e->state!=ENEMY_STATE_TELEGRAPH && e->state!=ENEMY_STATE_ATTACK && e->state!=ENEMY_STATE_RECOVERY)
        return false;
    e->vx=e->vz=0.0f;e->state_timer-=dt;
    if (e->state==ENEMY_STATE_TELEGRAPH) {
        if (e->boss_move==CULTIST_MOVE_SKULL)
            e->yaw=atan2f(g_druid.z-e->z,g_druid.x-e->x);
        if (e->state_timer>0.0f) return true;
        e->state=ENEMY_STATE_ATTACK;e->state_timer=CULTIST_CAST_RELEASE;
        if (e->boss_move==CULTIST_MOVE_HEAL) cultist_heal_target(e);
        else if (g_druid.hp>0 && g_druid.camo_timer<=0.0f)
            spells_cast_enemy_skull(e->x,e->y+18.0f,e->z,cosf(e->yaw),sinf(e->yaw),e->damage);
    } else if (e->state==ENEMY_STATE_ATTACK && e->state_timer<=0.0f) {
        e->state=ENEMY_STATE_RECOVERY;e->state_timer=CULTIST_CAST_RECOVERY;
    } else if (e->state==ENEMY_STATE_RECOVERY && e->state_timer<=0.0f) {
        e->state=ENEMY_STATE_CHASE;e->attack_cooldown=.20f;e->combo_step=-1;
    }
    return true;
}

void enemies_update(float dt) {
    campaign_combat_tick(dt);
    s_varkhul_fx.impact=fmaxf(0,s_varkhul_fx.impact-dt);
    for (int i=0;i<MAX_ENEMY_HEAL_EFFECTS;i++) {
        EnemyHealEffect *fx=&s_heal_fx[i];
        if (fx->life<=0.0f) continue;
        fx->life-=dt;
        if (fx->target<0 || fx->target>=MAX_ENEMIES || !s_enemies[fx->target].active ||
            s_enemies[fx->target].hp<=0 || s_enemies[fx->target].state==ENEMY_STATE_DEAD) fx->life=0.0f;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &s_enemies[i];
        if (!e->active) continue;

        // Death state fadeout
        if (e->state == ENEMY_STATE_DEAD) {
            e->state_timer -= dt;
            if (e->state_timer <= 0.0f) {
                e->active = false;
                if (i == s_boss_idx) s_boss_idx = -1;
            }
            continue;
        }

        // Timers
        if (e->hurt_timer > 0.0f) e->hurt_timer -= dt;
        if (e->attack_cooldown > 0.0f) e->attack_cooldown -= dt;
        if (e->boss_special_cooldown > 0.0f) e->boss_special_cooldown -= dt;
        if (e->root_timer > 0.0f) e->root_timer -= dt;
        if (e->poison_t > 0.0f) {                       // veneno: dano a cada meio segundo
            e->poison_t -= dt;
            e->poison_acc += dt;
            if (e->poison_acc >= 0.5f) {
                e->poison_acc -= 0.5f;
                int d = (int)(e->poison_dps * 0.5f + 0.5f);
                if (d > 0) {
                    float saved = g_hit_poison; g_hit_poison = 0.0f;
                    enemies_damage_at(e->x, e->z, 0.0f, d, 0.0f, 0.0f);
                    g_hit_poison = saved;
                    if (!e->active || e->state == ENEMY_STATE_DEAD) continue;
                }
            }
        }

        // Friction for knockback
        e->vx *= 0.85f;
        e->vz *= 0.85f;

        float dist_to_player = vec2_dist(e->x, e->z, g_druid.x, g_druid.z);
        float dir_to_px = (dist_to_player > 0.01f) ? (g_druid.x - e->x) / dist_to_player : 0.0f;
        float dir_to_pz = (dist_to_player > 0.01f) ? (g_druid.z - e->z) / dist_to_player : 1.0f;

        // Manto da cacadora: os monstros comuns perdem o alvo
        bool hidden = g_druid.camo_timer > 0.0f && e->type != ENEMY_TYPE_BOSS_MALAKOR;

        // AI FSM (o chefe tem a propria)
        if(world_event_enemy_update(i,dt)) { /* fleeing key carrier */ }
        else if(campaign_enemy_update(e,i,dt,dist_to_player,dir_to_px,dir_to_pz)) { /* campaign-specific FSM */ }
        else if (e->type == ENEMY_TYPE_BOSS_MALAKOR) boss_update(e, dt, dist_to_player, dir_to_px, dir_to_pz);
        else if (e->type==ENEMY_TYPE_DEMON && demon_update(e,dt,dist_to_player)) { /* special FSM */ }
        else if (e->type==ENEMY_TYPE_CULTIST && cultist_update(e,dt,dist_to_player)) { /* support FSM */ }
        else switch (e->state) {
            case ENEMY_STATE_IDLE: {
                // Check player detection range (120 units)
                if (dist_to_player < 150.0f && g_druid.hp > 0 && !hidden) {
                    e->state = ENEMY_STATE_CHASE;
                    alert_pack(e->pack);
                }
                break;
            }

            case ENEMY_STATE_CHASE: {
                if (hidden) { e->state = ENEMY_STATE_IDLE; e->vx *= 0.5f; e->vz *= 0.5f; break; }
                if (g_druid.hp <= 0) {
                    e->state = ENEMY_STATE_IDLE;
                    break;
                }

                e->yaw = atan2f(dir_to_pz, dir_to_px);
                e->anim_timer += dt * 10.0f;

                float speed = 0.0f;
                float attack_trigger_dist = 18.0f;

                if (e->type == ENEMY_TYPE_SKELETON) {
                    speed = enemy_move_speed(enemy_profile(e).movement);
                    attack_trigger_dist = 18.0f;
                } else if (e->type == ENEMY_TYPE_DEMON) {
                    speed = enemy_move_speed(enemy_profile(e).movement);
                    attack_trigger_dist = 22.0f;
                } else if (e->type == ENEMY_TYPE_CULTIST) {
                    // Cultist maintains range between 50 and 85 units
                    if (dist_to_player < 45.0f) {
                        // Back away
                        e->vx = -dir_to_px * enemy_move_speed(enemy_profile(e).movement);
                        e->vz = -dir_to_pz * enemy_move_speed(enemy_profile(e).movement);
                    } else if (dist_to_player > 80.0f) {
                        e->vx = dir_to_px * enemy_move_speed(enemy_profile(e).movement);
                        e->vz = dir_to_pz * enemy_move_speed(enemy_profile(e).movement);
                    } else {
                        e->vx = 0.0f;
                        e->vz = 0.0f;
                    }

                    // Casting and cooldowns belong exclusively to cultist_update.
                    break;
                }

                // Move toward player
                if (dist_to_player > attack_trigger_dist) {
                    float sm = (e->speed_mult > 0.0f) ? e->speed_mult : 1.0f;
                    e->vx += dir_to_px * speed * sm * dt * 8.0f;
                    e->vz += dir_to_pz * speed * sm * dt * 8.0f;
                } else {
                    if (e->attack_cooldown <= 0.0f) {
                        e->state = ENEMY_STATE_TELEGRAPH;
                        e->state_timer = e->telegraph_duration;
                    }
                }
                break;
            }

            case ENEMY_STATE_TELEGRAPH: {
                e->state_timer -= dt;
                e->yaw = atan2f(dir_to_pz, dir_to_px); // Face player during windup

                if (e->state_timer <= 0.0f) {
                    e->state = ENEMY_STATE_ATTACK;
                    e->state_timer = 0.25f;

                    // Execute attack based on type
                    if (e->type == ENEMY_TYPE_BOSS_MALAKOR) {
                        sound_play(SFX_BOSS_SLAM);
                        // Massive slam: damages player if within 38 units
                        if (dist_to_player < 38.0f) {
                            druid_take_damage(e->damage, e->x, e->z);
                        }
                        // Phase 2 radial nova projectiles
                        if (e->boss_phase == 2) {
                            for (int a = 0; a < 6; a++) {
                                float ang = (float)a * 1.047f;
                                spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, cosf(ang), sinf(ang), 18);
                            }
                        }
                    } else {
                        // Melee swipe (Skeleton or Demon)
                        sound_play(e->type == ENEMY_TYPE_DEMON
                            ? SFX_DEMON_SWING : SFX_SKELETON_SWING);
                        float hit_dist = (e->type == ENEMY_TYPE_DEMON) ? 24.0f : 20.0f;
                        if (dist_to_player < hit_dist) {
                            int before = g_druid.hp;
                            druid_take_damage(e->damage, e->x, e->z);
                            if ((e->affix & AFX_VAMP) && g_druid.hp < before) {   // vampirico: cura com o golpe
                                e->hp += (before - g_druid.hp) * 3;
                                if (e->hp > e->max_hp) e->hp = e->max_hp;
                            }
                        }
                    }
                }
                break;
            }

            case ENEMY_STATE_ATTACK: {
                e->state_timer -= dt;
                if (e->state_timer <= 0.0f) {
                    e->combo_step = 0;
                    e->state = ENEMY_STATE_RECOVERY;
                    e->state_timer = enemy_attack_recovery(enemy_profile(e).attack);
                    e->attack_cooldown = enemy_attack_cooldown(enemy_profile(e).attack);
                }
                break;
            }

            case ENEMY_STATE_RECOVERY: {
                e->state_timer -= dt;
                if (e->state_timer <= 0.0f) {
                    e->state = ENEMY_STATE_CHASE;
                }
                break;
            }

            default:
                break;
        }

        if (e->root_timer > 0.0f) { e->vx = 0.0f; e->vz = 0.0f; }   // preso pelas raizes

        // Apply velocities
        e->x += e->vx * dt;
        e->z += e->vz * dt;

        // Collide with world
        world_collide_circle(&e->x, &e->z, e->radius);

        // Separate from other enemies
        for (int j = i + 1; j < MAX_ENEMIES; j++) {
            if (!s_enemies[j].active || s_enemies[j].state == ENEMY_STATE_DEAD) continue;
            float edx = e->x - s_enemies[j].x;
            float edz = e->z - s_enemies[j].z;
            float edist_sq = edx * edx + edz * edz;
            float min_sep = e->radius + s_enemies[j].radius;
            if (edist_sq < (min_sep * min_sep) && edist_sq > 0.001f) {
                float edist = sqrtf(edist_sq);
                float overlap = (min_sep - edist) * 0.5f;
                float nx = edx / edist;
                float nz = edz / edist;
                e->x += nx * overlap;
                e->z += nz * overlap;
                s_enemies[j].x -= nx * overlap;
                s_enemies[j].z -= nz * overlap;
            }
        }

        // Build matrix (SRT de uma vez)
        float e_bob = (e->state == ENEMY_STATE_CHASE) ? fabsf(sinf(e->anim_timer)) * 1.2f : 0.0f;
        float e_yoff = 0.0f;
        if (e->state == ENEMY_STATE_DEAD) {
            e_yoff = - (1.5f - e->state_timer) * 6.0f; // afunda na cova
        }
        float e_s = (e->type == ENEMY_TYPE_BOSS_MALAKOR) ? 2.2f : 1.0f;
        float e_scale[3] = { e_s, e_s, e_s };
        float e_rot[3]   = { 0.0f, -e->yaw + 1.5707963f, 0.0f };
        float e_pos[3]   = { e->x, e->y + e_bob + e_yoff, e->z };
        t3d_mat4fp_from_srt_euler(&e->mat_fp, e_scale, e_rot, e_pos);
    }
}

void enemies_draw(void) {
    if (sprites_druid_ready()) return;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &s_enemies[i];
        if (!e->active) continue;

        // Flash on hurt or telegraph
        if (e->hurt_timer > 0.0f) {
            t3d_light_set_exposure(2.8f);
        } else if (e->state == ENEMY_STATE_TELEGRAPH) {
            t3d_light_set_exposure(1.6f);
        }

        gfx_matrix_push(&e->mat_fp);

        switch (e->type) {
            case ENEMY_TYPE_SKELETON:
                models_draw_mesh(&mesh_skeleton);
                break;
            case ENEMY_TYPE_DEMON:
                models_draw_mesh(&mesh_demon);
                break;
            case ENEMY_TYPE_CULTIST:
                models_draw_mesh(&mesh_cultist);
                break;
            case ENEMY_TYPE_BOSS_MALAKOR:
                models_draw_mesh(&mesh_boss_malakor);
                break;
        }

        t3d_matrix_pop(1);

        if (e->hurt_timer > 0.0f || e->state == ENEMY_STATE_TELEGRAPH) {
            t3d_light_set_exposure(1.0f);
        }
    }
}

Enemy* enemies_get_array(void) {
    return s_enemies;
}
const EnemyHealEffect *enemies_get_heal_effects(void) { return s_heal_fx; }

int enemies_get_active_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (s_enemies[i].active && s_enemies[i].state != ENEMY_STATE_DEAD) count++;
    }
    return count;
}

Enemy* enemies_get_boss(void) {
    if (s_boss_idx >= 0 && s_enemies[s_boss_idx].active) {
        return &s_enemies[s_boss_idx];
    }
    return NULL;
}
