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

static Enemy s_enemies[MAX_ENEMIES];
static int s_boss_idx = -1;

void enemies_init(void) {
    enemies_clear();
}

void enemies_clear(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        s_enemies[i].active = false;
    }
    s_boss_idx = -1;
}

float g_enemy_power = 1.0f;
float g_enemy_aggro = 1.0f;
float g_hit_poison = 0.0f;

// Dificuldade: vida e dano dos monstros (Normal, Pesadelo, Inferno)
static const float DIFF_HP[DIFF_COUNT]  = { 1.0f, 1.6f, 2.5f };
static const float DIFF_DMG[DIFF_COUNT] = { 1.0f, 1.4f, 1.9f };

void enemies_set_affixes(int idx, int count) {
    if (idx < 0 || idx >= MAX_ENEMIES) return;
    Enemy *e = &s_enemies[idx];
    for (int k = 0; k < count && k < 3; k++) {
        uint8_t pick;
        do { pick = (uint8_t)(1u << (rand() % 3)); } while (e->affix & pick);
        e->affix |= pick;
        if (pick == AFX_FAST) { e->speed_mult *= 1.35f; e->telegraph_duration *= 0.8f; }
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

int enemies_spawn_ex(EnemyType type, float x, float z, int pack, bool elite) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!s_enemies[i].active) {
            Enemy *e = &s_enemies[i];
            memset(e, 0, sizeof(Enemy));
            e->active = true;
            e->type = type;
            e->state = ENEMY_STATE_IDLE;
            e->x = x;
            e->y = 0.0f;
            e->z = z;
            e->yaw = ((float)(rand() % 628)) / 100.0f;

            switch (type) {
                case ENEMY_TYPE_SKELETON:
                    e->max_hp = 105;
                    e->hp = e->max_hp;
                    e->damage = 15;
                    e->radius = 7.0f;
                    e->exp_reward = 30;
                    e->telegraph_duration = 0.34f;
                    break;
                case ENEMY_TYPE_DEMON:
                    e->max_hp = 205;
                    e->hp = e->max_hp;
                    e->damage = 26;
                    e->radius = 10.0f;
                    e->exp_reward = 65;
                    e->telegraph_duration = 0.42f;
                    break;
                case ENEMY_TYPE_CULTIST:
                    e->max_hp = 90;
                    e->hp = e->max_hp;
                    e->damage = 18;
                    e->radius = 6.5f;
                    e->exp_reward = 45;
                    e->telegraph_duration = 0.46f;
                    break;
                case ENEMY_TYPE_BOSS_MALAKOR:
                    e->max_hp = 2600;           // fase 1 (Malakor); o Varkhul ganha uma barra nova
                    e->hp = e->max_hp;
                    e->damage = 40;             // golpes de chefe: cada acerto doi de verdade
                    e->radius = 26.0f;          // sprite bem maior: corpo maior
                    e->exp_reward = 350;
                    e->telegraph_duration = 0.65f;
                    e->boss_phase = 1;
                    e->boss_special_cooldown = 4.0f;
                    s_boss_idx = i;
                    break;
            }

            if (type != ENEMY_TYPE_BOSS_MALAKOR && g_enemy_power > 1.001f) {
                e->max_hp = (int)((float)e->max_hp * g_enemy_power);
                e->hp = e->max_hp;
                e->damage = (int)((float)e->damage * (1.0f + (g_enemy_power - 1.0f) * 0.8f));
                e->exp_reward = (int)((float)e->exp_reward * (1.0f + (g_enemy_power - 1.0f) * 0.5f));
            }
            {   // dificuldade da run (tambem vale para o chefe)
                int d = g_difficulty < 0 ? 0 : g_difficulty >= DIFF_COUNT ? DIFF_COUNT - 1 : g_difficulty;
                e->max_hp = (int)((float)e->max_hp * DIFF_HP[d]);
                e->hp = e->max_hp;
                e->damage = (int)((float)e->damage * DIFF_DMG[d]);
            }
            e->pack = pack;
            e->speed_mult = 1.0f;
            if (type != ENEMY_TYPE_BOSS_MALAKOR) {   // areas mais fundas: monstros mais rapidos e agressivos
                e->telegraph_duration *= g_enemy_aggro;
                e->speed_mult = 1.0f + (1.0f - g_enemy_aggro) * 0.6f;
            }
            e->elite = elite;
            if (elite && type != ENEMY_TYPE_BOSS_MALAKOR) {
                // Guardiao: bem mais resistente e letal.
                e->max_hp = (int)((float)e->max_hp * (3.2f + (1.0f - g_enemy_aggro) * 3.0f));
                e->hp = e->max_hp;
                e->damage = (e->damage * 135) / 100;
                e->radius *= 1.2f;
                e->exp_reward *= 4;
                e->telegraph_duration *= 0.9f;
                e->speed_mult *= 1.12f;
                if (g_difficulty > DIFF_NORMAL) enemies_set_affixes(i, g_difficulty);   // Pesadelo 1, Inferno 2
            }
            return i;
        }
    }
    return -1;
}

