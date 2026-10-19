#include "models.h"

// Mesh storage
Mesh mesh_druid_human;
Mesh mesh_druid_wolf;
Mesh mesh_skeleton;
Mesh mesh_demon;
Mesh mesh_cultist;
Mesh mesh_boss_malakor;
Mesh mesh_spirit_wolf;

Mesh mesh_part_torso;
Mesh mesh_part_head;
Mesh mesh_part_arm_upper;
Mesh mesh_part_arm_lower;
Mesh mesh_part_leg_upper;
Mesh mesh_part_leg_lower;

Mesh mesh_tree_dead;
Mesh mesh_gravestone;
Mesh mesh_chapel_arch;
Mesh mesh_crypt_pillar;
Mesh mesh_crypt_wall;
Mesh mesh_altar;
Mesh mesh_sarcophagus;
Mesh mesh_chest;

Mesh mesh_spirit_bolt;
Mesh mesh_thorn_spike;
Mesh mesh_dark_orb;

// Temporary triangle builder
#define MAX_BUILD_TRIS 320
typedef struct {
    int16_t x, y, z;
    uint32_t color;
    T3DVec3 norm;
} RawVert;

typedef struct {
    RawVert v[3];
} RawTri;

static RawTri s_tri_buffer[MAX_BUILD_TRIS];
static int s_tri_count = 0;

static void builder_reset(void) {
    s_tri_count = 0;
}

static void builder_add_tri(RawVert v0, RawVert v1, RawVert v2) {
    if (s_tri_count >= MAX_BUILD_TRIS) return;
    s_tri_buffer[s_tri_count].v[0] = v0;
    s_tri_buffer[s_tri_count].v[1] = v1;
    s_tri_buffer[s_tri_count].v[2] = v2;
    s_tri_count++;
}

static void builder_add_quad(RawVert v0, RawVert v1, RawVert v2, RawVert v3) {
    builder_add_tri(v0, v1, v2);
    builder_add_tri(v0, v2, v3);
}

// Box helper
static void builder_add_box(float minx, float miny, float minz,
                            float maxx, float maxy, float maxz,
                            uint32_t color_top, uint32_t color_side, uint32_t color_bot) {
    int16_t x0 = (int16_t)minx, x1 = (int16_t)maxx;
    int16_t y0 = (int16_t)miny, y1 = (int16_t)maxy;
    int16_t z0 = (int16_t)minz, z1 = (int16_t)maxz;

    // Top (+Y)
    builder_add_quad(
        (RawVert){x0, y1, z1, color_top, {{0, 1, 0}}},
        (RawVert){x1, y1, z1, color_top, {{0, 1, 0}}},
        (RawVert){x1, y1, z0, color_top, {{0, 1, 0}}},
        (RawVert){x0, y1, z0, color_top, {{0, 1, 0}}}
    );
    // Bottom (-Y)
    builder_add_quad(
        (RawVert){x0, y0, z0, color_bot, {{0, -1, 0}}},
        (RawVert){x1, y0, z0, color_bot, {{0, -1, 0}}},
        (RawVert){x1, y0, z1, color_bot, {{0, -1, 0}}},
        (RawVert){x0, y0, z1, color_bot, {{0, -1, 0}}}
    );
    // Front (+Z)
    builder_add_quad(
        (RawVert){x0, y0, z1, color_side, {{0, 0, 1}}},
        (RawVert){x1, y0, z1, color_side, {{0, 0, 1}}},
        (RawVert){x1, y1, z1, color_side, {{0, 0, 1}}},
        (RawVert){x0, y1, z1, color_side, {{0, 0, 1}}}
    );
    // Back (-Z)
    builder_add_quad(
        (RawVert){x1, y0, z0, color_side, {{0, 0, -1}}},
        (RawVert){x0, y0, z0, color_side, {{0, 0, -1}}},
        (RawVert){x0, y1, z0, color_side, {{0, 0, -1}}},
        (RawVert){x1, y1, z0, color_side, {{0, 0, -1}}}
    );
    // Right (+X)
    builder_add_quad(
        (RawVert){x1, y0, z1, color_side, {{1, 0, 0}}},
        (RawVert){x1, y0, z0, color_side, {{1, 0, 0}}},
        (RawVert){x1, y1, z0, color_side, {{1, 0, 0}}},
        (RawVert){x1, y1, z1, color_side, {{1, 0, 0}}}
    );
    // Left (-X)
    builder_add_quad(
        (RawVert){x0, y0, z0, color_side, {{-1, 0, 0}}},
        (RawVert){x0, y0, z1, color_side, {{-1, 0, 0}}},
        (RawVert){x0, y1, z1, color_side, {{-1, 0, 0}}},
        (RawVert){x0, y1, z0, color_side, {{-1, 0, 0}}}
    );
}

