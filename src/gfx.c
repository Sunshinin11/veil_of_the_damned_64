#include "gfx.h"
#include <stdarg.h>

// Anel de matrizes SEM cache. Tamanho potencia de 2 para mascara rapida.
// Precisa comportar (matrizes por quadro) x (FB_COUNT) para que um slot so
// seja reutilizado depois que o RDP terminou de ler o quadro anterior.
#define GFX_RING 512
static T3DMat4FP *s_ring = NULL;
static uint32_t   s_cursor = 0;
static T3DMat4FP *s_identity = NULL;

uint8_t g_dbg_cull = 0;
bool    g_dbg_fog  = true;
bool    g_dbg_hud  = false;

void gfx_init(void) {
    s_ring = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP) * GFX_RING);
    s_identity = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));

    T3DMat4 id;
    t3d_mat4_identity(&id);
    t3d_mat4_to_fixed(s_identity, &id);
    debugf("[GFX] Anel de matrizes uncached: %d slots.\n", GFX_RING);
}

void gfx_matrix_push(const T3DMat4FP *src) {
    T3DMat4FP *slot = &s_ring[s_cursor & (GFX_RING - 1)];
    s_cursor++;
    // Escrita em memoria uncached vai direto para a RAM: o RSP le o dado certo.
    *slot = *src;
    t3d_matrix_push(slot);
}

void gfx_push_identity(void) {
    t3d_matrix_push(s_identity);
}

void gfx_push_part(const float pos[3], const float rot[3]) {
    T3DMat4FP tmp;
    float sc[3] = { 1.0f, 1.0f, 1.0f };
    t3d_mat4fp_from_srt_euler(&tmp, sc, rot, pos);
    gfx_matrix_push(&tmp); // copia p/ anel uncached e multiplica com o pai
}

uint32_t gfx_cull_flag(void) {
    if (g_dbg_cull == 1) return T3D_FLAG_CULL_BACK;
    if (g_dbg_cull == 2) return T3D_FLAG_CULL_FRONT;
    return 0;
}

// --- Texto ------------------------------------------------------------------
// Mapeia os acentos PT-BR mais comuns (UTF-8, 2 bytes iniciados por 0xC3)
// para a letra base. A fonte embutida de debug e apenas ASCII.
void ui_ascii(char *dst, const char *src, int cap) {
    int o = 0;
    for (const unsigned char *p = (const unsigned char*)src; *p && o < cap - 1; p++) {
        unsigned char c = *p;
        if (c == 0xC3 && p[1]) {
            unsigned char n = *(++p);
            char r = '?';
            switch (n) {
                case 0xA1: case 0xA0: case 0xA2: case 0xA3: case 0xA4: // a acentuado
                case 0x81: case 0x80: case 0x82: case 0x83: case 0x84: r = 'A'; break;
                case 0xA9: case 0xA8: case 0xAA: case 0xAB:            // e acentuado
                case 0x89: case 0x88: case 0x8A: case 0x8B:            r = 'E'; break;
                case 0xAD: case 0xAC: case 0xAE: case 0xAF:            // i acentuado
                case 0x8D: case 0x8C: case 0x8E: case 0x8F:            r = 'I'; break;
                case 0xB3: case 0xB2: case 0xB4: case 0xB5: case 0xB6: // o acentuado
                case 0x93: case 0x92: case 0x94: case 0x95: case 0x96: r = 'O'; break;
                case 0xBA: case 0xB9: case 0xBB: case 0xBC:            // u acentuado
                case 0x9A: case 0x99: case 0x9B: case 0x9C:            r = 'U'; break;
                case 0xA7: case 0x87:                                  r = 'C'; break; // c cedilha
                case 0xB1: case 0x91:                                  r = 'N'; break; // n til
                default: r = '?'; break;
            }
            dst[o++] = r;
        } else if (c == '^') {
            dst[o++] = '|'; // '^' = codigo de estilo no rdpq_text; neutraliza
        } else if (c == '$') {
            // '$' = codigo de troca de fonte no rdpq_text; descarta
        } else if (c < 0x80) {
            dst[o++] = (char)c;
        }
        // demais bytes multibyte sao ignorados
    }
    dst[o] = '\0';
}

