#ifndef VEIL_CEMETERY_STAGE_H
#define VEIL_CEMETERY_STAGE_H
#include "common.h"
void cemetery_stage_build(const char *const *grid,int rows,int cols);
// The second map uses the same batching/culling engine with a separate kit.
void crypt_stage_build(const char *const *grid,int rows,int cols);
void altar_stage_build(const char *const *grid,int rows,int cols);
void forest_stage_build(const char *const *grid,int rows,int cols);
void swamp_stage_build(const char *const *grid,int rows,int cols);
void heart_stage_build(const char *const *grid,int rows,int cols);
void snow_stage_build(const char *const *grid,int rows,int cols);
void ice_stage_build(const char *const *grid,int rows,int cols);
void jarl_stage_build(const char *const *grid,int rows,int cols);
void expansion_stage_build(const char *const *grid,int rows,int cols,int campaign,int area);
float ice_stage_grip(float x,float z);
typedef enum { SNOW_PROP_ROCK_A,SNOW_PROP_ROCK_B,SNOW_PROP_ROCK_C,SNOW_PROP_RUIN,SNOW_PROP_COUNT } SnowProp;
bool snow_stage_place(SnowProp type,float x,float z,float yaw,float scale);
typedef enum { SWAMP_PROP_TREE_A,SWAMP_PROP_TREE_B,SWAMP_PROP_TREE_C,SWAMP_PROP_ROOTS,
               SWAMP_PROP_LOG,SWAMP_PROP_WALKWAY,SWAMP_PROP_SHELTER,SWAMP_PROP_FENCE,SWAMP_PROP_COUNT } SwampProp;
bool swamp_stage_place(SwampProp type,float x,float z,float yaw);
bool swamp_stage_pool(float x,float z,float radius,bool mud);
float swamp_stage_speed(float x,float z);
typedef enum { FOREST_PROP_TREE_A, FOREST_PROP_TREE_B, FOREST_PROP_TREE_C,
               FOREST_PROP_LOG, FOREST_PROP_STUMP, FOREST_PROP_STONE, FOREST_PROP_BROKEN_FENCE, FOREST_PROP_COUNT } ForestProp;
bool forest_stage_place(ForestProp type,float x,float z,float yaw,float scale);
bool forest_stage_spot_free(float x,float z,float radius);
bool forest_stage_web(float x,float z,float width);
typedef enum { ALT_PROP_PYLON, ALT_PROP_CAGE, ALT_PROP_THRONE, ALT_PROP_CHAIN, ALT_PROP_COUNT } AltarProp;
bool altar_stage_place(AltarProp type,float x,float z,float yaw);
typedef enum { ALT_PAINT_RITUAL,ALT_PAINT_ASH,ALT_PAINT_BONES,ALT_PAINT_BLOOD } AltarPaint;
int altar_stage_paint(AltarPaint type,float x,float z,float width);
void altar_stage_paint_set(int handle,AltarPaint type);
typedef enum { CRYPT_PROP_PILLAR, CRYPT_PROP_SARCOPHAGUS, CRYPT_PROP_ALTAR, CRYPT_PROP_ARCH,
               CRYPT_PROP_RUBBLE_A, CRYPT_PROP_RUBBLE_B, CRYPT_PROP_FALLEN_COLUMN, CRYPT_PROP_COUNT } CryptProp;
bool crypt_stage_place(CryptProp type,float x,float z,float yaw);
void cemetery_stage_release(void);
bool cemetery_stage_ready(void);
float cemetery_stage_ground_y(float x,float z,float width);
typedef enum { CEM_GRAVE, CEM_CROSS, CEM_FENCE, CEM_CRYPT } CemeteryProp;
// Place shared geometry before rendering starts. The world owns matching collision.
bool cemetery_stage_place(CemeteryProp type, float x, float z, float yaw);
typedef enum { CEM_DECAL_OPEN_GRAVE, CEM_DECAL_BONES, CEM_DECAL_BLOOD, CEM_DECAL_CRACKS, CEM_DECAL_COUNT } CemeteryDecal;
// Existing painted artwork on a real, flat quad; no screen-space depth override.
bool cemetery_stage_decal(CemeteryDecal type, float x, float z, float width);
void cemetery_stage_draw(float x, float z);
typedef struct {
    uint32_t queue_us, patches, objects, triangles, textures, cull_tests;
    uint32_t total_patches, total_objects;
    uint32_t shared_posts;
} CemeteryPerf;
const CemeteryPerf *cemetery_stage_perf(void);
#endif