// Prisma de N lados (cilindro facetado), com taper opcional (raio base->topo).
// Autora em unidades reais; multiplica por MESH_PREC para o grid fino.
static void builder_add_prism(float cx, float cz, float y0, float y1,
                              float r0x, float r0z, float r1x, float r1z, int sides,
                              uint32_t cTop, uint32_t cSide, uint32_t cBot) {
    const float P = (float)MESH_PREC;
    const float TAU = 6.2831853f;
    for (int i = 0; i < sides; i++) {
        float a0 = TAU * (float)i / (float)sides;
        float a1 = TAU * (float)(i + 1) / (float)sides;
        float c0 = cosf(a0), s0 = sinf(a0);
        float c1 = cosf(a1), s1 = sinf(a1);
        RawVert b0 = { (int16_t)((cx + c0*r0x)*P), (int16_t)(y0*P), (int16_t)((cz + s0*r0z)*P), cSide, {{c0, 0, s0}} };
        RawVert b1 = { (int16_t)((cx + c1*r0x)*P), (int16_t)(y0*P), (int16_t)((cz + s1*r0z)*P), cSide, {{c1, 0, s1}} };
        RawVert t0 = { (int16_t)((cx + c0*r1x)*P), (int16_t)(y1*P), (int16_t)((cz + s0*r1z)*P), cSide, {{c0, 0, s0}} };
        RawVert t1 = { (int16_t)((cx + c1*r1x)*P), (int16_t)(y1*P), (int16_t)((cz + s1*r1z)*P), cSide, {{c1, 0, s1}} };
        builder_add_quad(b0, b1, t1, t0);
        RawVert tc = { (int16_t)(cx*P), (int16_t)(y1*P), (int16_t)(cz*P), cTop, {{0, 1, 0}} };
        builder_add_tri(t0, t1, tc);
        RawVert bc = { (int16_t)(cx*P), (int16_t)(y0*P), (int16_t)(cz*P), cBot, {{0, -1, 0}} };
        builder_add_tri(b1, b0, bc);
    }
}

// Convert builder state to packed T3DVertPacked
static void builder_add_box_p(float minx, float miny, float minz,
                              float maxx, float maxy, float maxz,
                              uint32_t cTop, uint32_t cSide, uint32_t cBot) {
    const float P = (float)MESH_PREC;
    builder_add_box(minx*P, miny*P, minz*P, maxx*P, maxy*P, maxz*P, cTop, cSide, cBot);
}

static Mesh builder_finish(void) {
    Mesh mesh;
    mesh.tri_count = s_tri_count;
    mesh.vert_count = s_tri_count * 3;
    uint32_t pair_count = (mesh.vert_count + 1) / 2;

    mesh.verts = (T3DVertPacked*)malloc_uncached(pair_count * sizeof(T3DVertPacked));
    memset(mesh.verts, 0, pair_count * sizeof(T3DVertPacked));

    for (int t = 0; t < s_tri_count; t++) {
        for (int i = 0; i < 3; i++) {
            int v_idx = t * 3 + i;
            int pair_idx = v_idx / 2;
            int sub_idx = v_idx % 2;
            const RawVert *rv = &s_tri_buffer[t].v[i];
            uint16_t norm = t3d_vert_pack_normal(&rv->norm);

            if (sub_idx == 0) {
                mesh.verts[pair_idx].posA[0] = rv->x;
                mesh.verts[pair_idx].posA[1] = rv->y;
                mesh.verts[pair_idx].posA[2] = rv->z;
                mesh.verts[pair_idx].rgbaA = rv->color;
                mesh.verts[pair_idx].normA = norm;
            } else {
                mesh.verts[pair_idx].posB[0] = rv->x;
                mesh.verts[pair_idx].posB[1] = rv->y;
                mesh.verts[pair_idx].posB[2] = rv->z;
                mesh.verts[pair_idx].rgbaB = rv->color;
                mesh.verts[pair_idx].normB = norm;
            }
        }
    }

    return mesh;
}

// Build Druid (Human Form)
static Mesh build_druid_human(void) {
    builder_reset();
    uint32_t c_moss   = 0x2E442BFF;
    uint32_t c_tunic  = 0x4B3728FF;
    uint32_t c_cowl   = 0x5C4636FF;
    uint32_t c_skin   = 0xC29E7DFF;
    uint32_t c_horn   = 0xD0C4A4FF;
    uint32_t c_wood   = 0x543A24FF;
    uint32_t c_rune   = 0x00E5FFFF;

    // Robe / Legs
    builder_add_box(-3.5f, 0.0f, -2.5f,  3.5f, 10.0f, 2.5f, c_moss, c_tunic, c_tunic);
    // Torso / Chest
    builder_add_box(-4.0f, 10.0f, -3.0f, 4.0f, 20.0f, 3.0f, c_cowl, c_moss, c_tunic);
    // Head & Hood
    builder_add_box(-3.0f, 20.0f, -3.0f, 3.0f, 26.0f, 3.0f, c_cowl, c_cowl, c_skin);
    // Face plate/opening
    builder_add_box(-2.0f, 21.0f, 2.8f,  2.0f, 24.5f, 3.2f, c_skin, c_skin, c_skin);

    // Antlers / Horns on Cowl
    builder_add_box(-4.5f, 25.0f, -1.0f, -2.5f, 29.0f, 0.5f, c_horn, c_horn, c_horn);
    builder_add_box( 2.5f, 25.0f, -1.0f,  4.5f, 29.0f, 0.5f, c_horn, c_horn, c_horn);
    builder_add_box(-6.0f, 28.0f, -1.0f, -3.5f, 31.0f, 0.5f, c_horn, c_horn, c_horn);
    builder_add_box( 3.5f, 28.0f, -1.0f,  6.0f, 31.0f, 0.5f, c_horn, c_horn, c_horn);

    // Left Arm holding Staff
    builder_add_box(-6.5f, 10.0f, -2.0f, -4.0f, 19.0f, 2.0f, c_cowl, c_moss, c_tunic);
    // Right Arm (free/casting)
    builder_add_box( 4.0f, 10.0f, -2.0f,  6.5f, 19.0f, 2.0f, c_cowl, c_moss, c_tunic);

    // Mystic Staff (tall wooden staff in left hand)
    builder_add_box(-6.5f, 0.0f, 2.0f, -5.0f, 32.0f, 3.5f, c_wood, c_wood, c_wood);
    // Glowing spirit crystal at staff top
    builder_add_box(-7.5f, 31.0f, 1.0f, -4.0f, 35.5f, 4.5f, c_rune, c_rune, c_rune);

    return builder_finish();
}

