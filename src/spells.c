#include "gfx.h"
#include "spells.h"
#include "cards.h"
#include "models.h"
#include "models_gen.h"
#include "audio.h"
#include "druid.h"
#include "enemies.h"
#include "world.h"
#include "sprites.h"

static Projectile s_projectiles[MAX_PROJECTILES];
static ThornSpike s_thorns[MAX_THORNS];
static SpiritWolf s_wolves[MAX_WOLVES];
static RootTrap s_roots[MAX_ROOTS];
static Hawk s_hawks[MAX_HAWKS];

void spells_init(void) {
    spells_clear_all();
}

void spells_clear_all(void) {
    for (int i = 0; i < MAX_PROJECTILES; i++) s_projectiles[i].active = false;
    for (int i = 0; i < MAX_THORNS; i++) s_thorns[i].active = false;
    for (int i = 0; i < MAX_WOLVES; i++) s_wolves[i].active = false;
    for (int i = 0; i < MAX_ROOTS; i++) s_roots[i].active = false;
    for (int i = 0; i < MAX_HAWKS; i++) s_hawks[i].active = false;
}

// ---- Cacadora ----
bool spells_cast_arrow(float x, float z, float dir_x, float dir_z, int damage, bool pierce, float speed) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &s_projectiles[i];
        if (p->active) continue;
        p->active = true; p->type = PROJ_ARROW;
        p->x = x; p->y = 20.0f; p->z = z;
        p->vx = dir_x * speed; p->vz = dir_z * speed;
        p->radius = 4.0f; p->damage = damage; p->life = 1.0f;
        p->is_enemy = false; p->pierce = pierce; p->hit_cd = 0.0f;
        p->yaw = atan2f(dir_z, dir_x);
        return true;
    }
    return false;
}

bool spells_place_roots(float x, float z) {
    int lim = 2 + g_mods.trap_extra; if (lim > MAX_ROOTS) lim = MAX_ROOTS;
    int n = 0, oldest = -1; float ol = 1e9f;
    for (int i = 0; i < MAX_ROOTS; i++) if (s_roots[i].active) { n++; if (s_roots[i].life < ol) { ol = s_roots[i].life; oldest = i; } }
    if (n >= lim && oldest >= 0) s_roots[oldest].active = false;     // a mais antiga some
    for (int i = 0; i < MAX_ROOTS; i++) {
        if (s_roots[i].active) continue;
        float px = x, pz = z;
        world_collide_circle(&px, &pz, 6.0f);
        s_roots[i] = (RootTrap){ true, false, px, pz, 12.0f, 0.0f };
        sound_play(SFX_THORN_BURST);
        return true;
    }
    return false;
}

bool spells_cast_hawks(float x, float z) {
    int n = 1 + g_mods.wolf_extra; if (n > MAX_HAWKS) n = MAX_HAWKS;
    float life = 16.0f + g_mods.wolf_life;
    int dmg = cards_roll_damage(14 + g_druid.level * 4, g_mods.wolf_pct);
    for (int i = 0; i < MAX_HAWKS; i++) s_hawks[i].active = false;
    for (int i = 0; i < n; i++)
        s_hawks[i] = (Hawk){ true, x + (float)(i - (n - 1) * 0.5f) * 16.0f, 34.0f, z - 6.0f, 1.57f, life, life, 0.6f + 0.2f * i, 0.0f, dmg };
    sound_play(SFX_WOLF_SUMMON);
    return true;
}

const RootTrap* spells_get_roots(void) { return s_roots; }
const Hawk* spells_get_hawks(void) { return s_hawks; }

