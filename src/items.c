#include "gfx.h"
#include "items.h"
#include "cards.h"
#include "lang.h"
#include "models.h"
#include "audio.h"
#include "druid.h"
#include "hud.h"
#include "sprites.h"

static ItemDrop s_drops[MAX_DROPS];
static Chest s_chests[MAX_CHESTS];

void items_init(void) {
    items_clear();
}

void items_clear(void) {
    for (int i = 0; i < MAX_DROPS; i++) s_drops[i].active = false;
    for (int i = 0; i < MAX_CHESTS; i++) s_chests[i].active = false;
}

void items_spawn_drop(DropType type, float x, float z) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!s_drops[i].active) {
            s_drops[i].active = true;
            s_drops[i].type = type;
            s_drops[i].x = x + ((float)(rand() % 10) - 5.0f);
            s_drops[i].y = 4.0f;
            s_drops[i].z = z + ((float)(rand() % 10) - 5.0f);
            s_drops[i].bounce_timer = ((float)(rand() % 628)) / 100.0f;
            s_drops[i].life = 35.0f;
            return;
        }
    }
}

void items_add_chest(float x, float z, float yaw) {
    (void)yaw;
    items_add_chest_ex(x, z, false, false);
}

void items_unlock_chest(int i) {
    if (i >= 0 && i < MAX_CHESTS) s_chests[i].locked = false;
}

int items_add_chest_ex(float x, float z, bool small, bool locked) {
    float yaw = 0.0f;
    for (int i = 0; i < MAX_CHESTS; i++) {
        if (!s_chests[i].active) {
            s_chests[i].active = true;
            s_chests[i].small = small;
            s_chests[i].locked = locked;
            s_chests[i].opened = false;
            s_chests[i].open_timer = 0.0f;
            s_chests[i].x = x;
            s_chests[i].z = z;
            s_chests[i].yaw = yaw;

            float c_scale[3] = { 1.0f, 1.0f, 1.0f };
            float c_rot[3]   = { 0.0f, yaw, 0.0f };
            float c_pos[3]   = { x, 0.0f, z };
            t3d_mat4fp_from_srt_euler(&s_chests[i].mat_fp, c_scale, c_rot, c_pos);
            return i;
        }
    }
    return -1;
}