// Build Druid (Werewolf Beast Form)
static Mesh build_druid_wolf(void) {
    builder_reset();
    uint32_t c_fur_dark  = 0x261B16FF;
    uint32_t c_fur_med   = 0x483227FF;
    uint32_t c_fur_light = 0x6E4C3CFF;
    uint32_t c_eyes      = 0xFF3B00FF;
    uint32_t c_claws     = 0xD8D4CBFF;

    // Muscular hunched legs
    builder_add_box(-5.0f, 0.0f, -4.0f,  5.0f, 11.0f, 4.0f, c_fur_dark, c_fur_dark, c_fur_dark);
    // Massive hunched upper torso
    builder_add_box(-6.5f, 11.0f, -4.5f, 6.5f, 23.0f, 4.5f, c_fur_light, c_fur_med, c_fur_dark);
    // Bestial wolf head projecting forward
    builder_add_box(-4.0f, 19.0f, 1.0f,  4.0f, 26.5f, 8.0f, c_fur_med, c_fur_med, c_fur_dark);
    // Snout / Jaws
    builder_add_box(-2.5f, 18.5f, 7.5f,  2.5f, 22.5f, 12.0f, c_fur_dark, c_fur_dark, c_fur_dark);
    // Pointed Wolf Ears
    builder_add_box(-3.5f, 26.0f, 2.0f, -1.5f, 30.0f, 4.0f, c_fur_dark, c_fur_dark, c_fur_dark);
    builder_add_box( 1.5f, 26.0f, 2.0f,  3.5f, 30.0f, 4.0f, c_fur_dark, c_fur_dark, c_fur_dark);
    // Glowing red predatory eyes
    builder_add_box(-3.2f, 23.0f, 7.5f, -1.2f, 24.5f, 8.2f, c_eyes, c_eyes, c_eyes);
    builder_add_box( 1.2f, 23.0f, 7.5f,  3.2f, 24.5f, 8.2f, c_eyes, c_eyes, c_eyes);

    // Massive claw arms
    builder_add_box(-9.5f, 10.0f, 0.0f, -6.5f, 22.0f, 4.0f, c_fur_med, c_fur_med, c_fur_dark);
    builder_add_box( 6.5f, 10.0f, 0.0f,  9.5f, 22.0f, 4.0f, c_fur_med, c_fur_med, c_fur_dark);
    // Razor sharp claws
    builder_add_box(-9.5f, 6.0f, 2.0f, -7.0f, 10.5f, 5.5f, c_claws, c_claws, c_claws);
    builder_add_box( 7.0f, 6.0f, 2.0f,  9.5f, 10.5f, 5.5f, c_claws, c_claws, c_claws);

    return builder_finish();
}

// Build Skeleton Warrior
static Mesh build_skeleton(void) {
    builder_reset();
    uint32_t c_bone  = 0xD6CEBAFF;
    uint32_t c_dark  = 0x484236FF;
    uint32_t c_steel = 0x8A8E94FF;
    uint32_t c_rust  = 0x734832FF;

    // Pelvis & Legs
    builder_add_box(-3.0f, 0.0f, -1.5f,  3.0f, 11.0f, 1.5f, c_bone, c_bone, c_bone);
    // Ribcage & Spine
    builder_add_box(-3.5f, 11.0f, -2.0f, 3.5f, 19.5f, 2.0f, c_bone, c_bone, c_dark);
    // Skull
    builder_add_box(-2.5f, 19.5f, -2.0f, 2.5f, 25.0f, 2.5f, c_bone, c_bone, c_bone);
    // Dark hollow eye sockets
    builder_add_box(-2.0f, 21.5f, 2.2f, -0.6f, 23.2f, 2.7f, 0x111111FF, 0x111111FF, 0x111111FF);
    builder_add_box( 0.6f, 21.5f, 2.2f,  2.0f, 23.2f, 2.7f, 0x111111FF, 0x111111FF, 0x111111FF);

    // Left Arm holding Round Shield
    builder_add_box(-5.5f, 10.0f, -1.5f, -3.5f, 19.0f, 1.5f, c_bone, c_bone, c_bone);
    builder_add_box(-7.0f, 11.0f,  1.5f, -4.0f, 18.0f, 2.5f, c_rust, c_rust, c_steel);

    // Right Arm wielding sword
    builder_add_box( 3.5f, 10.0f, -1.5f,  5.5f, 19.0f, 1.5f, c_bone, c_bone, c_bone);
    // Rusty Sword blade
    builder_add_box( 4.5f, 14.0f,  1.0f,  5.5f, 28.0f, 2.2f, c_steel, c_steel, c_rust);

    return builder_finish();
}