static void roots_update(float dt) {
    for (int i = 0; i < MAX_ROOTS; i++) {
        RootTrap *t = &s_roots[i];
        if (!t->active) continue;
        t->life -= dt;
        t->anim += dt;
        if (t->life <= 0.0f) { t->active = false; continue; }
        if (t->sprung) continue;
        Enemy *en = enemies_get_array();
        for (int k = 0; k < MAX_ENEMIES; k++) {
            if (!en[k].active || en[k].hp <= 0 || en[k].state == ENEMY_STATE_DEAD) continue;
            if (vec2_dist_sq(en[k].x, en[k].z, t->x, t->z) > 26.0f * 26.0f) continue;
            // dispara: prende e machuca todos em volta
            float r = 46.0f * (1.0f + g_mods.thorn_radius_pct);
            int dmg = cards_roll_damage(20 + g_druid.level * 5, g_mods.thorn_pct);
            enemies_root_at(t->x, t->z, r, 3.0f + g_mods.root_time);
            enemies_damage_at(t->x, t->z, r, dmg, 0.0f, 0.0f);
            sound_play(SFX_THORN_BURST);
            t->sprung = true; t->anim = 0.0f;
            t->life = 3.0f + g_mods.root_time;     // fica visivel enquanto segura
            break;
        }
    }
}

static void hawks_update(float dt) {
    Enemy *en = enemies_get_array();
    for (int i = 0; i < MAX_HAWKS; i++) {
        Hawk *h = &s_hawks[i];
        if (!h->active) continue;
        h->life -= dt; h->anim += dt;
        if (h->life <= 0.0f) { h->active = false; continue; }
        if (h->cd > 0.0f) h->cd -= dt;
        int best = -1; float bd = 170.0f * 170.0f;
        for (int k = 0; k < MAX_ENEMIES; k++) {
            if (!en[k].active || en[k].hp <= 0 || en[k].state == ENEMY_STATE_DEAD) continue;
            float d = vec2_dist_sq(en[k].x, en[k].z, g_druid.x, g_druid.z);
            if (d < bd) { bd = d; best = k; }
        }
        float tx, tz;
        if (best >= 0) { tx = en[best].x; tz = en[best].z; }
        else {                                       // circula sobre a cacadora
            float a = h->anim * 1.6f + i * 2.1f;
            tx = g_druid.x + cosf(a) * 26.0f; tz = g_druid.z + sinf(a) * 20.0f;
        }
        float dx = tx - h->x, dz = tz - h->z, d = sqrtf(dx * dx + dz * dz);
        float sp = 150.0f;
        if (d > 1.0f) {
            float step = fminf(d, sp * dt);  // nao ultrapassa o alvo e vira de lado a cada frame
            h->x += dx / d * step; h->z += dz / d * step;
            h->yaw = atan2f(dz, dx);
        }
        // mergulha para atacar (desce), depois sobe de novo
        float want_y = (best >= 0 && d < 20.0f) ? 12.0f : 34.0f;
        h->y += (want_y - h->y) * clampf(6.0f * dt, 0.0f, 1.0f);
        if (best >= 0 && d < 18.0f && h->cd <= 0.0f) {
            h->cd = 0.9f;
            enemies_damage_at(tx, tz, 10.0f, h->damage, dx * 0.1f, dz * 0.1f);
            sound_play(SFX_WOLF_BITE);
        }
    }
}

static bool spawn_bolt(float x, float z, float dir_x, float dir_z) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!s_projectiles[i].active) {
            s_projectiles[i].active = true;
            s_projectiles[i].type = PROJ_SPIRIT_BOLT;
            s_projectiles[i].x = x;
            s_projectiles[i].y = 24.0f;   // altura da mao/cajado (12 parecia sair do pe)
            s_projectiles[i].z = z;
            s_projectiles[i].vx = dir_x * 220.0f;
            s_projectiles[i].vz = dir_z * 220.0f;
            s_projectiles[i].radius = 6.0f;
            s_projectiles[i].damage = cards_roll_damage(32 + g_druid.level * 6, g_mods.bolt_pct);
            s_projectiles[i].life = 1.6f;
            s_projectiles[i].is_enemy = false;
            s_projectiles[i].pierce = g_mods.bolt_pierce;
            s_projectiles[i].hit_cd = 0.0f;
            return true;
        }
    }
    return false;
}

