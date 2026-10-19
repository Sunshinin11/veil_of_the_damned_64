#ifndef WORLD_H
#define WORLD_H

#include "common.h"
#include "models.h"

#define MAX_COLLIDERS 256
#define MAX_SCENE_PROPS 520

typedef enum {
    SCENE_TREE_DEAD, SCENE_GRAVESTONE, SCENE_CHAPEL_ARCH,
    SCENE_CAMPFIRE, SCENE_TRAVELER_BODY, SCENE_CRYPT_BRAZIER,
    SCENE_CULTIST_BODY, SCENE_STAINED_GLASS, SCENE_BLACK_ALTAR,
    SCENE_MALAKOR_THRONE, SCENE_PENTAGRAM, SCENE_MONOLITH,
    SCENE_INFERNAL_BRAZIER, SCENE_WALL_PATH, SCENE_WALL_CRYPT,
    SCENE_WALL_ALTAR, SCENE_PORTAL_PATH, SCENE_PORTAL_CRYPT,
    SCENE_PORTAL_ALTAR, SCENE_CRYPT_CHAINS, SCENE_GRAVE_FENCE,
    SCENE_PATH_SHRINE, SCENE_CRYPT_PILLAR, SCENE_SARCOPHAGUS,
    SCENE_CRYPT_ALTAR, SCENE_GREAT_TREE, SCENE_GRAVE_CROSS,
    SCENE_TOMB_STATUE, SCENE_CRYPT_ENTRANCE, SCENE_DEAD_BUSH,
    SCENE_BONE_PILE, SCENE_COBWEB, SCENE_CELL_BARS, SCENE_SKULL_NICHE,
    SCENE_WALL_CHAINS,
    SCENE_DECAL_BLOOD, SCENE_DECAL_BONES, SCENE_DECAL_CRACKS, SCENE_DECAL_GRAVE,
    SCENE_STALAGMITE, SCENE_CAVE_ROCKS, SCENE_CRYSTAL,
    // Cripta/Caverna (mapa 2): arte das pranchetas Sprites/Cripta_Caverna (tools/import_cripta_caverna.py)
    SCENE_CAVE_STALAG_A, SCENE_CAVE_STALAG_B, SCENE_CAVE_STALAG_C, SCENE_CAVE_ROCKS_A, SCENE_CAVE_ROCKS_B, SCENE_CAVE_ROCKS_C, SCENE_CAVE_ROCKS_D, SCENE_CAVE_CRYSTAL_A, SCENE_CAVE_CRYSTAL_B, SCENE_CAVE_CRYSTAL_C, SCENE_CAVE_MUSH_A, SCENE_CAVE_MUSH_B, SCENE_CAVE_MUSH_C, SCENE_CAVE_SARCO_A, SCENE_CAVE_SARCO_B, SCENE_CAVE_ALTAR, SCENE_CAVE_BRAZIER, SCENE_CAVE_STATUE_A, SCENE_CAVE_STATUE_B, SCENE_CAVE_BONES_A, SCENE_CAVE_BONES_B, SCENE_CAVE_CORPSE, SCENE_CAVE_RIBCAGE, SCENE_CAVE_DECAL_BLOOD, SCENE_CAVE_DECAL_BONES, SCENE_CAVE_DECAL_CRACKS, SCENE_CAVE_DECAL_GRAVE, SCENE_CAVE_DECAL_WATER, SCENE_CAVE_DECAL_WEB, SCENE_CAVE_PORTAL, SCENE_CAVE_ENTRANCE, SCENE_CAVE_THRONE,
    // Altar Negro / inferno (mapa 3): Sprites/Altar_Negro (tools/import_altar.py)
    SCENE_HELL_SEAL_ON, SCENE_HELL_SEAL_OFF, SCENE_HELL_ROCKS_A, SCENE_HELL_ROCKS_B, SCENE_HELL_ROCKS_C, SCENE_HELL_ROCKS_D, SCENE_HELL_SPIKES_A, SCENE_HELL_SPIKES_B, SCENE_HELL_SPIKES_C, SCENE_HELL_FIREVENT, SCENE_HELL_BRAZIER, SCENE_HELL_HANGED, SCENE_HELL_SKULLS, SCENE_HELL_CAGE, SCENE_HELL_STATUE, SCENE_HELL_THRONE, SCENE_HELL_DECAL_LAVA, SCENE_HELL_DECAL_CRACK, SCENE_HELL_DECAL_RITUAL, SCENE_HELL_DECAL_BONES, SCENE_HELL_DECAL_ASH, SCENE_HELL_DECAL_BLOOD, SCENE_HELL_LAVA_POOL, SCENE_HELL_PORTAL, SCENE_VENDOR,
    // Floresta Morta: Pantano Podre (fase 2) e Coracao Seco (fase 3) - tools/import_forest_stages.py
    SCENE_SWAMP_TREE_A, SCENE_SWAMP_TREE_B, SCENE_SWAMP_TREE_C, SCENE_SWAMP_REEDS_A, SCENE_SWAMP_REEDS_B, SCENE_SWAMP_SHROOM, SCENE_SWAMP_LOG, SCENE_SWAMP_BOAT, SCENE_SWAMP_TOTEM, SCENE_SWAMP_HUT, SCENE_SWAMP_BONES, SCENE_SWAMP_LANTERN, SCENE_SWAMP_DECAL_POOL, SCENE_SWAMP_DECAL_POOL_S, SCENE_SWAMP_DECAL_MUD, SCENE_SWAMP_DECAL_LILY, SCENE_SWAMP_DECAL_ALGAE, SCENE_SWAMP_DECAL_BONES, SCENE_FOREST_PORTAL, SCENE_HEART_TOTEM_ON, SCENE_HEART_TOTEM_OFF, SCENE_HEART_ROOTS_A, SCENE_HEART_ROOTS_B, SCENE_HEART_ROOTS_C, SCENE_HEART_THORNS_A, SCENE_HEART_THORNS_B, SCENE_HEART_THORNS_C, SCENE_HEART_FUNGUS, SCENE_HEART_COCOON, SCENE_HEART_SKULLS, SCENE_HEART_STONE, SCENE_HEART_CARCASS, SCENE_HEART_THRONE, SCENE_HEART_SPORE, SCENE_HEART_DECAL_POISON, SCENE_HEART_DECAL_CRACK, SCENE_HEART_DECAL_CIRCLE, SCENE_HEART_DECAL_LEAVES, SCENE_HEART_DECAL_BONES, SCENE_HEART_DECAL_SAP, SCENE_HEART_POISON_POOL,
    // O Norte Esquecido: Passo Congelado, Cavernas de Gelo e Trono do Jarl - tools/import_north.py
    SCENE_SNOW_PINE_A, SCENE_SNOW_PINE_B, SCENE_SNOW_PINE_C, SCENE_SNOW_ROCK_A, SCENE_SNOW_ROCK_B, SCENE_SNOW_RUNESTONE, SCENE_SNOW_CART, SCENE_SNOW_CORPSE, SCENE_SNOW_BANNER, SCENE_SNOW_PALISADE, SCENE_SNOW_BONES, SCENE_SNOW_FIRE, SCENE_SNOW_DECAL_ICE, SCENE_SNOW_DECAL_DRIFT, SCENE_SNOW_DECAL_BLOOD, SCENE_SNOW_DECAL_TRACKS, SCENE_SNOW_DECAL_FROZEN, SCENE_SNOW_DECAL_BONES, SCENE_ICE_PORTAL, SCENE_ICE_STALAG_A, SCENE_ICE_STALAG_B, SCENE_ICE_STALAG_C, SCENE_ICE_CRYSTAL_A, SCENE_ICE_CRYSTAL_B, SCENE_ICE_CRYSTAL_C, SCENE_ICE_FROZEN_WARRIOR, SCENE_ICE_FROZEN_WOLF, SCENE_ICE_MAMMOTH, SCENE_ICE_BONES, SCENE_ICE_BRAZIER, SCENE_ICE_DECAL_CRACK, SCENE_ICE_DECAL_FROZEN, SCENE_ICE_DECAL_GLOW, SCENE_ICE_DECAL_BONES, SCENE_ICE_DECAL_SNOW, SCENE_ICE_DECAL_BLOOD, SCENE_FROST_SEAL_ON, SCENE_FROST_SEAL_OFF, SCENE_FROST_SPIKES_A, SCENE_FROST_SPIKES_B, SCENE_FROST_SPIKES_C, SCENE_FROST_ROCKS_A, SCENE_FROST_ROCKS_B, SCENE_FROST_ROCKS_C, SCENE_FROST_VENT, SCENE_FROST_HANGED, SCENE_FROST_SKULLS, SCENE_FROST_STATUE, SCENE_FROST_CAGE, SCENE_FROST_THRONE, SCENE_FROST_BRAZIER, SCENE_FROST_DECAL_ICE, SCENE_FROST_DECAL_CRACK, SCENE_FROST_DECAL_CIRCLE, SCENE_FROST_DECAL_BONES, SCENE_FROST_DECAL_SNOW, SCENE_FROST_DECAL_BLOOD, SCENE_FROST_POOL,
    SCENE_PROP_COUNT
} ScenePropId;