// Build Demon Fiend
static Mesh build_demon(void) {
    builder_reset();
    uint32_t c_skin = 0x6E1E1EFF;
    uint32_t c_dark = 0x361010FF;
    uint32_t c_horn = 0x1C1C1EFF;
    uint32_t c_eyes = 0xFFCC00FF;
    uint32_t c_iron = 0x54545EFF;

    // Heavy demon legs
    builder_add_box(-4.5f, 0.0f, -3.0f,  4.5f, 12.0f, 3.0f, c_skin, c_dark, c_dark);
    // Broad muscular torso
    builder_add_box(-6.0f, 12.0f, -4.0f, 6.0f, 23.0f, 4.0f, c_skin, c_skin, c_dark);
    // Demon head with snout/mouth
    builder_add_box(-3.5f, 23.0f, -3.0f, 3.5f, 29.5f, 3.5f, c_skin, c_skin, c_dark);
    // Fiery yellow eyes
    builder_add_box(-2.8f, 26.0f, 3.2f, -1.0f, 27.5f, 3.7f, c_eyes, c_eyes, c_eyes);
    builder_add_box( 1.0f, 26.0f, 3.2f,  2.8f, 27.5f, 3.7f, c_eyes, c_eyes, c_eyes);

    // Ram / Demon Horns
    builder_add_box(-5.5f, 28.0f, -1.0f, -3.0f, 33.5f, 1.5f, c_horn, c_horn, c_horn);
    builder_add_box( 3.0f, 28.0f, -1.0f,  5.5f, 33.5f, 1.5f, c_horn, c_horn, c_horn);
    builder_add_box(-7.5f, 32.0f, -3.0f, -5.0f, 35.0f, 0.0f, c_horn, c_horn, c_horn);
    builder_add_box( 5.0f, 32.0f, -3.0f,  7.5f, 35.0f, 0.0f, c_horn, c_horn, c_horn);

    // Heavy arms
    builder_add_box(-8.5f, 11.0f, -2.0f, -6.0f, 22.0f, 2.5f, c_skin, c_skin, c_dark);
    builder_add_box( 6.0f, 11.0f, -2.0f,  8.5f, 22.0f, 2.5f, c_skin, c_skin, c_dark);
    // Demonic Cleaver
    builder_add_box( 7.0f, 15.0f,  2.0f,  9.0f, 30.0f, 6.5f, c_iron, c_iron, c_dark);

    return builder_finish();
}

// Build Corrupted Cultist
static Mesh build_cultist(void) {
    builder_reset();
    uint32_t c_robe  = 0x36163CFF;
    uint32_t c_trim  = 0x8C1E3CFF;
    uint32_t c_mask  = 0xC2BAA8FF;
    uint32_t c_wood  = 0x422A1AFF;
    uint32_t c_blood = 0xD41530FF;

    // Long dark robe
    builder_add_box(-3.5f, 0.0f, -2.5f,  3.5f, 16.0f, 2.5f, c_robe, c_robe, c_robe);
    builder_add_box(-4.0f, 16.0f, -3.0f, 4.0f, 22.0f, 3.0f, c_trim, c_robe, c_robe);
    // Hooded head
    builder_add_box(-3.0f, 22.0f, -2.5f, 3.0f, 27.5f, 2.5f, c_robe, c_robe, c_robe);
    // Pale bone mask
    builder_add_box(-2.0f, 23.0f, 2.2f,  2.0f, 26.5f, 2.7f, c_mask, c_mask, c_mask);

    // Robed sleeves
    builder_add_box(-6.0f, 13.0f, -1.8f, -4.0f, 21.0f, 1.8f, c_robe, c_robe, c_trim);
    builder_add_box( 4.0f, 13.0f, -1.8f,  6.0f, 21.0f, 1.8f, c_robe, c_robe, c_trim);

    // Dark Blood Wand / Staff
    builder_add_box( 5.0f, 8.0f, 1.5f, 6.2f, 26.0f, 2.7f, c_wood, c_wood, c_wood);
    builder_add_box( 4.5f, 25.0f, 1.0f, 6.7f, 28.5f, 3.2f, c_blood, c_blood, c_blood);

    return builder_finish();
}