// Raio espiritual: 1 projetil + extras das cartas, em leque.
bool spells_cast_spirit_bolt(float x, float z, float dir_x, float dir_z) {
    int n = 1 + g_mods.bolt_extra;
    if (n > 5) n = 5;
    float base = atan2f(dir_z, dir_x);
    bool any = false;
    for (int k = 0; k < n; k++) {
        float a = base + ((float)k - (float)(n - 1) * 0.5f) * 0.20f;   // ~11 graus entre raios
        any |= spawn_bolt(x, z, cosf(a), sinf(a));
    }
    if (any) sound_play(SFX_SPIRIT_BOLT_CAST);
    return any;
}

bool spells_cast_enemy_dark_orb(float x, float y, float z, float dir_x, float dir_z, int damage) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!s_projectiles[i].active) {
            s_projectiles[i].active = true;
            s_projectiles[i].type = PROJ_DARK_BLOOD_ORB;
            s_projectiles[i].x = x;
            s_projectiles[i].y = y;
            s_projectiles[i].z = z;
            s_projectiles[i].vx = dir_x * 120.0f;
            s_projectiles[i].vz = dir_z * 120.0f;
            s_projectiles[i].radius = 6.0f;
            s_projectiles[i].damage = damage;
            s_projectiles[i].life = 2.5f;
            s_projectiles[i].is_enemy = true;

            sound_play(SFX_DARK_ORB_CAST);
            return true;
        }
    }
    return false;
}

bool spells_cast_thorn_burst(float center_x, float center_z) {
    sound_play(SFX_THORN_BURST);

    // Erupt 8 thorns in a ring + 2 in center
    float angles[8] = { 0.0f, 0.785f, 1.571f, 2.356f, 3.142f, 3.927f, 4.712f, 5.498f };
    float area = 1.0f + g_mods.thorn_radius_pct;
    float ring_r = 26.0f * area;
    int thorn_dmg = cards_roll_damage(45 + g_druid.level * 8, g_mods.thorn_pct);

    int spawned = 0;
    for (int i = 0; i < 8 && spawned < MAX_THORNS; i++) {
        for (int k = 0; k < MAX_THORNS; k++) {
            if (!s_thorns[k].active) {
                s_thorns[k].active = true;
                s_thorns[k].x = center_x + cosf(angles[i]) * ring_r;
                s_thorns[k].z = center_z + sinf(angles[i]) * ring_r;
                s_thorns[k].scale_y = 0.0f;
                s_thorns[k].life = 0.0f;
                s_thorns[k].max_life = 0.85f;
                s_thorns[k].radius = 18.0f;
                s_thorns[k].damage = thorn_dmg;
                s_thorns[k].hit_done = false;
                spawned++;
                break;
            }
        }
    }

    // Damage enemies in entire area immediately
    enemies_damage_at(center_x, center_z, 40.0f * area, thorn_dmg, 0.0f, 0.0f);
    debugf("[SPELL] Thorn Burst erupted! Spikes: %d, Center: (%.1f, %.1f)\n", spawned, center_x, center_z);
    return true;
}

