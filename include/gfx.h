#ifndef GFX_H
#define GFX_H

#include "common.h"

// ---------------------------------------------------------------------------
// Camada fina sobre o Tiny3D para corrigir dois problemas de hardware real:
//  1) Matrizes de modelo precisam estar em memoria SEM cache quando o RSP as
//     le via DMA. Guardamos um anel de matrizes uncached e copiamos para ele
//     antes de cada push (o Ares nao simula cache, por isso o bug so aparecia
//     no N64 real).
//  2) Helpers de texto/UI que convertem acentos PT-BR para a fonte embutida.
// ---------------------------------------------------------------------------

// Flags de depuracao controladas pelo D-pad no jogo.
extern uint8_t g_dbg_cull;  // 0 = nenhum, 1 = CULL_BACK, 2 = CULL_FRONT
extern bool    g_dbg_fog;   // liga/desliga neblina
extern bool    g_dbg_hud;   // mostra a linha de FPS/debug

void gfx_init(void);

// Copia a matriz fixa (que pode estar em struct com cache) para o anel
// uncached e empilha. Pareie com t3d_matrix_pop(1).
void gfx_matrix_push(const T3DMat4FP *src);

// Empilha a matriz identidade uncached (usar antes do piso/geometria estatica).
void gfx_push_identity(void);
// Empilha matriz local de uma junta (translacao+rotacao) multiplicando com o pai.
void gfx_push_part(const float pos[3], const float rot[3]);

// Flag de culling atual conforme g_dbg_cull.
uint32_t gfx_cull_flag(void);

// --- Texto / UI -------------------------------------------------------------
// Converte uma string UTF-8 PT-BR para ASCII (remove acentos) na fonte embutida.
void ui_ascii(char *dst, const char *src, int cap);
// Imprime texto (seguro p/ acentos) na fonte embutida de debug.
void ui_text(int x, int y, const char *s);
// Imprime centralizado horizontalmente em 320px.
void ui_text_center(int y, const char *s);
// printf-style.
void ui_textf(int x, int y, const char *fmt, ...);

// Fontes goticas + estilos de cor
enum { UI_STYLE_NORMAL = 0, UI_STYLE_SELECTED = 1, UI_STYLE_SHADOW = 2, UI_STYLE_DISABLED = 3, UI_STYLE_GOLD = 4 };
void ui_fonts_init(void);
// big: fonte grande; align 0 esq / 1 centro / 2 dir (centro/dir usam a caixa x..x+w)
void ui_print(bool big, int x, int y, int style, int align, int w, const char *s);

#endif // GFX_H
