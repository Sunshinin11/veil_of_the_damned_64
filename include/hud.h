#ifndef HUD_H
#define HUD_H

#include "common.h"

void hud_init(void);
void hud_show_banner(const char *title, const char *sub, float duration);
void hud_show_message(const char *msg, float duration);
void hud_update(float dt);
void hud_draw(void);       // caixas/barras 2D (rdpq_fill), enquanto anexado
void hud_draw_text(void);  // texto (rdpq_text), antes do rdpq_detach_show

#endif // HUD_H