bool spells_cast_spirit_wolves(float x, float z) {
    sound_play(SFX_WOLF_SUMMON);
    sound_play(SFX_WOLF_HOWL);

    int count = 2 + g_mods.wolf_extra;
    if (count > MAX_WOLVES) count = MAX_WOLVES;
    for (int i = 0; i < MAX_WOLVES; i++) s_wolves[i].active = false;
    for (int i = 0; i < count; i++) {
        float offset_x = ((float)i - (float)(count - 1) * 0.5f) * 18.0f;
        s_wolves[i].active = true;
        s_wolves[i].x = x + offset_x;
        s_wolves[i].y = 0.0f;
        s_wolves[i].z = z - 10.0f;
        s_wolves[i].vx = 0.0f;
        s_wolves[i].vz = 0.0f;
        s_wolves[i].yaw = 0.0f;
        s_wolves[i].life = 18.0f + g_mods.wolf_life;
        s_wolves[i].max_life = s_wolves[i].life;
        s_wolves[i].target_enemy_idx = -1;
        s_wolves[i].state = WOLF_IDLE;
        s_wolves[i].attack_cooldown = 0.0f;
        s_wolves[i].damage = cards_roll_damage(16 + g_druid.level * 4, g_mods.wolf_pct);
        s_wolves[i].anim_timer = 0.0f;
        s_wolves[i].stuck_t = 0.0f;
        s_wolves[i].ignore_t = 0.0f;
    }
    debugf("[SPELL] Spirit Wolves summoned! 2 spectral wolves ready.\n");

    return true;
}