// Fontes goticas (Pirata One): grande para titulos/menus, pequena para HUD e avisos.
// Se os arquivos nao estiverem na ROM, cai na fonte de debug (ASCII).
#define FONT_GOTHIC        2
#define FONT_GOTHIC_SMALL  3
static bool s_gothic = false;

static bool rom_has(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static void gothic_styles(rdpq_font_t *f) {
    rdpq_font_style(f, UI_STYLE_NORMAL,   &(rdpq_fontstyle_t){ .color = RGBA32(232, 220, 196, 255) });
    rdpq_font_style(f, UI_STYLE_SELECTED, &(rdpq_fontstyle_t){ .color = RGBA32(222, 46, 30, 255) });
    rdpq_font_style(f, UI_STYLE_SHADOW,   &(rdpq_fontstyle_t){ .color = RGBA32(0, 0, 0, 255) });
    rdpq_font_style(f, UI_STYLE_DISABLED, &(rdpq_fontstyle_t){ .color = RGBA32(110, 100, 92, 255) });
    rdpq_font_style(f, UI_STYLE_GOLD,     &(rdpq_fontstyle_t){ .color = RGBA32(214, 170, 82, 255) });
}

void ui_fonts_init(void) {
    if (rom_has("rom:/gothic_big.font64") && rom_has("rom:/gothic_small.font64")) {
        rdpq_font_t *big = rdpq_font_load("rom:/gothic_big.font64");
        rdpq_font_t *small = rdpq_font_load("rom:/gothic_small.font64");
        gothic_styles(big);
        gothic_styles(small);
        rdpq_text_register_font(FONT_GOTHIC, big);
        rdpq_text_register_font(FONT_GOTHIC_SMALL, small);
        s_gothic = true;
    }
}

// Remove os codigos de controle do rdpq_text; com a fonte gotica mantem os acentos (UTF-8).
static void ui_clean(char *dst, const char *src, int cap) {
    if (!s_gothic) { ui_ascii(dst, src, cap); return; }
    int o = 0;
    for (const char *p = src; *p && o < cap - 1; p++) {
        if (*p == '^' || *p == '$') continue;
        dst[o++] = *p;
    }
    dst[o] = '\0';
}

// Texto com sombra (1px) na fonte escolhida. big = titulo/menu. align: 0 esquerda, 1 centro, 2 direita
// (para 1/2, x e a borda esquerda da caixa de largura w).
void ui_print(bool big, int x, int y, int style, int align, int w, const char *s) {
    char buf[128];
    ui_clean(buf, s, sizeof(buf));
    if (!s_gothic) {
        rdpq_textparms_t p = { .width = (int16_t)(align ? w : 0), .align = align == 1 ? ALIGN_CENTER : align == 2 ? ALIGN_RIGHT : ALIGN_LEFT };
        rdpq_text_print(align ? &p : NULL, FONT_BUILTIN_DEBUG_MONO, x, y, buf);
        return;
    }
    int font = big ? FONT_GOTHIC : FONT_GOTHIC_SMALL;
    rdpq_textparms_t p = { .style_id = UI_STYLE_SHADOW, .width = (int16_t)(align ? w : 0),
                           .align = align == 1 ? ALIGN_CENTER : align == 2 ? ALIGN_RIGHT : ALIGN_LEFT };
    rdpq_text_print(&p, font, x + 1, y + 1, buf);
    p.style_id = (int16_t)style;
    rdpq_text_print(&p, font, x, y, buf);
}

void ui_text(int x, int y, const char *s) {
    ui_print(false, x, y, UI_STYLE_NORMAL, 0, 0, s);
}

void ui_text_center(int y, const char *s) {
    ui_print(false, 0, y, UI_STYLE_NORMAL, 1, SCREEN_WIDTH, s);
}

void ui_textf(int x, int y, const char *fmt, ...) {
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_print(false, x, y, UI_STYLE_NORMAL, 0, 0, buf);
}