// Build Boss Malakor, The Corrupted Warden (Grand, imposing)
static Mesh build_boss_malakor(void) {
    builder_reset();
    uint32_t c_plate = 0x22222AFF;
    uint32_t c_blood = 0x9E1111FF;
    uint32_t c_gold  = 0xB8860BFF;
    uint32_t c_fire  = 0xFF4500FF;
    uint32_t c_eye   = 0xFFFF00FF;

    // Armored legs (heavy scale)
    builder_add_box(-7.0f, 0.0f, -5.0f,  7.0f, 18.0f, 5.0f, c_plate, c_plate, c_plate);
    // Heavy spiked plate torso
    builder_add_box(-10.0f, 18.0f, -6.5f, 10.0f, 36.0f, 6.5f, c_blood, c_plate, c_gold);
    // Pauldrons (giant spiked shoulder guards)
    builder_add_box(-15.0f, 30.0f, -5.0f, -9.5f, 40.0f, 5.0f, c_gold, c_plate, c_blood);
    builder_add_box(  9.5f, 30.0f, -5.0f, 15.0f, 40.0f, 5.0f, c_gold, c_plate, c_blood);

    // Horned Warden Helmet
    builder_add_box(-5.0f, 36.0f, -5.0f, 5.0f, 46.0f, 5.0f, c_plate, c_plate, c_gold);
    // Demonic Horns
    builder_add_box(-9.0f, 44.0f, -2.0f, -4.5f, 52.0f, 2.0f, c_fire, c_plate, c_plate);
    builder_add_box( 4.5f, 44.0f, -2.0f,  9.0f, 52.0f, 2.0f, c_fire, c_plate, c_plate);
    // Glowing golden-red visor slits
    builder_add_box(-4.0f, 40.5f, 4.8f, -1.0f, 42.5f, 5.5f, c_eye, c_eye, c_eye);
    builder_add_box( 1.0f, 40.5f, 4.8f,  4.0f, 42.5f, 5.5f, c_eye, c_eye, c_eye);

    // Left Arm holding massive shield
    builder_add_box(-13.5f, 18.0f, -4.0f, -9.5f, 34.0f, 4.0f, c_plate, c_plate, c_blood);
    builder_add_box(-18.0f, 16.0f,  3.0f, -10.0f, 36.0f, 5.5f, c_plate, c_gold, c_blood);

    // Right Arm wielding Colossal Flaming Greatsword
    builder_add_box( 9.5f, 18.0f, -4.0f, 13.5f, 34.0f, 4.0f, c_plate, c_plate, c_blood);
    // Greatsword hilt & crossguard
    builder_add_box( 10.0f, 24.0f, 4.0f, 13.0f, 28.0f, 6.0f, c_gold, c_gold, c_gold);
    builder_add_box(  6.0f, 27.5f, 3.5f, 17.0f, 29.5f, 6.5f, c_gold, c_gold, c_gold);
    // Colossal Flaming Blade
    builder_add_box(  9.5f, 29.5f, 4.0f, 13.5f, 58.0f, 6.0f, c_fire, c_fire, c_blood);

    return builder_finish();
}

// Build Spirit Wolf
static Mesh build_spirit_wolf(void) {
    builder_reset();
    uint32_t c_body = 0x00D0FFFF; // Ethereal cyan glow
    uint32_t c_dark = 0x0078AAFF;
    uint32_t c_eye  = 0xE0FFFFFF;

    // Body
    builder_add_box(-3.0f, 3.0f, -6.0f, 3.0f, 9.0f, 6.0f, c_body, c_body, c_dark);
    // 4 Running Legs
    builder_add_box(-3.0f, 0.0f, -5.5f, -1.5f, 4.0f, -4.0f, c_dark, c_dark, c_dark);
    builder_add_box( 1.5f, 0.0f, -5.5f,  3.0f, 4.0f, -4.0f, c_dark, c_dark, c_dark);
    builder_add_box(-3.0f, 0.0f,  4.0f, -1.5f, 4.0f,  5.5f, c_dark, c_dark, c_dark);
    builder_add_box( 1.5f, 0.0f,  4.0f,  3.0f, 4.0f,  5.5f, c_dark, c_dark, c_dark);
    // Head & Snout
    builder_add_box(-2.5f, 7.0f, 5.0f, 2.5f, 12.0f, 9.5f, c_body, c_body, c_dark);
    builder_add_box(-1.8f, 7.0f, 9.5f, 1.8f, 10.0f, 13.0f, c_dark, c_dark, c_dark);
    // Ears
    builder_add_box(-2.5f, 12.0f, 6.0f, -1.0f, 14.5f, 7.5f, c_body, c_body, c_dark);
    builder_add_box( 1.0f, 12.0f, 6.0f,  2.5f, 14.5f, 7.5f, c_body, c_body, c_dark);
    // Glowing Eyes
    builder_add_box(-2.2f, 9.5f, 8.5f, -0.8f, 10.5f, 9.2f, c_eye, c_eye, c_eye);
    builder_add_box( 0.8f, 9.5f, 8.5f,  2.2f, 10.5f, 9.2f, c_eye, c_eye, c_eye);

    return builder_finish();
}