void spells_update(float dt) {
    // 1. Update Projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!s_projectiles[i].active) continue;

        s_projectiles[i].life -= dt;
        if (s_projectiles[i].life <= 0.0f) {
            s_projectiles[i].active = false;
            continue;
        }

        s_projectiles[i].x += s_projectiles[i].vx * dt;
        s_projectiles[i].z += s_projectiles[i].vz * dt;

        // Player projectile hitting enemies
        if (!s_projectiles[i].is_enemy) {
            if (s_projectiles[i].type == PROJ_ARROW) g_hit_poison = g_mods.poison * (float)s_projectiles[i].damage;
            if (s_projectiles[i].hit_cd > 0.0f) s_projectiles[i].hit_cd -= dt;
            else if (enemies_damage_at(s_projectiles[i].x, s_projectiles[i].z, s_projectiles[i].radius + 6.0f, s_projectiles[i].damage, s_projectiles[i].vx * 0.1f, s_projectiles[i].vz * 0.1f)) {
                sound_play(SFX_SPIRIT_BOLT_HIT);
                if (s_projectiles[i].type == PROJ_ARROW && g_mods.lifesteal > 0.0f)
                    druid_heal((int)(s_projectiles[i].damage * g_mods.lifesteal + 0.5f));
                g_hit_poison = 0.0f;
                if (s_projectiles[i].pierce) { s_projectiles[i].hit_cd = 0.15f; }   // atravessa
                else { s_projectiles[i].active = false; continue; }
            }
            g_hit_poison = 0.0f;
        } else {
            // Enemy projectile hitting player
            float d2 = vec2_dist_sq(s_projectiles[i].x, s_projectiles[i].z, g_druid.x, g_druid.z);
            float hit_r = s_projectiles[i].radius + g_druid.radius;
            if (d2 < (hit_r * hit_r)) {
                druid_take_damage(s_projectiles[i].damage, s_projectiles[i].x, s_projectiles[i].z);
                s_projectiles[i].active = false;
                continue;
            }
        }

        // Projectile hitting world walls
        float test_x = s_projectiles[i].x;
        float test_z = s_projectiles[i].z;
        if (world_collide_circle(&test_x, &test_z, s_projectiles[i].radius)) {
            sound_play(SFX_SPIRIT_BOLT_HIT);
            s_projectiles[i].active = false;
            continue;
        }

        // Build matrix (SRT de uma vez)
        float p_scale[3] = { 1.0f, 1.0f, 1.0f };
        float p_rot[3]   = { 0.0f, 0.0f, 0.0f };
        float p_pos[3]   = { s_projectiles[i].x, s_projectiles[i].y, s_projectiles[i].z };
        t3d_mat4fp_from_srt_euler(&s_projectiles[i].mat_fp, p_scale, p_rot, p_pos);
    }

    roots_update(dt);
    hawks_update(dt);

    // 2. Update Thorn Spikes
    for (int i = 0; i < MAX_THORNS; i++) {
        if (!s_thorns[i].active) continue;

        s_thorns[i].life += dt;
        if (s_thorns[i].life >= s_thorns[i].max_life) {
            s_thorns[i].active = false;
            continue;
        }

        // Rapid erupt then retract animation
        float norm = s_thorns[i].life / s_thorns[i].max_life;
        if (norm < 0.25f) {
            s_thorns[i].scale_y = norm / 0.25f * 1.3f;
        } else if (norm < 0.65f) {
            s_thorns[i].scale_y = 1.3f;
        } else {
            s_thorns[i].scale_y = (1.0f - (norm - 0.65f) / 0.35f) * 1.3f;
        }

        // Build matrix (SRT de uma vez)
        float th_scale[3] = { 1.0f, s_thorns[i].scale_y, 1.0f };
        float th_rot[3]   = { 0.0f, 0.0f, 0.0f };
        float th_pos[3]   = { s_thorns[i].x, 0.0f, s_thorns[i].z };
        t3d_mat4fp_from_srt_euler(&s_thorns[i].mat_fp, th_scale, th_rot, th_pos);
    }

    // 3. Update Spirit Wolves
    for (int i = 0; i < MAX_WOLVES; i++) {
        if (!s_wolves[i].active) continue;

        s_wolves[i].life -= dt;
        if (s_wolves[i].life <= 0.0f) {
            s_wolves[i].active = false;
            continue;
        }

        if (s_wolves[i].attack_cooldown > 0.0f) {
            s_wolves[i].attack_cooldown -= dt;
        }

        // Find nearest living enemy
        Enemy *enemies = enemies_get_array();
        if (s_wolves[i].ignore_t > 0.0f) s_wolves[i].ignore_t -= dt;
        // Alvo: o inimigo vivo mais proximo do lobo que esteja perto do druida. Grupos parados
        // longe do druida (atras de cercas, por exemplo) sao ignorados, para os lobos nao
        // correrem contra obstaculos sem nunca alcancar ninguem.
        int target_idx = -1;
        if (s_wolves[i].ignore_t <= 0.0f) {
            float best = 160.0f * 160.0f;
            for (int k = 0; k < MAX_ENEMIES; k++) {
                const Enemy *c = &enemies[k];
                if (!c->active || c->hp <= 0 || c->state == ENEMY_STATE_DEAD) continue;
                float ddx = c->x - g_druid.x, ddz = c->z - g_druid.z;
                float dd = ddx * ddx + ddz * ddz;
                if (dd > 190.0f * 190.0f) continue;
                if (c->state == ENEMY_STATE_IDLE && dd > 100.0f * 100.0f) continue;
                float wx = c->x - s_wolves[i].x, wz = c->z - s_wolves[i].z;
                float d2 = wx * wx + wz * wz;
                if (d2 < best) { best = d2; target_idx = k; }
            }
        }

        float dest_x, dest_z;
        bool has_target = (target_idx >= 0 && enemies[target_idx].active && enemies[target_idx].hp > 0);

        if (has_target) {
            dest_x = enemies[target_idx].x;
            dest_z = enemies[target_idx].z;
        } else {
            // Follow Druid with slight offset
            float side = (i == 0) ? -20.0f : 20.0f;
            dest_x = g_druid.x + cosf(g_druid.yaw + 1.57f) * side;
            dest_z = g_druid.z + sinf(g_druid.yaw + 1.57f) * side;
        }

        float dx = dest_x - s_wolves[i].x;
        float dz = dest_z - s_wolves[i].z;
        float dist = sqrtf(dx * dx + dz * dz);

        float bite_range = has_target ? 24.0f : 14.0f;   // a mordida alcanca alem do centro do inimigo
        if (dist > bite_range) {
            // Run towards target
            float speed = 95.0f;
            s_wolves[i].vx = (dx / dist) * speed;
            s_wolves[i].vz = (dz / dist) * speed;
            s_wolves[i].yaw = atan2f(s_wolves[i].vz, s_wolves[i].vx);
            s_wolves[i].anim_timer += dt * 14.0f;
            s_wolves[i].state = WOLF_RUN;
        } else {
            // Close enough to attack
            s_wolves[i].vx = 0.0f;
            s_wolves[i].vz = 0.0f;

            if (has_target && s_wolves[i].attack_cooldown <= 0.0f) {
                s_wolves[i].attack_cooldown = 0.75f;
                s_wolves[i].state = WOLF_ATTACK;
                sound_play(SFX_WOLF_BITE);
                enemies_damage_at(enemies[target_idx].x, enemies[target_idx].z, 16.0f, s_wolves[i].damage, dx * 0.2f, dz * 0.2f);
            } else {
                s_wolves[i].state = WOLF_IDLE;
            }
        }

        float prev_x = s_wolves[i].x, prev_z = s_wolves[i].z;
        s_wolves[i].x += s_wolves[i].vx * dt;
        s_wolves[i].z += s_wolves[i].vz * dt;
        world_collide_circle(&s_wolves[i].x, &s_wolves[i].z, 6.0f);
        // Preso: quer andar mas quase nao saiu do lugar. Desiste do alvo por um tempo e segue o druida.
        if (s_wolves[i].state == WOLF_RUN) {
            float mvx = s_wolves[i].x - prev_x, mvz = s_wolves[i].z - prev_z;
            if (mvx * mvx + mvz * mvz < (95.0f * dt * 0.3f) * (95.0f * dt * 0.3f)) s_wolves[i].stuck_t += dt;
            else s_wolves[i].stuck_t = 0.0f;
            if (s_wolves[i].stuck_t > 0.5f) { s_wolves[i].stuck_t = 0.0f; s_wolves[i].ignore_t = 1.5f; }
        } else {
            s_wolves[i].stuck_t = 0.0f;
        }

        // Build matrix (SRT de uma vez)
        float w_bob = (s_wolves[i].state == WOLF_RUN) ? fabsf(sinf(s_wolves[i].anim_timer)) * 1.5f : 0.0f;
        float w_scale[3] = { 1.0f, 1.0f, 1.0f };
        float w_rot[3]   = { 0.0f, -s_wolves[i].yaw + 1.5707963f, 0.0f };
        float w_pos[3]   = { s_wolves[i].x, s_wolves[i].y + w_bob, s_wolves[i].z };
        t3d_mat4fp_from_srt_euler(&s_wolves[i].mat_fp, w_scale, w_rot, w_pos);
    }
}