typedef struct {
    ScenePropId id;
    float x, y, z, height;
} SceneProp;

typedef enum {
    COL_CIRCLE,
    COL_BOX
} ColliderType;

typedef struct {
    ColliderType type;
    float x, z;
    float r;         // For circle
    float hx, hz;    // Half-extents for box
} WorldCollider;

typedef struct {
    AreaId id;
    const char *name;
    const char *subtitle;

    // Player spawn
    float spawn_x, spawn_z;
    float spawn_yaw;

    // Exit portal
    float exit_x, exit_z;
    float exit_radius;
    bool  exit_unlocked;

    // Dimensions
    float min_x, max_x;
    float min_z, max_z;

    // Atmospheric colors
    color_t clear_color;
    color_t fog_color;
    float fog_near, fog_far;
    uint8_t ambient_light[4];
    uint8_t dir_light_color[4];
    T3DVec3 dir_light_vec;
} AreaData;

extern AreaId g_current_area;
extern AreaData g_area_data[AREA_COUNT];

void world_init(void);
void world_load_area(AreaId area);
void world_update(float dt);
void world_draw(void);
const SceneProp *world_scene_props(int *count);

// Muros 3D (tiles '#'): usado pelos sprites para a oclusao estilo Diablo.
bool world_wall_info(float *half_w, float *half_h, float *tile, float *height, int *cols, int *rows);
bool world_wall_tile(int col, int row);

// Minimapa (grade explorada pelo druida)
void world_draw_minimap(void);
void world_minimap_toggle(void);

// Mascate: perto dele, A abre a loja
bool world_near_vendor(void);
int  world_load_serial(void);
float world_ground_speed(float x, float z);   // 0.6 dentro da agua do pantano, 1 fora
float world_ground_grip(float x, float z);    // 0.18 no gelo liso (escorrega), 1 fora
float world_vision_scale(void);               // nevasca do Passo Congelado fecha a visao
uint32_t world_scene_tint(void);    // tinta da campanha para o cenario (0 = nenhuma)
uint32_t world_monster_tint(void);  // tinta da campanha para os monstros (0 = nenhuma)   // muda a cada fase carregada (limites do mascate)

// Altar Negro (mapa 3): selos, filtro vermelho e chefe
bool world_boss_summoned(void);
void world_draw_screen_filter(void);
void world_draw_objectives(void);

// Collision testing
bool world_collide_circle(float *x, float *z, float radius);
bool world_check_exit(float x, float z);
void world_unlock_exit(void);

#endif // WORLD_H