// Build Dead Twisted Tree
static Mesh build_tree_dead(void) {
    builder_reset();
    uint32_t c_bark1 = 0x36271DFF;
    uint32_t c_bark2 = 0x221812FF;

    // Base trunk
    builder_add_box(-5.0f, 0.0f, -5.0f, 5.0f, 18.0f, 5.0f, c_bark1, c_bark2, c_bark2);
    // Tapering twisted middle
    builder_add_box(-4.0f, 18.0f, -3.5f, 3.5f, 34.0f, 4.0f, c_bark1, c_bark1, c_bark2);
    // Upper gnarled crown
    builder_add_box(-3.0f, 34.0f, -3.0f, 2.5f, 46.0f, 2.5f, c_bark2, c_bark1, c_bark2);
    // Twisted branches
    builder_add_box(-12.0f, 36.0f, -2.0f, -2.0f, 40.0f, 2.0f, c_bark1, c_bark2, c_bark2);
    builder_add_box(  2.0f, 38.0f, -2.0f, 13.0f, 42.0f, 2.0f, c_bark1, c_bark2, c_bark2);
    builder_add_box(-2.0f, 42.0f, -10.0f, 2.0f, 46.0f, -2.0f, c_bark1, c_bark2, c_bark2);
    builder_add_box(-2.0f, 40.0f,   2.0f, 2.0f, 44.0f, 11.0f, c_bark1, c_bark2, c_bark2);

    return builder_finish();
}

// Build Gravestone
static Mesh build_gravestone(void) {
    builder_reset();
    uint32_t c_stone = 0x585B62FF;
    uint32_t c_dark  = 0x3A3C42FF;
    uint32_t c_moss  = 0x364834FF;

    // Stone plinth base
    builder_add_box(-8.0f, 0.0f, -4.0f, 8.0f, 3.5f, 4.0f, c_moss, c_stone, c_dark);
    // Upright headstone slab
    builder_add_box(-6.5f, 3.5f, -2.5f, 6.5f, 18.0f, 2.5f, c_stone, c_stone, c_dark);
    // Arch / Peak
    builder_add_box(-5.0f, 18.0f, -2.2f, 5.0f, 21.5f, 2.2f, c_stone, c_stone, c_dark);
    // Gothic carved cross relief
    builder_add_box(-1.0f, 7.0f, 2.4f, 1.0f, 17.0f, 2.9f, c_dark, c_dark, c_dark);
    builder_add_box(-3.5f, 12.0f, 2.4f, 3.5f, 14.5f, 2.9f, c_dark, c_dark, c_dark);

    return builder_finish();
}

// Build Chapel Arch / Crypt Entrance
static Mesh build_chapel_arch(void) {
    builder_reset();
    uint32_t c_stone = 0x50545CFF;
    uint32_t c_dark  = 0x32343AFF;
    uint32_t c_iron  = 0x1A1C20FF;

    // Left stone column
    builder_add_box(-22.0f, 0.0f, -4.0f, -14.0f, 36.0f, 4.0f, c_stone, c_stone, c_dark);
    // Right stone column
    builder_add_box( 14.0f, 0.0f, -4.0f,  22.0f, 36.0f, 4.0f, c_stone, c_stone, c_dark);
    // Lintel / Upper Arch
    builder_add_box(-24.0f, 36.0f, -5.0f, 24.0f, 44.0f, 5.0f, c_stone, c_stone, c_dark);
    // Arch Peak
    builder_add_box(-8.0f, 44.0f, -4.0f, 8.0f, 50.0f, 4.0f, c_stone, c_stone, c_dark);

    // Decorative iron portcullis / gateway grate
    builder_add_box(-14.0f, 0.0f, -1.0f, 14.0f, 2.0f, 1.0f, c_iron, c_iron, c_iron);
    builder_add_box(-14.0f, 0.0f, -1.0f, -12.5f, 32.0f, 1.0f, c_iron, c_iron, c_iron);
    builder_add_box( 12.5f, 0.0f, -1.0f,  14.0f, 32.0f, 1.0f, c_iron, c_iron, c_iron);

    return builder_finish();
}

// Build Crypt Pillar
static Mesh build_crypt_pillar(void) {
    builder_reset();
    uint32_t c_stone = 0x4B4E54FF;
    uint32_t c_dark  = 0x2E3035FF;
    uint32_t c_gold  = 0x7A6A32FF;

    // Base pedestal
    builder_add_box(-6.0f, 0.0f, -6.0f, 6.0f, 5.0f, 6.0f, c_stone, c_stone, c_dark);
    // Main column shaft
    builder_add_box(-4.5f, 5.0f, -4.5f, 4.5f, 36.0f, 4.5f, c_stone, c_dark, c_dark);
    // Capital / Head
    builder_add_box(-6.5f, 36.0f, -6.5f, 6.5f, 42.0f, 6.5f, c_gold, c_stone, c_dark);
    // Wall sconce torch holder
    builder_add_box(-1.0f, 22.0f, 4.5f, 1.0f, 25.0f, 7.5f, 0x1A1A1AFF, 0x1A1A1AFF, 0x1A1A1AFF);
    builder_add_box(-1.5f, 24.5f, 6.5f, 1.5f, 27.5f, 9.5f, 0xFF6600FF, 0xFF4400FF, 0xFF2200FF);

    return builder_finish();
}

// Build Crypt Wall segment
static Mesh build_crypt_wall(void) {
    builder_reset();
    uint32_t c_brick1 = 0x4A4D53FF;
    uint32_t c_brick2 = 0x36383DFF;
    uint32_t c_trim   = 0x2A2C30FF;

    // Main wall body
    builder_add_box(-20.0f, 0.0f, -3.0f, 20.0f, 36.0f, 3.0f, c_brick1, c_brick2, c_trim);
    // Stone base ledge
    builder_add_box(-21.0f, 0.0f, -4.0f, 21.0f, 4.0f, 4.0f, c_trim, c_brick1, c_trim);
    // Top cornice
    builder_add_box(-21.0f, 34.0f, -4.0f, 21.0f, 38.0f, 4.0f, c_trim, c_brick1, c_trim);

    return builder_finish();
}

