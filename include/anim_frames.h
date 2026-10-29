#ifndef ANIM_FRAMES_H
#define ANIM_FRAMES_H
// Quadros por ciclo de CORRIDA (1..16). Gerado por tools/export_druid_run.py conforme as
// pranchetas que existirem em Sprites/ ("Druid Run Left 12 Frames.png" etc.). Padrao: 4.
// Druida e lobisomem: versoes suaves "<anim>_s" (tools/densify_anims.py), mesmo numero em todas.
#define NF_PLAYER         12
#define NF_DR_MOVE_SIDE   12
#define NF_DR_MOVE_FRONT  12
#define NF_DR_MOVE_BACK   12
#define NF_BE_MOVE_SIDE   4
// Escala extra por vista da corrida (1.0 = folhas do export_druid_run, cabeca ~47 px).
// tools/import_photoshop_run.py reescreve estes valores para as pranchetas do Photoshop.
#define RUN_SCALE_SIDE   0.877f
#define RUN_SCALE_FRONT  0.971f
#define RUN_SCALE_BACK   0.911f
#define RUN_BACK_WIDTH   1.00f
#endif
