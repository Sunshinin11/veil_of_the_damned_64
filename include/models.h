#ifndef MODELS_H
#define MODELS_H

#include "common.h"

// Precisao sub-inteira dos vertices (malha autorada em unidades reais x MESH_PREC;
// a raiz do personagem aplica a escala 1/MESH_PREC). Deixa prismas finos lisos.
#define MESH_PREC 4

// Mesh definition
typedef struct {
    T3DVertPacked *verts;
    uint32_t vert_count;
    uint32_t tri_count;
} Mesh;

// Global mesh registry
extern Mesh mesh_druid_human;
extern Mesh mesh_druid_wolf;
extern Mesh mesh_skeleton;
extern Mesh mesh_demon;
extern Mesh mesh_cultist;
extern Mesh mesh_boss_malakor;
extern Mesh mesh_spirit_wolf;

// Partes do boneco humanoide articulado (fase: geometria correta)
extern Mesh mesh_part_torso;
extern Mesh mesh_part_head;
extern Mesh mesh_part_arm_upper;
extern Mesh mesh_part_arm_lower;
extern Mesh mesh_part_leg_upper;
extern Mesh mesh_part_leg_lower;
extern Mesh mesh_druid3d_body;
extern Mesh mesh_druid3d_head;
extern Mesh mesh_druid3d_upper_arm;
extern Mesh mesh_druid3d_lower_arm;
extern Mesh mesh_druid3d_upper_leg;
extern Mesh mesh_druid3d_lower_leg;

extern Mesh mesh_tree_dead;
extern Mesh mesh_gravestone;
extern Mesh mesh_chapel_arch;
extern Mesh mesh_crypt_pillar;
extern Mesh mesh_crypt_wall;
extern Mesh mesh_altar;
extern Mesh mesh_sarcophagus;
extern Mesh mesh_chest;

extern Mesh mesh_spirit_bolt;
extern Mesh mesh_thorn_spike;
extern Mesh mesh_dark_orb;

void models_init(void);
void models_draw_mesh(const Mesh *mesh);
void models_draw_mesh_tinted(const Mesh *mesh, uint32_t color_override);

#endif // MODELS_H