// Build Profane Altar
static Mesh build_altar(void) {
    builder_reset();
    uint32_t c_stone = 0x3B3D44FF;
    uint32_t c_blood = 0x8A0E1EFF;
    uint32_t c_bone  = 0xC2BCA8FF;

    // Step base
    builder_add_box(-15.0f, 0.0f, -10.0f, 15.0f, 3.0f, 10.0f, c_stone, c_stone, c_stone);
    // Altar pedestal
    builder_add_box(-12.0f, 3.0f, -8.0f, 12.0f, 11.0f, 8.0f, c_stone, c_stone, c_stone);
    // Sacrificial table top with blood basin
    builder_add_box(-14.0f, 11.0f, -9.0f, 14.0f, 14.0f, 9.0f, c_stone, c_stone, c_stone);
    builder_add_box(-11.0f, 13.8f, -6.5f, 11.0f, 14.3f, 6.5f, c_blood, c_blood, c_blood);
    // Skull ornament
    builder_add_box(-2.5f, 14.0f, -2.5f, 2.5f, 18.0f, 2.5f, c_bone, c_bone, c_bone);

    return builder_finish();
}

// Build Sarcophagus
static Mesh build_sarcophagus(void) {
    builder_reset();
    uint32_t c_stone = 0x54575EFF;
    uint32_t c_dark  = 0x35383FFF;

    // Tomb tub
    builder_add_box(-12.0f, 0.0f, -6.0f, 12.0f, 8.0f, 6.0f, c_dark, c_stone, c_dark);
    // Heavy carved lid
    builder_add_box(-13.0f, 8.0f, -7.0f, 13.0f, 12.0f, 7.0f, c_stone, c_stone, c_dark);
    // Effigy carving
    builder_add_box(-9.0f, 12.0f, -3.5f, 9.0f, 13.5f, 3.5f, c_dark, c_dark, c_dark);

    return builder_finish();
}

// Build Treasure Chest
static Mesh build_chest(void) {
    builder_reset();
    uint32_t c_wood = 0x483222FF;
    uint32_t c_iron = 0x2A2E35FF;
    uint32_t c_gold = 0xD4AF37FF;

    // Bottom container
    builder_add_box(-6.0f, 0.0f, -4.5f, 6.0f, 6.5f, 4.5f, c_wood, c_wood, c_iron);
    // Rounded / arched lid
    builder_add_box(-6.5f, 6.5f, -5.0f, 6.5f, 10.5f, 5.0f, c_iron, c_wood, c_wood);
    // Gold latch / skull lock
    builder_add_box(-1.5f, 4.5f, 4.4f, 1.5f, 7.5f, 5.2f, c_gold, c_gold, c_gold);

    return builder_finish();
}

// Build Spirit Bolt (Glowing diamond/octahedron orb)
static Mesh build_spirit_bolt(void) {
    builder_reset();
    uint32_t c_cyan = 0x00FFFFFF;
    uint32_t c_core = 0xE0FFFFFF;

    // Central core
    builder_add_box(-2.5f, -2.5f, -2.5f, 2.5f, 2.5f, 2.5f, c_core, c_cyan, c_cyan);
    // Trailing fins / glow
    builder_add_box(-1.5f, -1.5f, -5.0f, 1.5f, 1.5f, -2.5f, c_cyan, c_cyan, c_cyan);
    builder_add_box(-4.0f, -0.8f, -0.8f, 4.0f, 0.8f, 0.8f, c_cyan, c_cyan, c_cyan);
    builder_add_box(-0.8f, -4.0f, -0.8f, 0.8f, 4.0f, 0.8f, c_cyan, c_cyan, c_cyan);

    return builder_finish();
}

// Build Thorn Spike (Cursed briar bursting from ground)
static Mesh build_thorn_spike(void) {
    builder_reset();
    uint32_t c_stem  = 0x241C1AFF;
    uint32_t c_thorn = 0x7E1825FF;
    uint32_t c_tip   = 0xC41E3AFF;

    // Base stump
    builder_add_box(-3.5f, 0.0f, -3.5f, 3.5f, 4.0f, 3.5f, c_stem, c_stem, c_stem);
    // Main sharp central spire
    builder_add_box(-2.0f, 4.0f, -2.0f, 2.0f, 16.0f, 2.0f, c_thorn, c_stem, c_stem);
    builder_add_box(-1.0f, 16.0f, -1.0f, 1.0f, 24.0f, 1.0f, c_tip, c_tip, c_thorn);
    // Angled side barbs
    builder_add_box(-5.5f, 7.0f, -1.5f, -1.5f, 11.0f, 1.5f, c_thorn, c_thorn, c_tip);
    builder_add_box( 1.5f, 9.0f, -1.5f,  6.0f, 13.0f, 1.5f, c_thorn, c_thorn, c_tip);

    return builder_finish();
}