void items_update(float dt) {
    // 1. Update Drops
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!s_drops[i].active) continue;

        s_drops[i].life -= dt;
        if (s_drops[i].life <= 0.0f) {
            s_drops[i].active = false;
            continue;
        }

        s_drops[i].bounce_timer += dt * 3.5f;
        float float_y = 4.0f + sinf(s_drops[i].bounce_timer) * 2.0f;

        // Check pickup by player
        float d2 = vec2_dist_sq(s_drops[i].x, s_drops[i].z, g_druid.x, g_druid.z);
        if (d2 < (14.0f * 14.0f)) {
            // Collected!
            s_drops[i].active = false;
            sound_play(SFX_PICKUP);

            switch (s_drops[i].type) {
                case DROP_HEALTH_POTION:
                    if (g_druid.health_potions < 5) g_druid.health_potions++;
                    break;
                case DROP_SPIRIT_VIAL:
                    if (g_druid.spirit_potions < 5) g_druid.spirit_potions++;
                    break;
                case DROP_SOUL_ORB:
                    druid_gain_exp(15);
                    druid_restore_spirit(15.0f);
                    break;
                case DROP_ESSENCE:
                    g_druid.essence += 3;
                    break;
                case DROP_CONTINUE:
                    if (g_druid.continues < 3) g_druid.continues++;
                    hud_show_banner(T(S_CONT_PICK_T), T(S_CONT_PICK_S), 3.0f);
                    sound_play(SFX_PICKUP);
                    break;
            }
            continue;
        }

        // Build matrix (SRT de uma vez)
        float dr_scale[3] = { 0.6f, 0.6f, 0.6f };
        float dr_rot[3]   = { 0.0f, s_drops[i].bounce_timer, 0.0f };
        float dr_pos[3]   = { s_drops[i].x, float_y, s_drops[i].z };
        t3d_mat4fp_from_srt_euler(&s_drops[i].mat_fp, dr_scale, dr_rot, dr_pos);
    }

    // 2. Check Chest opening
    for (int i = 0; i < MAX_CHESTS; i++) {
        if (!s_chests[i].active) continue;
        if (s_chests[i].opened) {
            if (s_chests[i].open_timer < 0.3f) s_chests[i].open_timer += dt;
            continue;
        }

        if (s_chests[i].locked) continue;   // selado: o evento em volta precisa ser vencido primeiro
        float d2 = vec2_dist_sq(s_chests[i].x, s_chests[i].z, g_druid.x, g_druid.z);
        if (s_chests[i].small && d2 < (22.0f * 22.0f)) {
            // Bauzinho: sem carta. 1 pocao ou elixir, 1 orbe, um pouco de essencia.
            s_chests[i].opened = true;
            s_chests[i].open_timer = 0.0f;
            sound_play(SFX_CHEST);
            float bx = s_chests[i].x, bz = s_chests[i].z;
            items_spawn_drop((rand() & 1) ? DROP_HEALTH_POTION : DROP_SPIRIT_VIAL, bx - 10.0f, bz + 6.0f);
            items_spawn_drop(DROP_SOUL_ORB, bx + 10.0f, bz + 6.0f);
            int ne = 1 + rand() % 3;
            for (int k = 0; k < ne; k++) items_spawn_drop(DROP_ESSENCE, bx + (k - 1) * 7.0f, bz - 10.0f);
            druid_gain_exp(40);
            continue;
        }
        if (!s_chests[i].small && d2 < (28.0f * 28.0f)) {
            // Open chest
            s_chests[i].opened = true;
            s_chests[i].open_timer = 0.0f;
            sound_play(SFX_CHEST);
            // Cada bau oferece uma carta aleatoria (o jogo pausa para o jogador escolher).
            cards_grant_random();
            hud_show_banner(T(S_CHEST), "", 2.0f);

            // Bau: carta (acima) + 1 pocao, 1 elixir, 2 orbes e 12% de chance de um continue.
            float bx = s_chests[i].x, bz = s_chests[i].z;
            items_spawn_drop(DROP_HEALTH_POTION, bx - 14.0f, bz + 8.0f);
            items_spawn_drop(DROP_SPIRIT_VIAL, bx + 14.0f, bz + 8.0f);
            items_spawn_drop(DROP_SOUL_ORB, bx - 12.0f, bz - 6.0f);
            items_spawn_drop(DROP_SOUL_ORB, bx + 12.0f, bz - 6.0f);
            if (rand() % 100 < 4) items_spawn_drop(DROP_CONTINUE, bx, bz + 18.0f);   // continue: raro
            for (int k = 0; k < 6; k++) items_spawn_drop(DROP_ESSENCE, bx + (k - 2.5f) * 7.0f, bz - 16.0f);   // 18 de essencia
            druid_gain_exp(160);
        }
    }
}

void items_draw(void) {
    if (sprites_druid_ready()) return;
    // Draw chests
    for (int i = 0; i < MAX_CHESTS; i++) {
        if (!s_chests[i].active) continue;
        gfx_matrix_push(&s_chests[i].mat_fp);
        models_draw_mesh(&mesh_chest);
        t3d_matrix_pop(1);
    }

    // Draw item drops
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!s_drops[i].active) continue;
        gfx_matrix_push(&s_drops[i].mat_fp);
        switch (s_drops[i].type) {
            case DROP_HEALTH_POTION:
            case DROP_SOUL_ORB:
                models_draw_mesh(&mesh_spirit_bolt);
                break;
            case DROP_SPIRIT_VIAL:
                models_draw_mesh(&mesh_spirit_bolt);
                break;
            case DROP_CONTINUE:
            case DROP_ESSENCE:
                models_draw_mesh(&mesh_spirit_bolt);
                break;
        }
        t3d_matrix_pop(1);
    }
}

const ItemDrop* items_get_drops(void) { return s_drops; }
const Chest* items_get_chests(void) { return s_chests; }
