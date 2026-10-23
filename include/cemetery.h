#ifndef CEMETERY_H
#define CEMETERY_H
#include "common.h"
#include "mapgen.h"
// Visual-only renderer. Call after the original meshes and FloorQuads are built.
void cemetery_release(void);
void cemetery_build(const char *const *grid, int rows, int cols,
                    const T3DVertPacked *base, int base_quads,
                    const T3DVertPacked *floor, const FloorQuad *quads, int count,
                    const T3DVertPacked *walls, int wall_quads,
                    const T3DVertPacked *tops, int top_quads);
bool cemetery_ready(void);
void cemetery_draw(bool walls, float player_x, float player_z);
#endif