bool enemies_damage_at(float x, float z, float radius, int damage, float knockback_x, float knockback_z) {
    bool hit_any = false;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &s_enemies[i];
        if (!e->active || e->state == ENEMY_STATE_DEAD) continue;

        float d2 = vec2_dist_sq(x, z, e->x, e->z);
        float combined_r = radius + e->radius;

        if (d2 <= (combined_r * combined_r)) {
            hit_any = true;
            if (e->type == ENEMY_TYPE_BOSS_MALAKOR && e->morph_timer > 0.0f) continue;   // transformando
            e->hp -= damage;
            if (g_hit_poison > 0.0f) { e->poison_t = 3.0f; e->poison_dps = g_hit_poison; }
            if (e->type == ENEMY_TYPE_BOSS_MALAKOR && e->boss_phase == 1 && e->hp <= 0) {
                // Malakor cai... e se ergue como Varkhul, com uma barra de vida nova.
                e->boss_phase = 2;
                e->max_hp = (int)(3000 * DIFF_HP[g_difficulty]);
                e->hp = e->max_hp;
                e->damage = (int)(50 * DIFF_DMG[g_difficulty]);
                e->morph_timer = 1.8f;
                e->state = ENEMY_STATE_RECOVERY;
                e->state_timer = 1.8f;
                e->combo_step = 0;
                e->boss_special_cooldown = 2.5f;
                sound_play(SFX_BOSS_ENRAGE);
                hud_show_banner(T(g_campaign == 2 ? S_BOSS_RAGE_TN : g_campaign == 1 ? S_BOSS_RAGE_TF : S_BOSS_RAGE_T), T(g_campaign == 2 ? S_BOSS_RAGE_SN : g_campaign == 1 ? S_BOSS_RAGE_SF : S_BOSS_RAGE_S), 2.5f);
                continue;
            }
            e->hurt_timer = 0.20f;
            static const SoundEffect hurt_sfx[] = {
                SFX_SKELETON_HURT, SFX_DEMON_HURT, SFX_CULTIST_HURT, SFX_BOSS_HURT
            };
            sound_play(hurt_sfx[e->type]);

            // Knockback (Boss resists heavy knockback)
            float kb_factor = (e->type == ENEMY_TYPE_BOSS_MALAKOR) ? 0.2f : 1.0f;
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

                if (e->type == ENEMY_TYPE_BOSS_MALAKOR) {
                    // Chefe final: carta garantida, quase sempre rara ou lendaria.
                    cards_grant_weighted(0, 40, 50, 10);
                    hud_show_banner(T(g_campaign == 2 ? S_BOSS_DEAD_TN : g_campaign == 1 ? S_BOSS_DEAD_TF : S_BOSS_DEAD_T), T(S_BOSS_DEAD_S), 3.5f);
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

static void boss_update(Enemy *e, float dt, float dist, float dx, float dz) {
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

void enemies_update(float dt) {
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
        if (e->type == ENEMY_TYPE_BOSS_MALAKOR) boss_update(e, dt, dist_to_player, dir_to_px, dir_to_pz);
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
                    speed = 56.0f;
                    attack_trigger_dist = 18.0f;
                } else if (e->type == ENEMY_TYPE_DEMON) {
                    speed = 64.0f;
                    attack_trigger_dist = 22.0f;
                } else if (e->type == ENEMY_TYPE_CULTIST) {
                    // Cultist maintains range between 50 and 85 units
                    if (dist_to_player < 45.0f) {
                        // Back away
                        e->vx = -dir_to_px * 40.0f;
                        e->vz = -dir_to_pz * 40.0f;
                    } else if (dist_to_player > 80.0f) {
                        e->vx = dir_to_px * 35.0f;
                        e->vz = dir_to_pz * 35.0f;
                    } else {
                        e->vx = 0.0f;
                        e->vz = 0.0f;
                    }

                    if (dist_to_player < 90.0f && e->attack_cooldown <= 0.0f) {
                        e->state = ENEMY_STATE_TELEGRAPH;
                        e->state_timer = e->telegraph_duration;
                    }
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
                    if (e->type == ENEMY_TYPE_CULTIST) {
                        spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, dir_to_px, dir_to_pz, e->damage);
                        if (e->elite) {   // guardiao cultista: leque de 3
                            float ca = cosf(0.35f), sa = sinf(0.35f);
                            spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, dir_to_px * ca - dir_to_pz * sa, dir_to_pz * ca + dir_to_px * sa, e->damage);
                            spells_cast_enemy_dark_orb(e->x, 14.0f, e->z, dir_to_px * ca + dir_to_pz * sa, dir_to_pz * ca - dir_to_px * sa, e->damage);
                        }
                    } else if (e->type == ENEMY_TYPE_BOSS_MALAKOR) {
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
                    // guardiao corpo a corpo: 45% de emendar um segundo golpe rapido
                    if (e->elite && e->type != ENEMY_TYPE_CULTIST && e->combo_step == 0 && rand() % 100 < 45) {
                        e->combo_step = 1;
                        e->state = ENEMY_STATE_TELEGRAPH;
                        e->state_timer = e->telegraph_duration * 0.6f;
                        break;
                    }
                    e->combo_step = 0;
                    e->state = ENEMY_STATE_RECOVERY;
                    e->state_timer = 0.45f * g_enemy_aggro;
                    e->attack_cooldown = ((e->type == ENEMY_TYPE_DEMON) ? 1.15f : 0.75f) * g_enemy_aggro;
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