// Build Dark Blood Orb (Cultist / Boss projectile)
static Mesh build_dark_orb(void) {
    builder_reset();
    uint32_t c_blood  = 0xE01020FF;
    uint32_t c_shadow = 0x480824FF;

    builder_add_box(-2.5f, -2.5f, -2.5f, 2.5f, 2.5f, 2.5f, c_blood, c_shadow, c_shadow);
    builder_add_box(-1.5f, -1.5f, -4.0f, 1.5f, 1.5f, 4.0f, c_shadow, c_blood, c_blood);

    return builder_finish();
}

// ---- Partes do humanoide articulado (caixa simples em torno do pivo da junta) ----
static Mesh build_part_torso(void) {
    builder_reset();
    builder_add_prism(0, 0, 0.0f, 11.0f, 2.4f, 1.6f, 2.7f, 1.8f, 8, 0x9A8C7AFF, 0x80725EFF, 0x5E5449FF);
    return builder_finish();
}
static Mesh build_part_head(void) {
    builder_reset();
    builder_add_prism(0, 0, 0.0f, 5.8f, 2.2f, 2.2f, 1.7f, 1.7f, 8, 0xA89A86FF, 0x8E806CFF, 0x6A5F50FF);
    // nariz/viseira na FRENTE (+Z): define a direcao que o boneco encara
    builder_add_box_p(-0.7f, 2.2f, 1.5f, 0.7f, 3.8f, 2.7f, 0x5A4F42FF, 0x5A4F42FF, 0x4A4034FF);
    return builder_finish();
}
static Mesh build_part_arm_upper(void) {
    builder_reset();
    builder_add_prism(0, 0, -6.5f, 0.0f, 1.25f, 1.25f, 1.05f, 1.05f, 6, 0x90826FFF, 0x7A6C59FF, 0x5A5045FF);
    return builder_finish();
}
static Mesh build_part_arm_lower(void) {
    builder_reset();
    builder_add_prism(0, 0, -6.0f, 0.0f, 1.05f, 1.05f, 0.85f, 0.85f, 6, 0x8A7C69FF, 0x746653FF, 0x564C41FF);
    return builder_finish();
}
static Mesh build_part_leg_upper(void) {
    builder_reset();
    builder_add_prism(0, 0, -7.0f, 0.0f, 1.45f, 1.45f, 1.2f, 1.2f, 6, 0x90826FFF, 0x7A6C59FF, 0x5A5045FF);
    return builder_finish();
}
static Mesh build_part_leg_lower(void) {
    builder_reset();
    builder_add_prism(0, 0, -7.0f, 0.0f, 1.2f, 1.2f, 1.0f, 1.0f, 6, 0x8A7C69FF, 0x746653FF, 0x564C41FF);
    return builder_finish();
}

void models_init(void) {
    debugf("[MODELS] Building procedural 3D gothic meshes...\n");

    mesh_druid_human    = build_druid_human();
    mesh_druid_wolf     = build_druid_wolf();
    mesh_skeleton       = build_skeleton();
    mesh_demon          = build_demon();
    mesh_cultist        = build_cultist();
    mesh_boss_malakor   = build_boss_malakor();
    mesh_spirit_wolf    = build_spirit_wolf();

    mesh_part_torso     = build_part_torso();
    mesh_part_head      = build_part_head();
    mesh_part_arm_upper = build_part_arm_upper();
    mesh_part_arm_lower = build_part_arm_lower();
    mesh_part_leg_upper = build_part_leg_upper();
    mesh_part_leg_lower = build_part_leg_lower();

    mesh_tree_dead      = build_tree_dead();
    mesh_gravestone     = build_gravestone();
    mesh_chapel_arch    = build_chapel_arch();
    mesh_crypt_pillar   = build_crypt_pillar();
    mesh_crypt_wall     = build_crypt_wall();
    mesh_altar          = build_altar();
    mesh_sarcophagus    = build_sarcophagus();
    mesh_chest          = build_chest();

    mesh_spirit_bolt    = build_spirit_bolt();
    mesh_thorn_spike    = build_thorn_spike();
    mesh_dark_orb       = build_dark_orb();

    debugf("[MODELS] All meshes initialized successfully!\n");
}

void models_draw_mesh(const Mesh *mesh) {
    if (!mesh || !mesh->verts || mesh->vert_count == 0) return;

    // Carrega vertices em blocos; o Tiny3D exige t3d_tri_sync antes de cada
    // novo t3d_vert_load e apos o ultimo batch (senao corrompe no RSP real).
    uint32_t remaining = mesh->vert_count;
    uint32_t offset = 0;

    while (remaining > 0) {
        uint32_t chunk = (remaining > 60) ? 60 : remaining;
        if (offset > 0) t3d_tri_sync();
        t3d_vert_load(&mesh->verts[offset / 2], 0, chunk);
        t3d_tri_draw_unindexed(0, chunk / 3);
        offset += chunk;
        remaining -= chunk;
    }
    t3d_tri_sync();
}

void models_draw_mesh_tinted(const Mesh *mesh, uint32_t color_override) {
    (void)color_override;
    // For hit flashes (e.g. white flash when taking damage)
    // T3D lighting exposure or vertex color can be drawn
    models_draw_mesh(mesh);
}