void spells_draw(void) {
    if (sprites_druid_ready()) return;
    // 1. Draw Projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!s_projectiles[i].active) continue;
        gfx_matrix_push(&s_projectiles[i].mat_fp);
        if (s_projectiles[i].type == PROJ_SPIRIT_BOLT) {
            models_draw_mesh(&mesh_spirit_bolt);
        } else {
            models_draw_mesh(&mesh_dark_orb);
        }
        t3d_matrix_pop(1);
    }

    // 2. Draw Thorn Spikes
    for (int i = 0; i < MAX_THORNS; i++) {
        if (!s_thorns[i].active || s_thorns[i].scale_y <= 0.01f) continue;
        gfx_matrix_push(&s_thorns[i].mat_fp);
        models_draw_mesh(&mesh_thorn_spike);
        t3d_matrix_pop(1);
    }

    // 3. Draw Spirit Wolves
    for (int i = 0; i < MAX_WOLVES; i++) {
        if (!s_wolves[i].active) continue;
        gfx_matrix_push(&s_wolves[i].mat_fp);
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | T3D_FLAG_NO_LIGHT | gfx_cull_flag());
        models_draw_mesh(&mesh_spirit_wolf_model);
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
        t3d_matrix_pop(1);
    }
}

const Projectile* spells_get_projectiles(void) { return s_projectiles; }
const ThornSpike* spells_get_thorns(void) { return s_thorns; }
const SpiritWolf* spells_get_wolves(void) { return s_wolves; }
