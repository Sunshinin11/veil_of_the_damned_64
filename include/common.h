#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Camera isometrica (fonte unica). A escala fixa dos sprites e calculada a
// partir destes valores, entao mexer aqui reescala todos os sprites juntos.
//
// CAM_K afasta a camera e fecha o FOV na mesma proporcao: o enquadramento no jogador fica igual,
// mas a perspectiva some (quase ortografica). Assim o chao e as paredes 3D mantem a MESMA escala
// em qualquer ponto da tela, e os sprites 2D (de tamanho fixo) deixam de parecer crescer ou
// encolher em relacao ao cenario. CAM_K = 1 volta a camera em perspectiva antiga.
#define CAM_K          6.0f
#define CAM_REF_FOV    0.95f     // camera de referencia: define o TAMANHO dos sprites (nao mexa)
#define CAM_REF_HEIGHT 118.0f
#define CAM_REF_BACK   92.0f
#define CAM_TARGET_Y   10.0f
#define CAM_FOV        (2.0f * atanf(tanf(CAM_REF_FOV * 0.5f) / CAM_K))
#define CAM_HEIGHT     (CAM_REF_HEIGHT * CAM_K)
#define CAM_BACK       (CAM_REF_BACK * CAM_K)
#define CAM_NEAR       (20.0f * CAM_K)
#define CAM_FAR        (500.0f * CAM_K)
#define CAM_FOG_OFFSET (149.6f * (CAM_K - 1.0f))   // a nevoa e medida a partir da camera

// Fixed delta time for consistent simulation
#define FIXED_DT (1.0f / 60.0f)

// Vector helper functions
static inline float vec2_dist_sq(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return dx * dx + dz * dz;
}

static inline float vec2_dist(float x1, float z1, float x2, float z2) {
    return sqrtf(vec2_dist_sq(x1, z1, x2, z2));
}

static inline float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline void t3d_mat4_rotate_y(T3DMat4 *mat, float angleRad) {
    t3d_mat4_rotate(mat, &(T3DVec3){{0, 1.0f, 0}}, angleRad);
}

// Game states
typedef enum {
    GAME_STATE_INTRO,
    GAME_STATE_PLAYING,
    GAME_STATE_AREA_TRANSITION,
    GAME_STATE_PLAYER_DEAD,
    GAME_STATE_VICTORY,
    GAME_STATE_CHAR_SELECT,   // escolha de personagem (depois de Novo Jogo)
    GAME_STATE_PAUSED,        // pausa (START)
    GAME_STATE_SHOP,          // mascate demoniaco (jogo pausado)
    GAME_STATE_CARD_OFFER     // carta encontrada: jogo pausa para pegar ou deixar
} GameState;

// Herois e campanhas
typedef enum { HERO_DRUID = 0, HERO_HUNTRESS = 1, HERO_BARBARIAN = 2, HERO_COUNT = 3 } HeroId;
extern int g_hero;       // heroi da partida (druid.c)
extern int g_campaign;   // 0 = Caminho dos Condenados, 1 = Floresta Morta (world.c)
#define CAMPAIGN_COUNT 3   // 0 Condenados (Druida), 1 Floresta Morta (Cacadora), 2 Norte Esquecido (Barbaro)
// Dificuldade da run: 0 Normal, 1 Pesadelo, 2 Inferno (liberadas zerando a campanha na anterior)
enum { DIFF_NORMAL = 0, DIFF_NIGHTMARE = 1, DIFF_HELL = 2, DIFF_COUNT = 3 };
extern int g_difficulty;
extern int g_seeds;      // Sementes do Bosque carregadas nesta run (0..3)

// Areas (fase dentro da campanha: 0 = ao ar livre, 1 = caverna, 2 = arena do chefe)
typedef enum {
    AREA_FORSAKEN_PATH = 0,
    AREA_HOLLOW_CRYPT  = 1,
    AREA_BLACK_ALTAR   = 2,
    AREA_COUNT         = 3
} AreaId;
#define STAGE_COUNT (AREA_COUNT * CAMPAIGN_COUNT)   // seletor de fase: campanha * 3 + fase

#endif // COMMON_H
