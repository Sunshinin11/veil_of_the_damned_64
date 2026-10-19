#include "world.h"
#include "lang.h"
#include "gfx.h"
#include "models.h"
#include "enemies.h"
#include "items.h"
#include "druid.h"
#include "spells.h"
#include "audio.h"
#include "hud.h"
#include "sprites.h"
#include "save.h"
#include "cards.h"
#include "mapgen.h"      // mapas 1 e 2 montados a cada partida (estilo Diablo II)

AreaId g_current_area = AREA_FORSAKEN_PATH;

int g_campaign = 0;
int g_difficulty = DIFF_NORMAL;
int g_seeds = 0;
static int s_load_serial = 0;
int world_load_serial(void) { return s_load_serial; }
// Tinta da campanha (multiplica chao, muros, cenario e monstros): a Floresta Morta reaproveita
// a arte das fases do druida pintada de musgo.
static float s_tr = 1.0f, s_tg = 1.0f, s_tb = 1.0f;
static uint32_t s_prop_tint = 0, s_mob_tint = 0;
static uint32_t tint32(uint32_t c) {
    float r = ((c >> 24) & 255) * s_tr, g = ((c >> 16) & 255) * s_tg, b = ((c >> 8) & 255) * s_tb;
    return ((uint32_t)(r > 255 ? 255 : r) << 24) | ((uint32_t)(g > 255 ? 255 : g) << 16) |
           ((uint32_t)(b > 255 ? 255 : b) << 8) | (c & 255u);
}
uint32_t world_scene_tint(void) { return s_prop_tint; }
uint32_t world_monster_tint(void) { return s_mob_tint; }

AreaData g_area_data[AREA_COUNT] = {
    // AREA 1: The Forsaken Path
    {
        .id = AREA_FORSAKEN_PATH,
        .name = "THE FORSAKEN PATH",
        .subtitle = "O Caminho dos Condenados",
        .spawn_x = 0.0f,
        .spawn_z = -160.0f,
        .spawn_yaw = 1.5707963f, // Facing forward (+Z)
        .exit_x = 0.0f,
        .exit_z = 175.0f,
        .exit_radius = 24.0f,
        .exit_unlocked = false,
        .min_x = -130.0f, .max_x = 130.0f,
        .min_z = -190.0f, .max_z = 190.0f,
        .clear_color = RGBA32(12, 16, 18, 255),
        .fog_color = RGBA32(15, 20, 22, 255),
        .fog_near = 120.0f, .fog_far = 380.0f,
        .ambient_light = { 95, 100, 105, 255 },
        .dir_light_color = { 150, 165, 180, 255 },
        .dir_light_vec = {{ 0.4f, 0.8f, 0.4f }}
    },
    // AREA 2: The Hollow Crypt
    {
        .id = AREA_HOLLOW_CRYPT,
        .name = "THE HOLLOW CRYPT",
        .subtitle = "A Cripta Vazia",
        .spawn_x = 0.0f,
        .spawn_z = -160.0f,
        .spawn_yaw = 1.5707963f,
        .exit_x = 0.0f,
        .exit_z = 175.0f,
        .exit_radius = 24.0f,
        .exit_unlocked = false,
        .min_x = -120.0f, .max_x = 120.0f,
        .min_z = -180.0f, .max_z = 180.0f,
        .clear_color = RGBA32(14, 10, 16, 255),
        .fog_color = RGBA32(18, 12, 20, 255),
        .fog_near = 100.0f, .fog_far = 340.0f,
        .ambient_light = { 85, 76, 90, 255 },
        .dir_light_color = { 190, 140, 90, 255 }, // Torchlight glow
        .dir_light_vec = {{ 0.2f, 0.9f, 0.3f }}
    },
    // AREA 3: The Black Altar
    {
        .id = AREA_BLACK_ALTAR,
        .name = "THE BLACK ALTAR",
        .subtitle = "O Altar Negro de Malakor",
        .spawn_x = 0.0f,
        .spawn_z = -150.0f,
        .spawn_yaw = 1.5707963f,
        .exit_x = 0.0f,
        .exit_z = 9999.0f, // No exit until Malakor defeated
        .exit_radius = 0.0f,
        .exit_unlocked = false,
        .min_x = -140.0f, .max_x = 140.0f,
        .min_z = -170.0f, .max_z = 170.0f,
        .clear_color = RGBA32(20, 8, 10, 255),
        .fog_color = RGBA32(24, 10, 12, 255),
        .fog_near = 130.0f, .fog_far = 400.0f,
        .ambient_light = { 100, 62, 68, 255 },
        .dir_light_color = { 225, 80, 60, 255 }, // Demonic crimson glow
        .dir_light_vec = {{ 0.3f, 0.85f, 0.35f }}
    }
};

static WorldCollider s_colliders[MAX_COLLIDERS];
static int s_collider_count = 0;
// Cada colisor fica numa lista do tile onde nasceu, entao a colisao so testa os 3x3 tiles
// vizinhos (com dezenas de inimigos, varrer todos os colisores a cada passo ficaria caro).
#define MAX_GRID_TILES 1600
static int16_t s_col_head[MAX_GRID_TILES];
static int16_t s_col_next[MAX_COLLIDERS];

static void add_circle_collider(float x, float z, float r);
static void add_box_collider(float x, float z, float hx, float hz);
static SceneProp s_scene_props[MAX_SCENE_PROPS];
static int s_scene_prop_count = 0;
static sprite_t *s_floor_textures[STAGE_COUNT];   // por fase de cada campanha (campanha * 3 + fase)

const SceneProp *world_scene_props(int *count) {
    *count = s_scene_prop_count;
    return s_scene_props;
}

// Decalques (manchas, pocas, lava, circulos) sao chatos no chao: se encostarem num muro o desenho
// "sobe" na parede. So entram se o retangulo inteiro estiver sobre chao.
static bool prop_is_decal(ScenePropId id) {
    return id == SCENE_PENTAGRAM || (id >= SCENE_DECAL_BLOOD && id <= SCENE_DECAL_GRAVE) ||
           (id >= SCENE_CAVE_DECAL_BLOOD && id <= SCENE_CAVE_DECAL_WEB) ||
           (id >= SCENE_HELL_DECAL_LAVA && id <= SCENE_HELL_LAVA_POOL) ||
           (id >= SCENE_SWAMP_DECAL_POOL && id <= SCENE_SWAMP_DECAL_BONES) ||
           (id >= SCENE_HEART_DECAL_POISON && id <= SCENE_HEART_POISON_POOL);
}
static bool decal_fits(float x, float z, float w);

// Pele da campanha: a Floresta Morta troca a arte do Altar pela do Coracao Seco e o portal de pedra
// pelo arco de raizes. Todo cenario passa por aqui (add_scene_prop e trocas diretas de id).
static ScenePropId scene_skin(ScenePropId id) {
    if (g_campaign == 2) {                         // Norte: portal de pedras runicas e o salao do Jarl
        if (id == SCENE_CAVE_PORTAL || id == SCENE_HELL_PORTAL) return SCENE_ICE_PORTAL;
        if (g_current_area != AREA_BLACK_ALTAR) return id;
        switch (id) {
            case SCENE_HELL_SEAL_ON:      return SCENE_FROST_SEAL_ON;
            case SCENE_HELL_SEAL_OFF:     return SCENE_FROST_SEAL_OFF;
            case SCENE_HELL_ROCKS_A:      return SCENE_FROST_ROCKS_A;
            case SCENE_HELL_ROCKS_B:      return SCENE_FROST_ROCKS_B;
            case SCENE_HELL_ROCKS_C:      return SCENE_FROST_ROCKS_C;
            case SCENE_HELL_ROCKS_D:      return SCENE_FROST_ROCKS_A;
            case SCENE_HELL_SPIKES_A:     return SCENE_FROST_SPIKES_A;
            case SCENE_HELL_SPIKES_B:     return SCENE_FROST_SPIKES_B;
            case SCENE_HELL_SPIKES_C:     return SCENE_FROST_SPIKES_C;
            case SCENE_HELL_FIREVENT:     return SCENE_FROST_VENT;
            case SCENE_HELL_BRAZIER:      return SCENE_FROST_BRAZIER;
            case SCENE_HELL_HANGED:       return SCENE_FROST_HANGED;
            case SCENE_HELL_SKULLS:       return SCENE_FROST_SKULLS;
            case SCENE_HELL_STATUE:       return SCENE_FROST_STATUE;
            case SCENE_HELL_CAGE:         return SCENE_FROST_CAGE;
            case SCENE_HELL_THRONE:       return SCENE_FROST_THRONE;
            case SCENE_HELL_DECAL_LAVA:   return SCENE_FROST_DECAL_ICE;
            case SCENE_HELL_DECAL_CRACK:  return SCENE_FROST_DECAL_CRACK;
            case SCENE_HELL_DECAL_RITUAL: return SCENE_FROST_DECAL_CIRCLE;
            case SCENE_HELL_DECAL_BONES:  return SCENE_FROST_DECAL_BONES;
            case SCENE_HELL_DECAL_ASH:    return SCENE_FROST_DECAL_SNOW;
            case SCENE_HELL_DECAL_BLOOD:  return SCENE_FROST_DECAL_BLOOD;
            case SCENE_HELL_LAVA_POOL:    return SCENE_FROST_POOL;
            default: return id;
        }
    }
    if (g_campaign != 1) return id;
    if (id == SCENE_CAVE_PORTAL || id == SCENE_HELL_PORTAL) return SCENE_FOREST_PORTAL;
    if (g_current_area != AREA_BLACK_ALTAR) return id;
    switch (id) {
        case SCENE_HELL_SEAL_ON:      return SCENE_HEART_TOTEM_ON;
        case SCENE_HELL_SEAL_OFF:     return SCENE_HEART_TOTEM_OFF;
        case SCENE_HELL_ROCKS_A:      return SCENE_HEART_ROOTS_A;
        case SCENE_HELL_ROCKS_B:      return SCENE_HEART_ROOTS_B;
        case SCENE_HELL_ROCKS_C:      return SCENE_HEART_ROOTS_C;
        case SCENE_HELL_ROCKS_D:      return SCENE_HEART_CARCASS;
        case SCENE_HELL_SPIKES_A:     return SCENE_HEART_THORNS_A;
        case SCENE_HELL_SPIKES_B:     return SCENE_HEART_THORNS_B;
        case SCENE_HELL_SPIKES_C:     return SCENE_HEART_THORNS_C;
        case SCENE_HELL_FIREVENT:     return SCENE_HEART_SPORE;
        case SCENE_HELL_BRAZIER:      return SCENE_HEART_FUNGUS;
        case SCENE_HELL_HANGED:       return SCENE_HEART_COCOON;
        case SCENE_HELL_SKULLS:       return SCENE_HEART_SKULLS;
        case SCENE_HELL_STATUE:       return SCENE_HEART_STONE;
        case SCENE_HELL_THRONE:       return SCENE_HEART_THRONE;
        case SCENE_HELL_DECAL_LAVA:   return SCENE_HEART_DECAL_POISON;
        case SCENE_HELL_DECAL_CRACK:  return SCENE_HEART_DECAL_CRACK;
        case SCENE_HELL_DECAL_RITUAL: return SCENE_HEART_DECAL_CIRCLE;
        case SCENE_HELL_DECAL_BONES:  return SCENE_HEART_DECAL_BONES;
        case SCENE_HELL_DECAL_ASH:    return SCENE_HEART_DECAL_LEAVES;
        case SCENE_HELL_DECAL_BLOOD:  return SCENE_HEART_DECAL_SAP;
        case SCENE_HELL_LAVA_POOL:    return SCENE_HEART_POISON_POOL;
        default: return id;
    }
}

static void add_scene_prop(ScenePropId id, float x, float z, float height, float col_r) {
    if (s_scene_prop_count >= MAX_SCENE_PROPS) return;
    ScenePropId sk = scene_skin(id);
    if (sk >= SCENE_HEART_ROOTS_A && sk <= SCENE_HEART_ROOTS_C && id != sk) height *= 2.4f;   // raizes gigantes no lugar das pedras
    if (sk == SCENE_HEART_CARCASS && id != sk) height *= 1.8f;
    id = sk;
    if (prop_is_decal(id) && !decal_fits(x, z, height)) return;
    s_scene_props[s_scene_prop_count++] = (SceneProp){id, x, 0.0f, z, height};
    if (col_r > 0.0f) add_circle_collider(x, z, col_r);
}

// Floor mesh
#define FLOOR_TILES_X 8
#define FLOOR_TILES_Z 10
static T3DVertPacked *s_floor_verts = NULL;
static int s_floor_vert_count = 0;

// ===== Mapas em grade (tiles) =====
#define TILE 72.0f
#define WALL_H 72.0f
// Mapa 1: altura do muro. A oclusao dos sprites e feita por recorte (sprites.c): o que fica
// atras do muro some e o druida vira silhueta, como no Diablo. Quanto mais alto, mais area
// atras do muro fica escondida (~0.8 x altura).
#define WALL_H_LOW 44.0f
#define WALL_H_CRYPT 64.0f
static float s_wall_h_cur = 72.0f;
#define PENTA_Y 1.0f
#define MAX_WALL_FACES 512
#define PENTA_SEGS 28                 // segmentos do circulo do pentagrama
#define PENTA_QUADS (PENTA_SEGS + 5)  // circulo + 5 linhas da estrela

// Paredes 3D como blocos solidos: faces laterais (textura de parede) + tampa
// superior (textura de topo), para terem profundidade real (nao papel).
static T3DVertPacked *s_wall_verts = NULL;      // faces laterais
static int s_wall_vert_count = 0;
static T3DVertPacked *s_wall_top_verts = NULL;  // tampas (topo) dos blocos
static int s_wall_top_vert_count = 0;
static sprite_t *s_wall_textures[STAGE_COUNT];
static sprite_t *s_wall_top_textures[STAGE_COUNT];
static sprite_t *s_wall_tex = NULL;
static sprite_t *s_wall_top_tex = NULL;
// Pentagrama procedural (geometria vermelha), nitido em qualquer tamanho.
static T3DVertPacked *s_penta_verts = NULL;
static int s_penta_vert_count = 0;
// Mapa do Caminho dos Condenados: include/map_forsaken.h (gerado por tools/gen_forsaken_map.py)
#include "map_altar.h"   // gerado por tools/gen_altar_map.py

static bool s_crypt_theme = false; // mapa 2: monstros/cenario da cripta
static bool s_altar_theme = false; // mapa 3: Altar Negro (selos + Malakor)
static bool s_swamp_theme = false; // Floresta Morta fase 2: Pantano Podre
static bool s_snow_theme = false;  // Norte fase 1: Passo Congelado
static bool s_ice_theme = false;   // Norte fase 2: Cavernas de Gelo
// Agua do pantano: deixa a heroina lenta (os monstros nao)
#define MAX_SLOW 32
typedef struct { float x, z, r; } SlowZone;
static SlowZone s_slow[MAX_SLOW];
static int s_slow_count = 0;
static void add_slow(float x, float z, float r) { if (s_slow_count < MAX_SLOW) s_slow[s_slow_count++] = (SlowZone){x, z, r}; }
// Gelo liso do Passo Congelado: a heroina escorrega (acelera e freia devagar)
static SlowZone s_slip[MAX_SLOW];
static int s_slip_count = 0;
static void add_slip(float x, float z, float r) { if (s_slip_count < MAX_SLOW) s_slip[s_slip_count++] = (SlowZone){x, z, r}; }
float world_ground_grip(float x, float z) {
    for (int i = 0; i < s_slip_count; i++)
        if (vec2_dist_sq(x, z, s_slip[i].x, s_slip[i].z) < s_slip[i].r * s_slip[i].r) return 0.18f;
    return 1.0f;
}
// Nevasca: de tempos em tempos a visao fecha (so no Passo Congelado)
float world_vision_scale(void) {
    if (g_campaign != 2 || g_current_area != AREA_FORSAKEN_PATH) return 1.0f;
    float t = fmodf((float)get_ticks_ms() * 0.001f, 45.0f);          // ciclo de 45 s
    if (t < 33.0f) return 1.0f;
    float k = t < 36.0f ? (t - 33.0f) / 3.0f : t > 42.0f ? (45.0f - t) / 3.0f : 1.0f;
    return 1.0f - 0.32f * k;                                          // nevasca: 32% menos visao
}
float world_ground_speed(float x, float z) {
    for (int i = 0; i < s_slow_count; i++)
        if (vec2_dist_sq(x, z, s_slow[i].x, s_slow[i].z) < s_slow[i].r * s_slow[i].r) return 0.6f;
    return 1.0f;
}
// Mapa 3: os 3 SELOS DE SANGUE. Ficar dentro do circulo purifica o selo (hordas atacam enquanto isso);
// com os 3 purificados, Malakor surge no centro do pentagrama.
#define SEAL_COUNT_MAX 3
#define SEAL_RADIUS    46.0f    // circulo onde o druida precisa ficar
#define SEAL_TIME      14.0f    // segundos dentro do circulo para purificar
#define SEAL_WAVE_T     3.2f    // a cada quantos segundos chega uma leva de monstros
typedef struct { float x, z, progress, wave_t; bool done, elite_sent; int decal, obelisk; } Seal;
// Perigos do inferno: lagos de lava (sempre queimam) e fendas de fogo (queimam quando a chama sobe).
typedef struct { float x, z, r; bool vent; } Hazard;
#define MAX_HAZARDS 24
static Hazard s_hazards[MAX_HAZARDS];
static int s_hazard_count = 0;
static Seal s_seals[SEAL_COUNT_MAX];
static int s_seal_count = 0, s_seals_done = 0;
static float s_boss_x = 0.0f, s_boss_z = 0.0f;
static bool s_boss_summoned = false;
static float s_vendor_x = 0.0f, s_vendor_z = 0.0f;   // mascate (posicao sorteada por partida)
static bool s_has_vendor = false;
static const char *const *s_grid=NULL;
static int s_grid_cols=0, s_grid_rowsN=0;
static uint8_t s_explored[MAX_GRID_TILES];   // minimapa: tiles ja vistos
static bool s_minimap_on = true;
static float s_half_w=0.0f, s_half_h=0.0f;
static bool tile_walk(char c){ return c!='#' && c!=' '; }

static void link_collider(int i, float x, float z) {
    if (!s_grid || s_grid_cols * s_grid_rowsN > MAX_GRID_TILES) return;
    int c = (int)floorf((x + s_half_w) / TILE);
    int r = (int)floorf((s_half_h - z) / TILE);
    if (c < 0) c = 0;
    if (r < 0) r = 0;
    if (c >= s_grid_cols) c = s_grid_cols - 1;
    if (r >= s_grid_rowsN) r = s_grid_rowsN - 1;
    int t = r * s_grid_cols + c;
    s_col_next[i] = s_col_head[t];
    s_col_head[t] = (int16_t)i;
}

static void add_circle_collider(float x, float z, float r) {
    if (s_collider_count >= MAX_COLLIDERS) return;
    s_colliders[s_collider_count] = (WorldCollider){
        .type = COL_CIRCLE,
        .x = x, .z = z, .r = r
    };
    link_collider(s_collider_count, x, z);
    s_collider_count++;
}

static void add_box_collider(float x, float z, float hx, float hz) {
    if (s_collider_count >= MAX_COLLIDERS) return;
    s_colliders[s_collider_count] = (WorldCollider){
        .type = COL_BOX,
        .x = x, .z = z, .hx = hx, .hz = hz
    };
    link_collider(s_collider_count, x, z);
    s_collider_count++;
}

// Generate segmented floor with gothic vertex colors
static void build_floor(uint32_t col, int cols, int rows) {
    if (!s_floor_verts)
        s_floor_verts = (T3DVertPacked*)malloc_uncached(3 * sizeof(T3DVertPacked)); // 6 verts
    s_floor_vert_count = 6;
    AreaData *ad = &g_area_data[g_current_area];
    uint16_t nup = t3d_vert_pack_normal(&(T3DVec3){{0,1,0}});
    float x0=ad->min_x, x1=ad->max_x, z0=ad->min_z, z1=ad->max_z;
    int16_t su=(int16_t)((cols*32)<<5), sv=(int16_t)((rows*32)<<5); // textura repete por tile
    float vx[6]={x0,x1,x1,x0,x1,x0}, vz[6]={z0,z0,z1,z0,z1,z1};
    int16_t us[6]={0,su,su,0,su,0},   vs[6]={0,0,sv,0,sv,sv};
    for (int k=0;k<6;k++){
        int pr=k/2, sub=k%2;
        if(sub==0){ s_floor_verts[pr].posA[0]=(int16_t)vx[k]; s_floor_verts[pr].posA[1]=0; s_floor_verts[pr].posA[2]=(int16_t)vz[k];
            s_floor_verts[pr].rgbaA=col; s_floor_verts[pr].normA=nup; s_floor_verts[pr].stA[0]=us[k]; s_floor_verts[pr].stA[1]=vs[k]; }
        else { s_floor_verts[pr].posB[0]=(int16_t)vx[k]; s_floor_verts[pr].posB[1]=0; s_floor_verts[pr].posB[2]=(int16_t)vz[k];
            s_floor_verts[pr].rgbaB=col; s_floor_verts[pr].normB=nup; s_floor_verts[pr].stB[0]=us[k]; s_floor_verts[pr].stB[1]=vs[k]; }
    }
}

// Escreve um vertice (posicao/cor/normal/UV) na posicao global k do array packed.
static void qset(T3DVertPacked *base, int k, float x, float y, float z,
                 uint32_t col, uint16_t nrm, int16_t s, int16_t t) {
    int pr = k/2, sub = k%2;
    if (sub==0) {
        base[pr].posA[0]=(int16_t)x; base[pr].posA[1]=(int16_t)y; base[pr].posA[2]=(int16_t)z;
        base[pr].rgbaA=col; base[pr].normA=nrm; base[pr].stA[0]=s; base[pr].stA[1]=t;
    } else {
        base[pr].posB[0]=(int16_t)x; base[pr].posB[1]=(int16_t)y; base[pr].posB[2]=(int16_t)z;
        base[pr].rgbaB=col; base[pr].normB=nrm; base[pr].stB[0]=s; base[pr].stB[1]=t;
    }
}

// Empilha um quad (2 triangulos, 6 verts) a partir de 4 cantos + UVs.
static void push_quad(T3DVertPacked *base, int *vc, float P[4][3], int16_t ST[4][2],
                      uint32_t col, uint16_t nrm) {
    static const int idx[6] = {0,1,2,0,2,3};
    int o = *vc;
    for (int k=0;k<6;k++) {
        int ci = idx[k];
        qset(base, o+k, P[ci][0],P[ci][1],P[ci][2], col, nrm, ST[ci][0],ST[ci][1]);
    }
    *vc = o + 6;
}

// Constroi as paredes como BLOCOS SOLIDOS, para terem profundidade (nao papel):
//  - faces laterais (s_wall_verts): um quad vertical por face de tile '#'
//    exposta a um tile caminhavel, com a textura de parede (64x32, cabe TMEM).
//  - tampas de topo (s_wall_top_verts): um quad horizontal no topo de cada tile
//    de parede exposto, com a textura de topo. E o topo visto de cima que da a
//    leitura de bloco macico na camera 3/4.
static void build_walls(const char *const *map, int rows, int cols, float wall_h) {
    if (!s_wall_verts)
        s_wall_verts = (T3DVertPacked*)malloc_uncached(MAX_WALL_FACES*3*sizeof(T3DVertPacked));
    if (!s_wall_top_verts)
        s_wall_top_verts = (T3DVertPacked*)malloc_uncached(MAX_WALL_FACES*3*sizeof(T3DVertPacked));
    int vc = 0, tvc = 0;
    const uint32_t col = tint32(0xFFFFFFFFu);
    const int16_t sW = (int16_t)(64<<5);
    const int16_t sH = (int16_t)((wall_h < 60.0f ? 32 : 64)<<5); // 1x largura; muro baixo: 1x altura, alto: 2x
    const int16_t tW = (int16_t)(64<<5), tH = (int16_t)(32<<5); // topo: 1x
    const float H = wall_h;
    s_wall_h_cur = wall_h;
    for (int r=0;r<rows;r++) for (int c=0;c<cols;c++) {
        if (map[r][c] != '#') continue;
        float tx0 = c*TILE - s_half_w, tx1 = tx0 + TILE;
        float tz1 = s_half_h - r*TILE,  tz0 = tz1 - TILE; // tz1=borda norte, tz0=sul
        bool exposed = (r>0 && tile_walk(map[r-1][c])) || (r<rows-1 && tile_walk(map[r+1][c])) ||
                       (c>0 && tile_walk(map[r][c-1])) || (c<cols-1 && tile_walk(map[r][c+1]));
        // Face norte (vizinho r-1 caminhavel): plano z=tz1
        if (r>0 && tile_walk(map[r-1][c]) && vc/6 < MAX_WALL_FACES) {
            uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0,0,1}});
            float P[4][3]={{tx0,0,tz1},{tx1,0,tz1},{tx1,H,tz1},{tx0,H,tz1}};
            int16_t ST[4][2]={{0,sH},{sW,sH},{sW,0},{0,0}};
            push_quad(s_wall_verts,&vc,P,ST,col,n);
        }
        // Face sul (vizinho r+1): plano z=tz0
        if (r<rows-1 && tile_walk(map[r+1][c]) && vc/6 < MAX_WALL_FACES) {
            uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0,0,-1}});
            float P[4][3]={{tx0,0,tz0},{tx1,0,tz0},{tx1,H,tz0},{tx0,H,tz0}};
            int16_t ST[4][2]={{0,sH},{sW,sH},{sW,0},{0,0}};
            push_quad(s_wall_verts,&vc,P,ST,col,n);
        }
        // Face oeste (vizinho c-1): plano x=tx0
        if (c>0 && tile_walk(map[r][c-1]) && vc/6 < MAX_WALL_FACES) {
            uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{-1,0,0}});
            float P[4][3]={{tx0,0,tz0},{tx0,0,tz1},{tx0,H,tz1},{tx0,H,tz0}};
            int16_t ST[4][2]={{0,sH},{sW,sH},{sW,0},{0,0}};
            push_quad(s_wall_verts,&vc,P,ST,col,n);
        }
        // Face leste (vizinho c+1): plano x=tx1
        if (c<cols-1 && tile_walk(map[r][c+1]) && vc/6 < MAX_WALL_FACES) {
            uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{1,0,0}});
            float P[4][3]={{tx1,0,tz0},{tx1,0,tz1},{tx1,H,tz1},{tx1,H,tz0}};
            int16_t ST[4][2]={{0,sH},{sW,sH},{sW,0},{0,0}};
            push_quad(s_wall_verts,&vc,P,ST,col,n);
        }
        // Tampa de topo (y=H), para tiles expostos
        if (exposed && tvc/6 < MAX_WALL_FACES) {
            uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0,1,0}});
            float P[4][3]={{tx0,H,tz0},{tx1,H,tz0},{tx1,H,tz1},{tx0,H,tz1}};
            int16_t ST[4][2]={{0,tH},{tW,tH},{tW,0},{0,0}};
            push_quad(s_wall_top_verts,&tvc,P,ST,col,n);
        }
    }
    s_wall_vert_count = vc;
    s_wall_top_vert_count = tvc;
}

bool world_wall_tile(int col, int row) {
    if (!s_grid || col<0 || row<0 || col>=s_grid_cols || row>=s_grid_rowsN) return false;
    return s_grid[row][col]=='#';
}
static bool decal_fits(float x, float z, float w) {
    if (!s_grid) return true;
    float hx = w * 0.5f, hz = w * 0.27f;          // decal 2:1 deitado
    for (int k = 0; k < 9; k++) {
        float px = x + hx * (float)(k % 3 - 1), pz = z + hz * (float)(k / 3 - 1);
        int c = (int)floorf((px + s_half_w) / TILE), r = (int)floorf((s_half_h - pz) / TILE);
        if (c < 0 || r < 0 || c >= s_grid_cols || r >= s_grid_rowsN || s_grid[r][c] == '#') return false;
    }
    return true;
}

bool world_wall_info(float *half_w, float *half_h, float *tile, float *height, int *cols, int *rows) {
    if (!s_grid) return false;
    *half_w=s_half_w; *half_h=s_half_h; *tile=TILE; *height=s_wall_h_cur;
    *cols=s_grid_cols; *rows=s_grid_rowsN;
    return true;
}

// ---- Minimapa (janela pequena centrada no druida; canto superior direito; Z liga/desliga) ----
// O eixo X do mundo e espelhado na tela (x+ = esquerda), entao a janela tambem e espelhada.
#define MM_HALF 9      // janela de (2*MM_HALF+1)^2 tiles
#define MM_PX   2      // pixels por tile
static uint8_t mm_class(int c, int r) {
    if (c < 0 || r < 0 || c >= s_grid_cols || r >= s_grid_rowsN) return 0;
    if (!s_explored[r * s_grid_cols + c]) return 0;
    return (s_grid[r][c] == '#') ? 2 : 1;
}
void world_minimap_toggle(void) { s_minimap_on = !s_minimap_on; }
void world_draw_minimap(void) {
    if (!s_minimap_on || !s_grid || s_grid_cols * s_grid_rowsN > MAX_GRID_TILES) return;
    const int n = 2 * MM_HALF + 1, size = n * MM_PX;
    const int x0 = SCREEN_WIDTH - 12 - size, y0 = 10;
    int pcol = (int)((g_druid.x + s_half_w) / TILE), prow = (int)((s_half_h - g_druid.z) / TILE);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 110));
    rdpq_fill_rectangle(x0 - 1, y0 - 1, x0 + size + 1, y0 + size + 1);
    for (int wr = -MM_HALF; wr <= MM_HALF; wr++) {
        int r = prow + wr, i = -MM_HALF;
        while (i <= MM_HALF) {
            uint8_t k = mm_class(pcol - i, r);   // espelhado: direita da tela = coluna menor
            int e = i + 1;
            while (e <= MM_HALF && mm_class(pcol - e, r) == k) e++;
            if (k) {
                rdpq_set_prim_color(k == 2 ? RGBA32(150, 140, 125, 230) : RGBA32(60, 56, 68, 205));
                rdpq_fill_rectangle(x0 + (i + MM_HALF) * MM_PX, y0 + (wr + MM_HALF) * MM_PX,
                                    x0 + (e + MM_HALF) * MM_PX, y0 + (wr + MM_HALF + 1) * MM_PX);
            }
            i = e;
        }
    }
    AreaData *ad = &g_area_data[g_current_area];
    uint32_t t = get_ticks_ms();
    if (ad->exit_unlocked && ((t / 250) & 1)) {   // portal aberto: ponto ciano (preso a borda se estiver longe)
        int ci = pcol - (int)((ad->exit_x + s_half_w) / TILE);
        int ri = (int)((s_half_h - ad->exit_z) / TILE) - prow;
        if (ci < -MM_HALF) ci = -MM_HALF;
        if (ci >  MM_HALF) ci =  MM_HALF;
        if (ri < -MM_HALF) ri = -MM_HALF;
        if (ri >  MM_HALF) ri =  MM_HALF;
        int ex = x0 + (ci + MM_HALF) * MM_PX, ey = y0 + (ri + MM_HALF) * MM_PX;
        rdpq_set_prim_color(RGBA32(80, 230, 255, 255));
        rdpq_fill_rectangle(ex - 1, ey - 1, ex + MM_PX + 1, ey + MM_PX + 1);
    }
    int dx = x0 + MM_HALF * MM_PX, dy = y0 + MM_HALF * MM_PX;   // druida: sempre no centro
    rdpq_set_prim_color(((t / 300) & 1) ? RGBA32(255, 235, 120, 255) : RGBA32(255, 255, 255, 255));
    rdpq_fill_rectangle(dx - 1, dy - 1, dx + MM_PX + 1, dy + MM_PX + 1);
    rdpq_mode_blender(0);
}

// Pentagrama PROCEDURAL: geometria vermelha (circulo + estrela de 5 pontas em
// linhas), deitada no plano y=PENTA_Y. Nitido em qualquer tamanho, sem textura
// e sem limite de TMEM. Fica sob o personagem (desenhado por cima, em 2D).
static void penta_strip(float ax,float az,float bx,float bz,float w,
                        uint32_t col,uint16_t n,int *vc) {
    float dx=bx-ax, dz=bz-az, len=sqrtf(dx*dx+dz*dz);
    if (len<0.001f) return;
    float px=-dz/len*w, pz=dx/len*w; // perpendicular * meia-largura
    float P[4][3]={{ax+px,PENTA_Y,az+pz},{ax-px,PENTA_Y,az-pz},
                   {bx-px,PENTA_Y,bz-pz},{bx+px,PENTA_Y,bz+pz}};
    int16_t ST[4][2]={{0,0},{0,0},{0,0},{0,0}};
    push_quad(s_penta_verts,vc,P,ST,col,n);
}
static void build_penta(float cx, float cz, float half) {
    if (!s_penta_verts)
        s_penta_verts = (T3DVertPacked*)malloc_uncached(PENTA_QUADS*3*sizeof(T3DVertPacked));
    const uint32_t col = g_campaign == 1 ? 0x8C7A30FFu : g_campaign == 2 ? 0x4A90D0FFu : 0xE02828FFu; // vermelho sangue (floresta: ambar de raiz)
    uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0,1,0}});
    float R = half*0.94f;              // raio do circulo
    float lw = half*0.045f;            // meia-largura das linhas
    int vc = 0;
    // Circulo (anel) em PENTA_SEGS segmentos
    for (int i=0;i<PENTA_SEGS;i++) {
        float a0=(float)i/PENTA_SEGS*6.2831853f, a1=(float)(i+1)/PENTA_SEGS*6.2831853f;
        penta_strip(cx+R*cosf(a0),cz+R*sinf(a0),cx+R*cosf(a1),cz+R*sinf(a1),lw,col,n,&vc);
    }
    // Estrela de 5 pontas: liga vertices de 2 em 2 (ponta para o norte)
    float vx[5], vz[5];
    for (int k=0;k<5;k++) {
        float a = -1.5707963f + (float)k*1.2566371f; // -90 + k*72 graus
        vx[k]=cx+R*cosf(a); vz[k]=cz+R*sinf(a);
    }
    int order[6]={0,2,4,1,3,0};
    for (int k=0;k<5;k++)
        penta_strip(vx[order[k]],vz[order[k]],vx[order[k+1]],vz[order[k+1]],lw,col,n,&vc);
    s_penta_vert_count = vc;
}

// ============================================================================
// Caminho dos Condenados (area 1): populacao aleatoria (roguelike) e chao em camadas
// ============================================================================
static float frand(void) { return (float)(rand() & 0x7FFF) * (1.0f / 32768.0f); }
static float frange(float a, float b) { return a + (b - a) * frand(); }

typedef struct { char ch; float x, z; } Marker;
typedef struct { float x, z; bool done; } Ambush;
// Ponto de horda: os monstros so nascem quando o druida chega perto (e voltam a "dormir" se ele se afastar
// muito sem lutar). Hordas grandes, de 10 a 15.
typedef struct { char ch; float x, z; int left; int pack; bool elite_pending; } Spawner;
#define MAX_SPAWNERS 32
#define SPAWN_NEAR     230.0f   // distancia do druida que acorda a horda (logo fora da tela)
#define SPAWN_MIN_GAP  110.0f   // nenhum monstro nasce colado no druida
#define DESPAWN_FAR    620.0f   // monstro parado (nao perseguindo) longe assim volta para o ponto
static Spawner s_spawners[MAX_SPAWNERS];
static int s_spawner_count = 0;
#define MAX_MARKERS 64
#define MAX_AMBUSH 8
static Marker s_markers[MAX_MARKERS];
static int s_marker_count = 0;
static Ambush s_ambush[MAX_AMBUSH];
static int s_ambush_count = 0;
static int s_elites_total = 0;
static int s_elites_shown = 0;
static int s_pack_seq = 1;

// Posicao livre de muros, cercas, arvores e lapides?
static bool world_spot_free(float x, float z, float r) {
    float px = x, pz = z;
    return !world_collide_circle(&px, &pz, r);
}

static EnemyType pick_type(char kind) {
    int roll = rand() % 100;
    if (s_altar_theme) {   // altar: demonios e cultistas, poucos esqueletos
        (void)kind;
        return roll < 50 ? ENEMY_TYPE_DEMON : (roll < 82 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_SKELETON);
    }
    if (s_crypt_theme) {   // cripta: mais cultistas e demonios, menos esqueletos
        switch (kind) {
            case 's': return roll < 40 ? ENEMY_TYPE_SKELETON : ENEMY_TYPE_CULTIST;
            case 'S': return roll < 30 ? ENEMY_TYPE_SKELETON : (roll < 70 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON);
            case 'Z': return roll < 20 ? ENEMY_TYPE_SKELETON : (roll < 55 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON);
            case 'A': return roll < 35 ? ENEMY_TYPE_SKELETON : (roll < 70 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON);
            default:  return roll < 40 ? ENEMY_TYPE_SKELETON : ENEMY_TYPE_CULTIST;
        }
    }
    switch (kind) {
        case 's': return roll < 70 ? ENEMY_TYPE_SKELETON : ENEMY_TYPE_CULTIST;
        case 'S': return roll < 55 ? ENEMY_TYPE_SKELETON : (roll < 85 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON);
        case 'Z': return roll < 40 ? ENEMY_TYPE_SKELETON : (roll < 68 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON);
        case 'A': return roll < 60 ? ENEMY_TYPE_SKELETON : (roll < 85 ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON);
        default:  return roll < 65 ? ENEMY_TYPE_SKELETON : ENEMY_TYPE_CULTIST; // escolta do guardiao
    }
}

static int pack_size(char kind) {
    switch (kind) {
        case 's': return 10 + rand() % 2;  // hordas: 10..11
        case 'S': return 12 + rand() % 2;  // 12..13
        case 'Z': return 14 + rand() % 2;  // 14..15
        case 'q': return 8;                // escolta do guardiao
        default:  return 10;
    }
}

static int spawn_pack_member(EnemyType t, float cx, float cz, int pack, bool elite, float rmin, float rmax) {
    for (int tries = 0; tries < 16; tries++) {
        float ang = frand() * 6.2831853f;
        float rad = frange(rmin, rmax);
        float x = cx + cosf(ang) * rad;
        float z = cz + sinf(ang) * rad;
        if (!world_spot_free(x, z, 12.0f)) continue;
        return enemies_spawn_ex(t, x, z, pack, elite);
    }
    return -1;
}

static bool on_trail(float x, float z, float margin);
static void events_setup(void);
static void chest_ev_add(int chest, float x, float z);
static int s_cev_count = 0;
static bool clear_of_trail(float *x, float *z, float r);

// Coloca o cenario de um tile do Caminho dos Condenados. Retorna true se tratou o caractere.
static bool forsaken_place(AreaData *ad, char ch, float wx, float wz) {
    // Nada de objeto em pe sobre a trilha de pedra: se cair nela, procura um ponto livre ao lado
    // (ou descarta). Cercas, portoes, estatuas dos portoes, decalques e o portal ficam como estao.
    {
        float clr = 0.0f;
        switch (ch) {
            case 'T': case 'X': case '~': case 'Y': clr = 14.0f; break;
            case 'K': clr = 12.0f; break;
            case 'F': clr = 16.0f; break;
            case 'R': clr = 16.0f; break;
            case 'U': clr = 18.0f; break;
            case 'p': clr = 22.0f; break;
            case 'G': clr = 30.0f; break;
            default: break;
        }
        if (clr > 0.0f && !clear_of_trail(&wx, &wz, clr)) return true;
    }
    switch (ch) {
        case '#': return true; // muro: malha 3D + colisao pela grade
        case 'E': ad->spawn_x = wx; ad->spawn_z = wz; ad->spawn_yaw = 1.5707963f; return true;
        // Saida: o portal so e criado quando os guardioes caem (world_unlock_exit).
        case 'P': ad->exit_x = wx; ad->exit_z = wz; ad->exit_radius = 40.0f; return true;
        case 'T': add_scene_prop(SCENE_GRAVESTONE, wx + frange(-5, 5), wz + frange(-4, 4), 20.0f, 5.0f); return true;
        case 'X': add_scene_prop(SCENE_GRAVE_CROSS, wx + frange(-5, 5), wz + frange(-4, 4), 22.0f, 5.0f); return true;
        case 'U': add_scene_prop(SCENE_TOMB_STATUE, wx, wz, 34.0f, 8.0f); return true;
        // 'u': estatua na frente de um trecho de cerca (fecha o tile inteiro, sem bloco invisivel)
        case 'u': add_scene_prop(SCENE_GRAVE_FENCE, wx, wz, 44.0f, 0.0f);
                  add_box_collider(wx, wz, TILE * 0.5f + 1.0f, 8.0f);
                  add_scene_prop(SCENE_TOMB_STATUE, wx, wz - 9.0f, 34.0f, 0.0f); return true;
        case 'G': add_scene_prop(SCENE_GREAT_TREE, wx, wz, 165.0f, 16.0f); return true;
        case 'R': add_scene_prop(SCENE_TREE_DEAD, wx + frange(-14, 14), wz + frange(-12, 12), frange(76, 92), 10.0f); return true;
        case 'K': add_scene_prop(SCENE_DEAD_BUSH, wx + frange(-16, 16), wz + frange(-14, 14), frange(22, 30), 0.0f); return true;
        case 'J': add_scene_prop(SCENE_GRAVE_FENCE, wx, wz, 44.0f, 0.0f);
                  add_box_collider(wx, wz, TILE * 0.5f + 1.0f, 8.0f); return true;
        case 'g': return true; // vao do portao: 1 tile livre entre muro/cerca; sem colisor nenhum
        case 'p': add_scene_prop(SCENE_PATH_SHRINE, wx, wz, 40.0f, 14.0f); return true;
        case 'F': add_scene_prop(SCENE_CAMPFIRE, wx, wz, 30.0f, 8.0f); return true;
        case '~': add_scene_prop(SCENE_TRAVELER_BODY, wx, wz, 14.0f, 0.0f); return true;   // corpo deitado: ~40% do druida
        case 'Y': add_scene_prop(SCENE_BONE_PILE, wx + frange(-10, 10), wz + frange(-8, 8), frange(22, 28), 0.0f); return true;
        case 'b': add_scene_prop(SCENE_DECAL_BONES, wx, wz, 44.0f, 0.0f); return true;
        case 'v': add_scene_prop(SCENE_DECAL_BLOOD, wx, wz, 54.0f, 0.0f); return true;
        case 'k': add_scene_prop(SCENE_DECAL_CRACKS, wx, wz, 58.0f, 0.0f); return true;
        case 'o': add_scene_prop(SCENE_DECAL_GRAVE, wx, wz, 60.0f, 0.0f); return true;
        case 's': case 'S': case 'Z': case 'q': case 'a': case 'C':
            if (s_marker_count < MAX_MARKERS) s_markers[s_marker_count++] = (Marker){ch, wx, wz};
            return true;
        default: return false;
    }
}

// Cenario da Cripta Vazia (mapa 2). Os caracteres que nao sao da cripta caem no cenario do mapa 1.
static ScenePropId pick3(ScenePropId first, int n) { return (ScenePropId)(first + rand() % n); }

static bool crypt_place(AreaData *ad, char ch, float wx, float wz) {
    switch (ch) {
        case 'L': add_scene_prop(pick3(SCENE_CAVE_STALAG_A, 3), wx, wz, 100.0f, 9.0f); return true;  // coluna de caverna
        case 'Q': add_scene_prop(pick3(SCENE_CAVE_SARCO_A, 2), wx, wz, 32.0f, 0.0f);
                  add_box_collider(wx, wz, 19.0f, 10.0f); return true;
        case 'A': add_scene_prop(SCENE_CAVE_ALTAR, wx, wz, 34.0f, 0.0f);
                  add_box_collider(wx, wz, 20.0f, 11.0f); return true;
        case 'F': add_scene_prop(SCENE_CAVE_BRAZIER, wx, wz, 34.0f, 8.0f); return true;
        // itens que antes "boiavam" no chao: pedras, cogumelos e cristais
        case 'V': case 'W': case 'B': add_scene_prop(pick3(SCENE_CAVE_ROCKS_A, 4), wx, wz, 22.0f, 0.0f); return true;
        case 'H': add_scene_prop(pick3(SCENE_CAVE_MUSH_A, 3), wx, wz, 20.0f, 0.0f); return true;
        case 'I': case 'M': add_scene_prop(pick3(SCENE_CAVE_CRYSTAL_A, 3), wx, wz, 34.0f, 0.0f); return true;
        case '~': add_scene_prop(SCENE_CAVE_CORPSE, wx, wz, 13.0f, 0.0f); return true;
        case 'U': add_scene_prop(pick3(SCENE_CAVE_STATUE_A, 2), wx, wz, 40.0f, 8.0f); return true;
        case 'Y': add_scene_prop(pick3(SCENE_CAVE_BONES_A, 2), wx + frange(-10, 10), wz + frange(-8, 8), frange(20, 26), 0.0f); return true;
        case 'b': add_scene_prop(SCENE_CAVE_DECAL_BONES, wx, wz, 44.0f, 0.0f); return true;
        case 'v': add_scene_prop(SCENE_CAVE_DECAL_BLOOD, wx, wz, 54.0f, 0.0f); return true;
        case 'k': add_scene_prop((rand() & 1) ? SCENE_CAVE_DECAL_WATER : SCENE_CAVE_DECAL_WEB, wx, wz, 54.0f, 0.0f); return true;
        case 'o': add_scene_prop(SCENE_CAVE_DECAL_GRAVE, wx, wz, 58.0f, 0.0f); return true;
        case 'E': ad->spawn_x = wx; ad->spawn_z = wz; ad->spawn_yaw = 1.5707963f;
                  // boca da caverna logo atras (ao sul) de onde o druida nasce
                  add_scene_prop(SCENE_CAVE_ENTRANCE, wx, wz - TILE * 0.95f, 92.0f, 0.0f);
                  add_box_collider(wx, wz - TILE * 0.95f, 34.0f, 14.0f);
                  return true;
        case 'P': ad->exit_x = wx; ad->exit_z = wz; ad->exit_radius = 40.0f;
                  // trono de ossos do guardiao atras do portal (ao norte)
                  add_scene_prop(SCENE_CAVE_THRONE, wx, wz + TILE * 1.25f, 64.0f, 0.0f);
                  add_box_collider(wx, wz + TILE * 1.25f, 20.0f, 12.0f);
                  return true;
        default: return forsaken_place(ad, ch, wx, wz);
    }
}

// Altar Negro (mapa 3): selos, ponto do chefe; hordas/emboscadas/baus/manchas como no mapa 1.
// O resto (pentagrama, trono, monolitos, braseiros, entrada) cai no switch padrao do load_grid.
static void add_hazard(float x, float z, float r, bool vent) {
    if (s_hazard_count < MAX_HAZARDS) s_hazards[s_hazard_count++] = (Hazard){x, z, r, vent};
}

static bool altar_place(AreaData *ad, char ch, float wx, float wz) {
    switch (ch) {
        case '*':
            if (s_seal_count < SEAL_COUNT_MAX) {
                Seal *sl = &s_seals[s_seal_count++];
                *sl = (Seal){wx, wz, 0.0f, 0.0f, false, false, -1, -1};
                int n0 = s_scene_prop_count;
                add_scene_prop(SCENE_HELL_DECAL_RITUAL, wx, wz, 104.0f, 0.0f);   // circulo de sangue no chao
                if (s_scene_prop_count > n0) sl->decal = n0;
                n0 = s_scene_prop_count;
                add_scene_prop(SCENE_HELL_SEAL_ON, wx, wz + 10.0f, 66.0f, 11.0f); // o selo (obelisco com runas)
                if (s_scene_prop_count > n0) sl->obelisk = n0;
            }
            return true;
        case 'B': s_boss_x = wx; s_boss_z = wz; return true;   // Malakor so surge depois dos 3 selos
        case 'F': add_scene_prop(SCENE_HELL_BRAZIER, wx, wz, 40.0f, 8.0f); return true;
        case 'M': add_scene_prop(pick3(SCENE_HELL_SPIKES_A, 3), wx, wz, frange(64, 80), 10.0f); return true;
        case 'H': add_scene_prop(SCENE_HELL_THRONE, wx, wz, 86.0f, 0.0f);
                  add_box_collider(wx, wz, 26.0f, 14.0f); return true;
        case 'L': add_scene_prop(SCENE_HELL_LAVA_POOL, wx, wz, 104.0f, 0.0f);
                  add_hazard(wx, wz, 34.0f, false);
                  if (g_campaign == 2) add_slow(wx, wz, 40.0f);      // agua gelada: tambem prende os pes
                  return true;
        case 'f': add_scene_prop(SCENE_HELL_FIREVENT, wx, wz, 52.0f, 0.0f);
                  add_hazard(wx, wz, 16.0f, true); return true;
        case 'l': add_scene_prop(SCENE_HELL_DECAL_CRACK, wx, wz, 60.0f, 0.0f); return true;
        case 'r': add_scene_prop(pick3(SCENE_HELL_ROCKS_A, 4), wx + frange(-8, 8), wz + frange(-6, 6), 24.0f, 0.0f); return true;
        case 'h': add_scene_prop(SCENE_HELL_HANGED, wx, wz, 70.0f, 6.0f); return true;
        case 'x': add_scene_prop(SCENE_HELL_SKULLS, wx, wz, 62.0f, 5.0f); return true;
        case 'g': add_scene_prop(SCENE_HELL_CAGE, wx, wz, 58.0f, 10.0f); return true;
        case 'u': add_scene_prop(SCENE_HELL_STATUE, wx, wz, 54.0f, 12.0f); return true;
        case 'Y': case 'b': add_scene_prop(SCENE_HELL_DECAL_BONES, wx, wz, 50.0f, 0.0f); return true;
        case 'v': add_scene_prop(SCENE_HELL_DECAL_BLOOD, wx, wz, 54.0f, 0.0f); return true;
        case 'k': add_scene_prop(SCENE_HELL_DECAL_CRACK, wx, wz, 58.0f, 0.0f); return true;
        case 'o': add_scene_prop(SCENE_HELL_DECAL_ASH, wx, wz, 58.0f, 0.0f); return true;
        case 's': case 'S': case 'Z': case 'q': case 'a': case 'C':
            return forsaken_place(ad, ch, wx, wz);
        default: return false;
    }
}

// Passo Congelado (Norte, fase 1).
static bool snow_place(AreaData *ad, char ch, float wx, float wz) {
    switch (ch) {
        case 'R': add_scene_prop(pick3(SCENE_SNOW_PINE_A, 3), wx + frange(-12, 12), wz + frange(-10, 10), frange(86, 104), 10.0f); return true;
        case 'K': add_scene_prop(SCENE_SNOW_ROCK_B, wx + frange(-12, 12), wz + frange(-10, 10), frange(22, 28), 0.0f); return true;
        case 'V': add_scene_prop(SCENE_SNOW_ROCK_A, wx, wz, 40.0f, 12.0f); return true;
        case 'U': add_scene_prop(SCENE_SNOW_RUNESTONE, wx, wz, 56.0f, 8.0f); return true;
        case 'B': add_scene_prop(SCENE_SNOW_CART, wx, wz, 40.0f, 0.0f); add_box_collider(wx, wz, 22.0f, 11.0f); return true;
        case '~': add_scene_prop(SCENE_SNOW_CORPSE, wx, wz, 16.0f, 0.0f); return true;
        case 'N': add_scene_prop(SCENE_SNOW_BANNER, wx, wz, 64.0f, 4.0f); return true;
        case 'J': add_scene_prop(SCENE_SNOW_PALISADE, wx, wz, 44.0f, 11.0f); return true;
        case 'Y': add_scene_prop(SCENE_SNOW_BONES, wx, wz, 30.0f, 0.0f); return true;
        case 'F': add_scene_prop(SCENE_SNOW_FIRE, wx, wz, 34.0f, 8.0f); return true;
        case 'I': add_scene_prop(SCENE_SNOW_DECAL_ICE, wx, wz, 76.0f, 0.0f); add_slip(wx, wz, 34.0f); return true;
        case 'd': case 'o': add_scene_prop(SCENE_SNOW_DECAL_DRIFT, wx, wz, 58.0f, 0.0f); return true;
        case 't': add_scene_prop(SCENE_SNOW_DECAL_TRACKS, wx, wz, 50.0f, 0.0f); return true;
        case 'v': add_scene_prop(SCENE_SNOW_DECAL_BLOOD, wx, wz, 52.0f, 0.0f); return true;
        case 'k': add_scene_prop(SCENE_SNOW_DECAL_FROZEN, wx, wz, 56.0f, 0.0f); return true;
        case 'b': add_scene_prop(SCENE_SNOW_DECAL_BONES, wx, wz, 50.0f, 0.0f); return true;
        default: return forsaken_place(ad, ch, wx, wz);
    }
}

// Cavernas de Gelo (Norte, fase 2): tuneis estreitos; entrada e saida sem a arte da cripta.
static bool ice_place(AreaData *ad, char ch, float wx, float wz) {
    switch (ch) {
        case 'L': add_scene_prop(pick3(SCENE_ICE_STALAG_A, 3), wx, wz, 100.0f, 9.0f); return true;
        case 'I': case 'M': add_scene_prop(pick3(SCENE_ICE_CRYSTAL_A, 3), wx, wz, 34.0f, 0.0f); return true;
        case 'V': add_scene_prop(pick3(SCENE_ICE_CRYSTAL_A, 3), wx, wz, 22.0f, 0.0f); return true;
        case 'Q': add_scene_prop(SCENE_ICE_FROZEN_WARRIOR, wx, wz, 58.0f, 9.0f); return true;
        case 'w': add_scene_prop(SCENE_ICE_FROZEN_WOLF, wx, wz, 44.0f, 11.0f); return true;
        case 'A': add_scene_prop(SCENE_ICE_MAMMOTH, wx, wz, 60.0f, 0.0f); add_box_collider(wx, wz, 26.0f, 12.0f); return true;
        case 'F': add_scene_prop(SCENE_ICE_BRAZIER, wx, wz, 38.0f, 8.0f); return true;
        case 'Y': case 'B': case 'W': add_scene_prop(SCENE_ICE_BONES, wx + frange(-8, 8), wz + frange(-6, 6), 24.0f, 0.0f); return true;
        case 'H': add_scene_prop(SCENE_ICE_DECAL_SNOW, wx, wz, 54.0f, 0.0f); return true;
        case 'k': add_scene_prop(SCENE_ICE_DECAL_CRACK, wx, wz, 56.0f, 0.0f); return true;
        case 'o': add_scene_prop(SCENE_ICE_DECAL_FROZEN, wx, wz, 58.0f, 0.0f); return true;
        case 'g': add_scene_prop(SCENE_ICE_DECAL_GLOW, wx, wz, 54.0f, 0.0f); return true;
        case 'b': add_scene_prop(SCENE_ICE_DECAL_BONES, wx, wz, 48.0f, 0.0f); return true;
        case 'v': add_scene_prop(SCENE_ICE_DECAL_BLOOD, wx, wz, 50.0f, 0.0f); return true;
        default: return forsaken_place(ad, ch, wx, wz);   // E, P, marcadores
    }
}

// Pantano Podre (Floresta Morta, fase 2). Marcadores, entrada e saida ficam com o forsaken_place.
static bool swamp_place(AreaData *ad, char ch, float wx, float wz) {
    switch (ch) {
        case 'R': add_scene_prop(pick3(SCENE_SWAMP_TREE_A, 3), wx + frange(-12, 12), wz + frange(-10, 10), frange(84, 100), 10.0f); return true;
        case 'K': add_scene_prop((rand() & 1) ? SCENE_SWAMP_REEDS_A : SCENE_SWAMP_REEDS_B, wx + frange(-14, 14), wz + frange(-12, 12), frange(26, 34), 0.0f); return true;
        case 'H': add_scene_prop(SCENE_SWAMP_SHROOM, wx, wz, 22.0f, 0.0f); return true;
        case 'V': add_scene_prop(SCENE_SWAMP_LOG, wx, wz, 24.0f, 0.0f); return true;
        case 'W': add_scene_prop(SCENE_SWAMP_DECAL_POOL, wx, wz, 104.0f, 0.0f); add_slow(wx, wz, 44.0f); return true;
        case 'w': add_scene_prop(SCENE_SWAMP_DECAL_POOL_S, wx, wz, 62.0f, 0.0f); add_slow(wx, wz, 26.0f); return true;
        case 'U': add_scene_prop(SCENE_SWAMP_TOTEM, wx, wz, 60.0f, 8.0f); return true;
        case 'F': add_scene_prop(SCENE_SWAMP_LANTERN, wx, wz, 52.0f, 6.0f); return true;
        case 'B': add_scene_prop(SCENE_SWAMP_BOAT, wx, wz, 40.0f, 0.0f); add_box_collider(wx, wz, 22.0f, 10.0f); return true;
        case 'h': add_scene_prop(SCENE_SWAMP_HUT, wx, wz, 86.0f, 20.0f); return true;
        case 'Y': add_scene_prop(SCENE_SWAMP_BONES, wx + frange(-8, 8), wz + frange(-6, 6), 24.0f, 0.0f); return true;
        case 'l': add_scene_prop(SCENE_SWAMP_DECAL_LILY, wx, wz, 50.0f, 0.0f); return true;
        case 'g': case 'v': add_scene_prop(SCENE_SWAMP_DECAL_ALGAE, wx, wz, 54.0f, 0.0f); return true;
        case 'm': case 'k': case 'o': add_scene_prop(SCENE_SWAMP_DECAL_MUD, wx, wz, 56.0f, 0.0f); return true;
        case 'b': add_scene_prop(SCENE_SWAMP_DECAL_BONES, wx, wz, 50.0f, 0.0f); return true;
        default: return forsaken_place(ad, ch, wx, wz);
    }
}

// Depois do cenario pronto: sorteia baus, forma os grupos e registra as emboscadas.
static void forsaken_populate(void) {
    int slots[MAX_MARKERS], ns = 0;
    for (int i = 0; i < s_marker_count; i++) if (s_markers[i].ch == 'C') slots[ns++] = i;
    for (int i = ns - 1; i > 0; i--) { int j = rand() % (i + 1); int t = slots[i]; slots[i] = slots[j]; slots[j] = t; }
    int open_n = ns < 4 ? ns : 4; // so 4 vagas de bau abrem por partida
    if (ns <= open_n && ns >= 2) open_n = ns - 1;   // garante uma vaga para o mascate
    // Baus grandes vem selados: cada um tem um evento (emboscada, guardiao ou vigilia) que precisa ser vencido.
    s_cev_count = 0;
    for (int i = 0; i < open_n; i++) {
        float cx = s_markers[slots[i]].x, cz = s_markers[slots[i]].z;
        chest_ev_add(items_add_chest_ex(cx, cz, false, true), cx, cz);
    }
    // Mascate: numa das vagas de bau que sobraram (sorteada a cada partida)
    s_has_vendor = false;
    if (ns > open_n) {
        s_vendor_x = s_markers[slots[open_n]].x; s_vendor_z = s_markers[slots[open_n]].z;
        add_scene_prop(SCENE_VENDOR, s_vendor_x, s_vendor_z, 36.0f, 10.0f);   // mascate + carroca (antes 52: grande demais)
        s_has_vendor = true;
    }
    // Bauzinhos: as vagas que sobraram + alguns espalhados pelo mapa. Abrem direto, saque pequeno.
    int nsmall = 0;
    for (int i = open_n + (s_has_vendor ? 1 : 0); i < ns && nsmall < 3; i++, nsmall++)
        items_add_chest_ex(s_markers[slots[i]].x, s_markers[slots[i]].z, true, false);
    for (int tries = 0; tries < 300 && nsmall < 4; tries++) {
        int c = 1 + rand() % (s_grid_cols - 2), r = 1 + rand() % (s_grid_rowsN - 2);
        if (s_grid[r][c] != '.') continue;
        float x = (c + 0.5f) * TILE - s_half_w + frange(-10, 10), z = s_half_h - (r + 0.5f) * TILE + frange(-10, 10);
        AreaData *ad0 = &g_area_data[g_current_area];
        if (vec2_dist_sq(x, z, ad0->spawn_x, ad0->spawn_z) < 200.0f * 200.0f) continue;
        if (!world_spot_free(x, z, 16.0f)) continue;
        bool far = true;
        const Chest *ch = items_get_chests();
        for (int k = 0; k < MAX_CHESTS && far; k++) if (ch[k].active && vec2_dist_sq(x, z, ch[k].x, ch[k].z) < 260.0f * 260.0f) far = false;
        if (!far) continue;
        if (items_add_chest_ex(x, z, true, false) >= 0) nsmall++;
    }

    for (int i = 0; i < s_marker_count; i++) {
        Marker *m = &s_markers[i];
        if (m->ch == 'C') continue;
        if (m->ch == 'a') {
            if (s_ambush_count < MAX_AMBUSH) s_ambush[s_ambush_count++] = (Ambush){m->x, m->z, false};
            continue;
        }
        // Nao nasce nada agora: so registra o ponto de horda.
        if (s_spawner_count >= MAX_SPAWNERS) continue;
        Spawner *sp = &s_spawners[s_spawner_count++];
        sp->ch = m->ch; sp->x = m->x; sp->z = m->z;
        sp->left = pack_size(m->ch);
        sp->pack = s_pack_seq++;
        sp->elite_pending = (m->ch == 'q');
        if (m->ch == 'q') s_elites_total++;
    }
    s_elites_shown = s_elites_total;
}

// Detalhes aleatorios por partida: manchas de sangue, ossos, rachaduras, arbustos.
static void forsaken_scatter(AreaData *ad) {
    static const ScenePropId decals[3] = { SCENE_DECAL_BLOOD, SCENE_DECAL_BONES, SCENE_DECAL_CRACKS };
    for (int r = 1; r < s_grid_rowsN - 1; r++) for (int c = 1; c < s_grid_cols - 1; c++) {
        if (s_grid[r][c] != '.') continue;
        float wx = (c + 0.5f) * TILE - s_half_w + frange(-22, 22);
        float wz = s_half_h - (r + 0.5f) * TILE + frange(-22, 22);
        if (vec2_dist_sq(wx, wz, ad->spawn_x, ad->spawn_z) < 110.0f * 110.0f) continue;
        if (vec2_dist_sq(wx, wz, ad->exit_x, ad->exit_z) < 90.0f * 90.0f) continue;
        int roll = rand() % 100;
        if (s_altar_theme) {   // inferno: cinzas, sangue, fissuras, ossos e pedras de lava
            static const ScenePropId hd[4] = { SCENE_HELL_DECAL_CRACK, SCENE_HELL_DECAL_BONES, SCENE_HELL_DECAL_ASH, SCENE_HELL_DECAL_BLOOD };
            if (roll < 5)       add_scene_prop(hd[rand() % 4], wx, wz, frange(36, 54), 0.0f);
            else if (roll < 8)  add_scene_prop(pick3(SCENE_HELL_ROCKS_A, 4), wx, wz, frange(18, 26), 0.0f);
            continue;
        }
        if (s_snow_theme) {    // neve: montes, pegadas, pedrinhas
            if (roll < 5)       add_scene_prop((rand() & 1) ? SCENE_SNOW_DECAL_DRIFT : SCENE_SNOW_DECAL_TRACKS, wx, wz, frange(36, 52), 0.0f);
            else if (roll < 8 && !on_trail(wx, wz, 14.0f)) add_scene_prop(SCENE_SNOW_ROCK_B, wx, wz, frange(18, 26), 0.0f);
            continue;
        }
        if (s_ice_theme) {     // gelo: rachaduras, brilho, cristais pequenos
            if (roll < 4)       add_scene_prop((ScenePropId)(SCENE_ICE_DECAL_CRACK + rand() % 3), wx, wz, frange(34, 50), 0.0f);
            else if (roll < 7)  add_scene_prop(pick3(SCENE_ICE_CRYSTAL_A, 3), wx, wz, frange(16, 24), 0.0f);
            else if (roll < 8)  add_scene_prop(SCENE_ICE_BONES, wx, wz, frange(18, 24), 0.0f);
            continue;
        }
        if (s_swamp_theme) {   // pantano: limo, lama, vitorias-regias, juncos e cogumelos
            if (roll < 4)       add_scene_prop((ScenePropId)(SCENE_SWAMP_DECAL_MUD + rand() % 3), wx, wz, frange(36, 52), 0.0f);
            else if (roll < 7 && !on_trail(wx, wz, 14.0f)) add_scene_prop((rand() & 1) ? SCENE_SWAMP_REEDS_A : SCENE_SWAMP_REEDS_B, wx, wz, frange(22, 30), 0.0f);
            else if (roll < 8)  add_scene_prop(SCENE_SWAMP_SHROOM, wx, wz, frange(16, 22), 0.0f);
            continue;
        }
        if (s_crypt_theme) {   // caverna: manchas, pocas, teias, ossos, cogumelos e pedras
            if (roll < 4)       add_scene_prop((ScenePropId)(SCENE_CAVE_DECAL_BLOOD + rand() % 6), wx, wz, frange(34, 50), 0.0f);
            else if (roll < 6)  add_scene_prop(pick3(SCENE_CAVE_MUSH_A, 3), wx, wz, frange(16, 22), 0.0f);
            else if (roll < 8)  add_scene_prop(pick3(SCENE_CAVE_ROCKS_A, 4), wx, wz, frange(18, 24), 0.0f);
            else if (roll < 9)  add_scene_prop((rand() & 1) ? SCENE_CAVE_RIBCAGE : SCENE_CAVE_BONES_B, wx, wz, frange(18, 24), 0.0f);
            continue;
        }
        if (roll < 9)        add_scene_prop(decals[rand() % 3], wx, wz, frange(32, 50), 0.0f);   // deitado: pode ir na trilha
        else if (roll < 17 && on_trail(wx, wz, 16.0f)) continue;                                  // em pe: nunca na trilha
        else if (roll < 12)  add_scene_prop(SCENE_BONE_PILE, wx, wz, frange(20, 26), 0.0f);
        else if (roll < 17 && !s_crypt_theme && !s_altar_theme)  add_scene_prop(SCENE_DEAD_BUSH, wx, wz, frange(20, 28), 0.0f);
    }
}

static int elites_pending(void) {
    int n = 0;
    for (int i = 0; i < s_spawner_count; i++) if (s_spawners[i].elite_pending) n++;
    return n;
}

// Acorda as hordas perto do druida e devolve ao ponto os monstros parados que ficaram muito longe.
static void spawners_update(void) {
    for (int i = 0; i < s_spawner_count; i++) {
        Spawner *sp = &s_spawners[i];
        if (sp->left <= 0 && !sp->elite_pending) continue;
        if (vec2_dist_sq(g_druid.x, g_druid.z, sp->x, sp->z) > SPAWN_NEAR * SPAWN_NEAR) continue;
        if (sp->elite_pending) {
            EnemyType et = s_crypt_theme ? ((rand() % 100 < 55) ? ENEMY_TYPE_DEMON : ENEMY_TYPE_CULTIST)
                                         : ((rand() % 100 < 60) ? ENEMY_TYPE_DEMON : ENEMY_TYPE_SKELETON);
            int idx = world_spot_free(sp->x, sp->z, 14.0f) ? enemies_spawn_ex(et, sp->x, sp->z, sp->pack, true)
                                                          : spawn_pack_member(et, sp->x, sp->z, sp->pack, true, 10.0f, 40.0f);
            if (idx >= 0) sp->elite_pending = false; else continue;   // sem vaga: tenta no proximo quadro
        }
        float rmin = sp->ch == 'q' ? 32.0f : 18.0f, rmax = sp->ch == 'q' ? 70.0f : 75.0f;
        while (sp->left > 0) {
            int idx = -1;
            for (int tries = 0; tries < 6 && idx < 0; tries++) {
                float ang = frand() * 6.2831853f, rad = frange(rmin, rmax);
                float x = sp->x + cosf(ang) * rad, z = sp->z + sinf(ang) * rad;
                if (vec2_dist_sq(x, z, g_druid.x, g_druid.z) < SPAWN_MIN_GAP * SPAWN_MIN_GAP) continue;
                if (!world_spot_free(x, z, 12.0f)) continue;
                idx = enemies_spawn_ex(pick_type(sp->ch), x, z, sp->pack, false);
                if (idx < 0) { tries = 99; }   // sem vaga no limite de monstros
            }
            if (idx < 0) break;                // o resto nasce quando abrir vaga
            sp->left--;
        }
    }
    // Monstros parados longe do druida voltam para o ponto (liberam vaga para hordas novas).
    Enemy *arr = enemies_get_array();
    for (int j = 0; j < MAX_ENEMIES; j++) {
        Enemy *e = &arr[j];
        if (!e->active || e->elite || e->state != ENEMY_STATE_IDLE) continue;
        if (vec2_dist_sq(e->x, e->z, g_druid.x, g_druid.z) < DESPAWN_FAR * DESPAWN_FAR) continue;
        for (int i = 0; i < s_spawner_count; i++)
            if (s_spawners[i].pack == e->pack) { s_spawners[i].left++; e->active = false; break; }
    }
}

// Emboscadas (a terra se abre) e aviso de guardioes restantes.
static void forsaken_update(void) {
    spawners_update();
    if (g_druid.hp > 0) {
        for (int i = 0; i < s_ambush_count; i++) {
            Ambush *a = &s_ambush[i];
            if (a->done) continue;
            if (vec2_dist_sq(g_druid.x, g_druid.z, a->x, a->z) > 170.0f * 170.0f) continue;
            a->done = true;
            int n = 6 + rand() % 4;   // 6..9
            int pack = s_pack_seq++;
            int spawned = 0;
            for (int k = 0; k < n; k++) {
                int idx = spawn_pack_member(pick_type('A'), a->x, a->z, pack, false, 10.0f, 50.0f);
                if (idx >= 0) { enemies_force_chase(idx); spawned++; }
            }
            if (spawned > 0) {
                sound_play(SFX_BOSS_ENRAGE);
                if (s_crypt_theme) hud_show_banner(T(S_AMBUSH_C_T), T(S_AMBUSH_C_S), 2.0f);
                else hud_show_banner(T(S_AMBUSH_T), T(S_AMBUSH_S), 2.0f);
            }
        }
    }
    int left = enemies_count_elites() + elites_pending();
    if (s_elites_total > 0 && left < s_elites_shown) {
        char buf[48];
        if (left > 0) snprintf(buf, sizeof(buf), T(S_GUARD_LEFT), left);
        else          snprintf(buf, sizeof(buf), "%s", T(S_GUARD_ALL));
        hud_show_message(buf, 3.5f);
    }
    s_elites_shown = left;
}

// ---------------------------------------------------------------------------
// Chao em camadas: malha base com variacao de cor + poligonos de trilha/terra/lajes/grama.
// So desenha o que esta perto da camera.
// ---------------------------------------------------------------------------
#define CEM_CELL (TILE * 2.0f)
#define CEM_MAX_BASE_QUADS 400
static T3DVertPacked *s_cem_base = NULL;
static T3DVertPacked *s_cem_floor = NULL;
static int s_cem_nxc = 0, s_cem_nzc = 0;
static int s_cem_layers = 0;      // quads de camadas (trilha/terra/...) do mapa 1; 0 = so a base com variacao (cripta)

static uint32_t floor_color(int gx, int gz, float strength) {
    uint32_t h = ((uint32_t)gx * 73856093u) ^ ((uint32_t)gz * 19349663u);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    float n = (float)(h & 255u) / 255.0f;
    float low = 0.5f + 0.5f * sinf(gx * 0.9f + gz * 0.55f) * cosf(gx * 0.37f - gz * 0.71f);
    float v = 1.0f - strength * (0.55f * (1.0f - low) + 0.45f * (1.0f - n));
    float r = v, g = v * (0.96f + 0.04f * n), b = v * (0.90f + 0.10f * n);
    return tint32(((uint32_t)(r * 255.0f) << 24) | ((uint32_t)(g * 255.0f) << 16) | ((uint32_t)(b * 255.0f) << 8) | 0xFFu);
}

// Quad horizontal com UV ancorada no mundo (a textura nao "pula" entre retangulos vizinhos).
static void cem_quad(T3DVertPacked *base, int *vc, float x0, float z0, float x1, float z1, float y,
                     uint32_t c00, uint32_t c10, uint32_t c11, uint32_t c01) {
    uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0, 1, 0}});
    float bx = floorf(x0 / TILE) * TILE, bz = floorf(z0 / TILE) * TILE;
    const float k = 1024.0f / TILE; // 32 texels em S10.5 por tile
    int16_t s0 = (int16_t)((x0 - bx) * k), s1 = (int16_t)((x1 - bx) * k);
    int16_t t0 = (int16_t)((z0 - bz) * k), t1 = (int16_t)((z1 - bz) * k);
    float X[6] = {x0, x1, x1, x0, x1, x0};
    float Z[6] = {z0, z0, z1, z0, z1, z1};
    int16_t S[6] = {s0, s1, s1, s0, s1, s0};
    int16_t T[6] = {t0, t0, t1, t0, t1, t1};
    uint32_t C[6] = {c00, c10, c11, c00, c11, c01};
    int o = *vc;
    for (int i = 0; i < 6; i++) qset(base, o + i, X[i], y, Z[i], C[i], n, S[i], T[i]);
    *vc = o + 6;
}

// Poligono de 6 vertices (2 triangulos) com UV ancorada no mundo e cor de iluminacao por vertice.
static void cem_tri6(T3DVertPacked *base, int *vc, const FloorQuad *f, float y) {
    float uvm = (f->kind == 0) ? 2.0f : 1.0f; // trilha de pedra: textura repete 2x por tile (pedras menores)
    uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0, 1, 0}});
    int minx = f->x[0], minz = f->z[0];
    for (int i = 1; i < 6; i++) { if (f->x[i] < minx) minx = f->x[i]; if (f->z[i] < minz) minz = f->z[i]; }
    float bx = floorf((float)minx / TILE) * TILE, bz = floorf((float)minz / TILE) * TILE;
    const float k = uvm * 1024.0f / TILE; // 32 texels em S10.5 por tile (x uvm)
    int o = *vc;
    for (int i = 0; i < 6; i++) {
        float x = (float)f->x[i], z = (float)f->z[i];
        int gx = (int)floorf(x / (TILE * 0.5f)), gz = (int)floorf(z / (TILE * 0.5f));
        qset(base, o + i, x, y, z, floor_color(gx, gz, 0.24f), n,
             (int16_t)((x - bx) * k), (int16_t)((z - bz) * k));
    }
    *vc = o + 6;
}

// Altura (em unidades inteiras: o vertice e int16) de cada camada. Ordem de empilhamento:
// grama < terra < trilha < lajes.
static const float CEM_KIND_Y[4] = { 3.0f, 2.0f, 4.0f, 1.0f }; // 0 trilha, 1 terra, 2 lajes, 3 grama
static int16_t s_cem_bb[MG_MAX_FLOOR][4];
static const FloorQuad *s_fl = NULL;   // chao gerado pelo mapgen
static int s_fl_n = 0; // x0, z0, x1, z1 de cada entrada

static void build_cemetery_floor(bool layers, float strength) {
    int nxc = (int)ceilf(s_grid_cols * TILE / CEM_CELL);
    int nzc = (int)ceilf(s_grid_rowsN * TILE / CEM_CELL);
    if (nxc * nzc > CEM_MAX_BASE_QUADS) return;
    s_cem_nxc = nxc; s_cem_nzc = nzc;
    if (!s_cem_base) s_cem_base = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 3 * CEM_MAX_BASE_QUADS);
    s_fl_n = layers ? mapgen_floor(&s_fl) : 0;
    if (layers && !s_cem_floor) s_cem_floor = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 3 * MG_MAX_FLOOR);
    if (!s_cem_base || (layers && !s_cem_floor)) { s_cem_nxc = 0; return; }
    s_cem_layers = layers ? s_fl_n : 0;

    int vc = 0;
    for (int cz = 0; cz < nzc; cz++) for (int cx = 0; cx < nxc; cx++) {
        float x0 = -s_half_w + cx * CEM_CELL, z0 = -s_half_h + cz * CEM_CELL;
        cem_quad(s_cem_base, &vc, x0, z0, x0 + CEM_CELL, z0 + CEM_CELL, 0.0f,
                 floor_color(cx, cz, strength), floor_color(cx + 1, cz, strength),
                 floor_color(cx + 1, cz + 1, strength), floor_color(cx, cz + 1, strength));
    }
    vc = 0;
    for (int i = 0; layers && i < s_fl_n; i++) {
        const FloorQuad *f = &s_fl[i];
        cem_tri6(s_cem_floor, &vc, f, CEM_KIND_Y[f->kind & 3]);
        int16_t x0 = f->x[0], x1 = f->x[0], z0 = f->z[0], z1 = f->z[0];
        for (int j = 1; j < 6; j++) {
            if (f->x[j] < x0) x0 = f->x[j];
            if (f->x[j] > x1) x1 = f->x[j];
            if (f->z[j] < z0) z0 = f->z[j];
            if (f->z[j] > z1) z1 = f->z[j];
        }
        s_cem_bb[i][0] = x0; s_cem_bb[i][1] = z0; s_cem_bb[i][2] = x1; s_cem_bb[i][3] = z1;
    }
}

static bool tri_has(const FloorQuad *f, int o, float px, float pz) {
    float ax = f->x[o], az = f->z[o], bx = f->x[o + 1], bz = f->z[o + 1], cx = f->x[o + 2], cz = f->z[o + 2];
    float d1 = (px - bx) * (az - bz) - (ax - bx) * (pz - bz);
    float d2 = (px - cx) * (bz - cz) - (bx - cx) * (pz - cz);
    float d3 = (px - ax) * (cz - az) - (cx - ax) * (pz - az);
    bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0), pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(neg && pos);
}

// O ponto (ou algum ponto a 'margin' de distancia dele) cai sobre a trilha de pedra?
static bool on_trail(float x, float z, float margin) {
    static const float off[5][2] = { {0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    if (s_cem_nxc <= 0 || s_cem_layers <= 0) return false;
    for (int o = 0; o < 5; o++) {
        float px = x + off[o][0] * margin, pz = z + off[o][1] * margin;
        for (int i = 0; i < s_fl_n; i++) {
            const FloorQuad *f = &s_fl[i];
            if (f->kind != 0) continue;
            if (px < s_cem_bb[i][0] || px > s_cem_bb[i][2] || pz < s_cem_bb[i][1] || pz > s_cem_bb[i][3]) continue;
            if (tri_has(f, 0, px, pz) || tri_has(f, 3, px, pz)) return true;
        }
    }
    return false;
}

// Se (x,z) esta sobre a trilha, move para o ponto livre mais proximo fora dela. false = nao achou.
static bool clear_of_trail(float *x, float *z, float r) {
    if (!on_trail(*x, *z, r)) return true;
    float start = frange(0.0f, 6.28f);
    for (float d = 14.0f; d <= 120.0f; d += 10.0f) {
        for (int k = 0; k < 16; k++) {
            float a = start + k * 0.3927f;
            float nx = *x + cosf(a) * d, nz = *z + sinf(a) * d;
            int c = (int)floorf((nx + s_half_w) / TILE), rr = (int)floorf((s_half_h - nz) / TILE);
            if (c < 1 || rr < 1 || c >= s_grid_cols - 1 || rr >= s_grid_rowsN - 1) continue;
            char tc = s_grid[rr][c];
            if (!tile_walk(tc) || tc == 'J' || tc == 'u' || tc == 'g' || tc == 'P' || tc == 'E') continue;
            if (on_trail(nx, nz, r)) continue;
            if (!world_spot_free(nx, nz, r * 0.7f)) continue;
            *x = nx; *z = nz;
            return true;
        }
    }
    return false;
}

static bool cem_visible(float x0, float z0, float x1, float z1) {
    return x1 > g_druid.x - 300.0f && x0 < g_druid.x + 300.0f &&
           z1 > g_druid.z - 160.0f && z0 < g_druid.z + 300.0f;
}

// Desenha 'quads' quads consecutivos a partir de 'first' (6 vertices cada).
static void cem_draw_run(T3DVertPacked *verts, int first, int quads) {
    uint32_t offset = (uint32_t)first * 6u, remaining = (uint32_t)quads * 6u;
    bool first_chunk = true;
    while (remaining > 0) {
        uint32_t chunk = (remaining > 60) ? 60 : remaining;
        if (!first_chunk) t3d_tri_sync();
        first_chunk = false;
        t3d_vert_load(&verts[offset / 2], 0, chunk);
        t3d_tri_draw_unindexed(0, chunk / 3);
        offset += chunk;
        remaining -= chunk;
    }
    t3d_tri_sync();
}

static void cem_begin_layer(sprite_t *spr) {
    surface_t tex = sprite_get_pixels(spr);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
    rdpq_tex_upload(TILE0, &tex, &(rdpq_texparms_t){
        .s.repeats = REPEAT_INFINITE, .t.repeats = REPEAT_INFINITE
    });
    gfx_push_identity();
}

static void draw_cemetery_floor(void) {
    static sprite_t *tex_k[4]; // 0 trilha, 1 terra, 2 lajes, 3 grama
    static bool loaded = false;
    if (!loaded) {
        tex_k[0] = sprite_load("rom:/scene_floor_trail.rgba16.sprite");
        tex_k[1] = sprite_load("rom:/scene_floor_soil.rgba16.sprite");
        tex_k[2] = sprite_load("rom:/scene_floor_cave.rgba16.sprite");
        tex_k[3] = sprite_load("rom:/scene_floor_grass.rgba16.sprite");
        loaded = true;
    }
    sprite_t *base_tex = s_floor_textures[g_campaign * AREA_COUNT + g_current_area];
    if (!s_cem_base || !base_tex) return;

    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);

    // Base de musgo/terra com variacao de cor (celulas de 2x2 tiles).
    cem_begin_layer(base_tex);
    int quads = s_cem_nxc * s_cem_nzc, run = -1;
    for (int q = 0; q <= quads; q++) {
        bool vis = false;
        if (q < quads) {
            float x0 = -s_half_w + (q % s_cem_nxc) * CEM_CELL, z0 = -s_half_h + (q / s_cem_nxc) * CEM_CELL;
            vis = cem_visible(x0, z0, x0 + CEM_CELL, z0 + CEM_CELL);
        }
        if (vis) { if (run < 0) run = q; }
        else if (run >= 0) { cem_draw_run(s_cem_base, run, q - run); run = -1; }
    }
    t3d_matrix_pop(1);

    // Camadas por cima, de baixo para cima: grama morta, terra revirada, trilha e lajes.
    static const int order[4] = { 3, 1, 0, 2 };
    for (int oi = 0; oi < (s_cem_layers > 0 ? 4 : 0); oi++) {
        int k = order[oi];
        if (!tex_k[k]) continue;
        bool began = false;
        run = -1;
        for (int i = 0; i <= s_fl_n; i++) {
            bool vis = false;
            if (i < s_fl_n) {
                vis = (s_fl[i].kind == k) &&
                      cem_visible(s_cem_bb[i][0], s_cem_bb[i][1], s_cem_bb[i][2], s_cem_bb[i][3]);
            }
            if (vis) { if (run < 0) run = i; }
            else if (run >= 0) {
                if (!began) { cem_begin_layer(tex_k[k]); began = true; }
                cem_draw_run(s_cem_floor, run, i - run);
                run = -1;
            }
        }
        if (began) t3d_matrix_pop(1);
    }

    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
}

void world_init(void) {
    debugf("[WORLD] Initializing world system...\n");
    world_load_area(AREA_FORSAKEN_PATH);
}

static void load_grid(const char *const *map, int rows, int cols) {
    AreaId a = g_current_area; AreaData *ad = &g_area_data[a];
    s_grid = map; s_grid_rowsN = rows; s_grid_cols = cols;
    s_half_w = cols*TILE*0.5f; s_half_h = rows*TILE*0.5f;
    ad->min_x=-s_half_w; ad->max_x=s_half_w; ad->min_z=-s_half_h; ad->max_z=s_half_h;
    for (int i=0; i<cols*rows && i<MAX_GRID_TILES; i++) s_col_head[i]=-1;
    memset(s_explored, 0, sizeof(s_explored));
    s_marker_count=0; s_cev_count=0; s_ambush_count=0; s_spawner_count=0; s_elites_total=0; s_elites_shown=0; s_pack_seq=1;
    build_floor(tint32(0xFFFFFFFFu), cols, rows);
    build_walls(map, rows, cols, a == AREA_FORSAKEN_PATH ? WALL_H_LOW : a == AREA_HOLLOW_CRYPT ? WALL_H_CRYPT : WALL_H); // paredes 3D (substitui billboards)
    s_cem_nxc = 0; s_cem_layers = 0;
    s_swamp_theme = (a == AREA_HOLLOW_CRYPT && g_campaign == 1);
    s_ice_theme = (a == AREA_HOLLOW_CRYPT && g_campaign == 2);
    s_snow_theme = (a == AREA_FORSAKEN_PATH && g_campaign == 2);
    s_crypt_theme = (a == AREA_HOLLOW_CRYPT && !s_swamp_theme && !s_ice_theme);
    s_slip_count = 0;
    s_altar_theme = (a == AREA_BLACK_ALTAR);
    s_slow_count = 0;
    s_seal_count = 0; s_seals_done = 0; s_boss_summoned = false; s_hazard_count = 0;
    if (a == AREA_FORSAKEN_PATH || s_swamp_theme) build_cemetery_floor(true, s_snow_theme ? 0.22f : 0.42f);   // pantano/neve: trilha + manchas
    else if (a == AREA_HOLLOW_CRYPT) build_cemetery_floor(false, 0.34f);

    // Textura de parede da area (64x32, cabe na TMEM), em cache por area.
    int st = g_campaign * AREA_COUNT + a;
    if (!s_wall_textures[st]) {
        static const char *wall_paths[STAGE_COUNT] = {
            "rom:/scene_wall_path.rgba16.sprite",
            "rom:/scene_wall_cave.rgba16.sprite",
            "rom:/scene_wall_hell.rgba16.sprite",
            "rom:/scene_wall_path.rgba16.sprite",
            "rom:/scene_wall_swamp.rgba16.sprite",
            "rom:/scene_wall_heart.rgba16.sprite",
            "rom:/scene_wall_snow.rgba16.sprite",
            "rom:/scene_wall_ice.rgba16.sprite",
            "rom:/scene_wall_frost.rgba16.sprite"
        };
        s_wall_textures[st] = sprite_load(wall_paths[st]);
    }
    s_wall_tex = s_wall_textures[st];
    if (!s_wall_top_textures[st]) {
        static const char *wall_top_paths[STAGE_COUNT] = {
            "rom:/scene_wall_path_top.rgba16.sprite",
            "rom:/scene_wall_cave_top.rgba16.sprite",
            "rom:/scene_wall_hell_top.rgba16.sprite",
            "rom:/scene_wall_path_top.rgba16.sprite",
            "rom:/scene_wall_swamp_top.rgba16.sprite",
            "rom:/scene_wall_heart_top.rgba16.sprite",
            "rom:/scene_wall_snow_top.rgba16.sprite",
            "rom:/scene_wall_ice_top.rgba16.sprite",
            "rom:/scene_wall_frost_top.rgba16.sprite"
        };
        s_wall_top_textures[st] = sprite_load(wall_top_paths[st]);
    }
    s_wall_top_tex = s_wall_top_textures[st];

    ScenePropId fire = (a==AREA_FORSAKEN_PATH)?SCENE_CAMPFIRE : (a==AREA_HOLLOW_CRYPT)?SCENE_CRYPT_BRAZIER : SCENE_INFERNAL_BRAZIER;
    ScenePropId body = (a==AREA_HOLLOW_CRYPT)?SCENE_CULTIST_BODY : SCENE_TRAVELER_BODY;
    ScenePropId portal = (a==AREA_FORSAKEN_PATH)?SCENE_PORTAL_PATH : (a==AREA_HOLLOW_CRYPT)?SCENE_PORTAL_CRYPT : SCENE_PORTAL_ALTAR;
    EnemyType cyc[3] = { ENEMY_TYPE_SKELETON, ENEMY_TYPE_DEMON, ENEMY_TYPE_CULTIST };
    int pminr=9999,pmaxr=-1,pminc=9999,pmaxc=-1, ecount=0;

    for (int r=0;r<rows;r++) for (int c=0;c<cols;c++){
        char ch=map[r][c];
        float wx=(c+0.5f)*TILE - s_half_w;
        float wz=s_half_h - (r+0.5f)*TILE;
        if (a == AREA_FORSAKEN_PATH && g_campaign == 1 && strchr("HVBWIM", ch) && crypt_place(ad, ch, wx, wz)) continue;   // floresta: cogumelos e pedras da caverna
        if (a == AREA_FORSAKEN_PATH && forsaken_place(ad, ch, wx, wz)) continue;
        if (a == AREA_HOLLOW_CRYPT && g_campaign == 1 && swamp_place(ad, ch, wx, wz)) continue;
        if (a == AREA_HOLLOW_CRYPT && g_campaign == 2 && ice_place(ad, ch, wx, wz)) continue;
        if (a == AREA_FORSAKEN_PATH && g_campaign == 2 && snow_place(ad, ch, wx, wz)) continue;
        if (a == AREA_HOLLOW_CRYPT && crypt_place(ad, ch, wx, wz)) continue;
        if (a == AREA_BLACK_ALTAR && altar_place(ad, ch, wx, wz)) continue;
        switch(ch){
            case '#': break; // parede desenhada pela malha 3D; colisao via grade
            case 'E': ad->spawn_x=wx; ad->spawn_z=wz; ad->spawn_yaw=1.5707963f; break;
            case 'O': ad->exit_x=wx; ad->exit_z=wz; ad->exit_radius=40.0f;
                      add_scene_prop(portal, wx, wz, 72.0f, 0.0f); break; // portal de saida
            case 'D': ad->exit_x=wx; ad->exit_z=wz; ad->exit_radius=40.0f;
                      add_scene_prop(SCENE_CRYPT_ENTRANCE, wx, wz, 110.0f, 0.0f); break; // mausoleu = descida p/ cripta (saida unica)
            case 'T': add_scene_prop(SCENE_GRAVESTONE, wx, wz, 40.0f, 8.0f); break;
            case 'X': add_scene_prop(SCENE_GRAVE_CROSS, wx, wz, 44.0f, 7.0f); break;
            case 'U': add_scene_prop(SCENE_TOMB_STATUE, wx, wz, 58.0f, 10.0f); break;
            case 'G': add_scene_prop(SCENE_GREAT_TREE, wx, wz, 160.0f, 16.0f); break;
            case 'K': add_scene_prop(SCENE_DEAD_BUSH, wx, wz, 26.0f, 0.0f); break;
            case 'N': add_scene_prop(SCENE_CRYPT_ENTRANCE, wx, wz, 100.0f, 22.0f); break;
            case 'J': add_scene_prop(SCENE_GRAVE_FENCE, wx, wz, 36.0f, 11.0f); break;
            case 'R': add_scene_prop(SCENE_TREE_DEAD,  wx, wz, 60.0f, 10.0f); break;
            case 'F': add_scene_prop(fire, wx, wz, 22.0f, 6.0f); break;
            case 'M': add_scene_prop(SCENE_MONOLITH, wx, wz, 52.0f, 12.0f); break;
            case 'H': add_scene_prop(SCENE_MALAKOR_THRONE, wx, wz, 60.0f, 16.0f); break;
            case '~': add_scene_prop(body, wx, wz, 9.0f, 0.0f); break;
            case 'C': items_add_chest(wx, wz, 0.0f); break;
            case 'S': enemies_spawn(cyc[ecount%3], wx, wz); ecount++; break;
            case 'B': enemies_spawn(ENEMY_TYPE_BOSS_MALAKOR, wx, wz); break;
            case 'P': if(r<pminr)pminr=r; if(r>pmaxr)pmaxr=r; if(c<pminc)pminc=c; if(c>pmaxc)pmaxc=c; break;
            default: break;
        }
    }
    if (pmaxr>=0){
        float cx=((pminc+pmaxc+1)*0.5f)*TILE - s_half_w;
        float cz=s_half_h - ((pminr+pmaxr+1)*0.5f)*TILE;
        float half=(float)(pmaxc-pminc+1)*TILE*0.5f;
        build_penta(cx, cz, half); // pentagrama deitado no chao (quad 3D)
    }
    if (a == AREA_FORSAKEN_PATH || a == AREA_HOLLOW_CRYPT || a == AREA_BLACK_ALTAR) { forsaken_populate(); events_setup(); forsaken_scatter(ad); }
}

void world_load_area(AreaId area) {
    if (area >= AREA_COUNT) area = AREA_FORSAKEN_PATH;
    g_current_area = area;
    s_collider_count = 0;
    s_grid = NULL;
    s_scene_prop_count = 0;
    s_wall_vert_count = 0;
    s_wall_top_vert_count = 0;
    s_penta_vert_count = 0;

    int stg = g_campaign * AREA_COUNT + area;
    if (!s_floor_textures[stg]) {
        static const char *floor_paths[STAGE_COUNT] = {
            "rom:/scene_floor_moss.rgba16.sprite",
            "rom:/scene_floor_cave.rgba16.sprite",
            "rom:/scene_floor_hell.rgba16.sprite",
            "rom:/scene_floor_moss.rgba16.sprite",
            "rom:/scene_floor_swamp.rgba16.sprite",
            "rom:/scene_floor_heart.rgba16.sprite",
            "rom:/scene_floor_snow.rgba16.sprite",
            "rom:/scene_floor_ice.rgba16.sprite",
            "rom:/scene_floor_frost.rgba16.sprite"
        };
        s_floor_textures[stg] = sprite_load(floor_paths[stg]);
    }

    s_load_serial++;
    {   // semente nova a cada partida: cada tentativa gera um cemiterio diferente (roguelike)
        static bool seeded = false;
        uint32_t t = (uint32_t)get_ticks();
        if (!seeded) { srand(t); seeded = true; }
        else srand((unsigned)rand() ^ t);
    }

    // Curva de dificuldade: cada area bem mais dura que a anterior (vida, dano e agressividade).
    g_enemy_power = (area == AREA_HOLLOW_CRYPT) ? 1.5f : (area == AREA_BLACK_ALTAR) ? 2.1f : 1.0f;
    g_enemy_aggro = (area == AREA_HOLLOW_CRYPT) ? 0.88f : (area == AREA_BLACK_ALTAR) ? 0.78f : 1.0f;   // altar: bem mais dificil   // cripta: monstros mais fortes
    enemies_clear();
    items_clear();
    spells_clear_all();

    AreaData *ad = &g_area_data[area];
    ad->exit_unlocked = false;

    // Reset player position at entrance
    druid_reset_at(ad->spawn_x, ad->spawn_z);
    g_druid.yaw = ad->spawn_yaw;
    g_druid.target_yaw = ad->spawn_yaw;

    debugf("[WORLD] Loading area %d: %s\n", (int)area, ad->name);

    {   // campanha: Floresta Morta = mesma arte com tinta de musgo, ceu/neblina esverdeados
        static AreaData orig[AREA_COUNT];
        static bool saved = false;
        if (!saved) { memcpy(orig, g_area_data, sizeof(orig)); saved = true; }
        ad->clear_color = orig[area].clear_color; ad->fog_color = orig[area].fog_color;
        s_tr = s_tg = s_tb = 1.0f; s_prop_tint = 0; s_mob_tint = 0;
        if (g_campaign == 2) {                    // Norte: arte propria; monstros azulados pelo frio
            s_mob_tint = 0xB0CCF0FFu;
            if (area == AREA_FORSAKEN_PATH)      { ad->clear_color = RGBA32(70, 80, 96, 255); ad->fog_color = RGBA32(150, 160, 178, 255); }   // neve: neblina clara
            else if (area == AREA_HOLLOW_CRYPT)  { ad->clear_color = RGBA32(6, 12, 22, 255);  ad->fog_color = RGBA32(14, 26, 44, 255); }
            else                                 { ad->clear_color = RGBA32(10, 12, 18, 255); ad->fog_color = RGBA32(24, 30, 42, 255); }
        }
        if (g_campaign == 1) {
            // so a floresta (fase 1) reaproveita as texturas do cemiterio com tinta de musgo;
            // pantano e coracao seco tem arte propria (tools/import_forest_stages.py)
            if (area == AREA_FORSAKEN_PATH) { s_tr = 0.78f; s_tg = 0.92f; s_tb = 0.66f; }
            s_prop_tint = 0;
            s_mob_tint = 0xB4DC96FFu;                                        // monstros podres, cor de musgo
            if (area == AREA_HOLLOW_CRYPT) {          // pantano: neblina verde-petroleo
                ad->clear_color = RGBA32(8, 16, 16, 255);
                ad->fog_color = RGBA32(18, 30, 28, 255);
            } else if (area == AREA_BLACK_ALTAR) {    // coracao seco: fim de tarde ambar
                ad->clear_color = RGBA32(18, 12, 6, 255);
                ad->fog_color = RGBA32(36, 26, 14, 255);
            } else {
                ad->clear_color = RGBA32(10, 18, 12, 255);
                ad->fog_color = RGBA32(14, 24, 16, 255);
            }
        }
    }

    if (area == AREA_FORSAKEN_PATH || area == AREA_HOLLOW_CRYPT) {   // mapa novo a cada partida
        int mr = 0, mc = 0;
        int kind = (area == AREA_HOLLOW_CRYPT ? 1 : 0) + (g_campaign == 1 ? 2 : 0);
        const char *const *m = mapgen_build(kind, &mr, &mc);
        load_grid(m, mr, mc);
    }
    else if (area == AREA_BLACK_ALTAR)   load_grid(MAP_ALT,   MAP_ALT_ROWS,   MAP_ALT_COLS);

    // (re)posiciona o jogador na entrada definida pela grade
    druid_reset_at(ad->spawn_x, ad->spawn_z);
    g_druid.yaw = ad->spawn_yaw; g_druid.target_yaw = ad->spawn_yaw;
    hud_show_banner(area_name(g_campaign * AREA_COUNT + area), area_subtitle(g_campaign * AREA_COUNT + area), 3.0f);
    if ((area == AREA_FORSAKEN_PATH || area == AREA_HOLLOW_CRYPT) && s_elites_total > 0) {
        char buf[80];
        snprintf(buf, sizeof(buf), area == AREA_HOLLOW_CRYPT ? T(S_GUARD_PORTAL) : T(S_GUARD_MAUS), s_elites_total);
        hud_show_message(buf, 6.0f);
    } else if (area == AREA_BLACK_ALTAR) {
        hud_show_message(T(g_campaign == 2 ? S_SEALS_MSG_N : g_campaign == 1 ? S_SEALS_MSG_F : S_SEALS_MSG), 6.0f);
    }
    sprites_preload_area();   // Expansion Pak: monstros ja na memoria antes da primeira horda
}

// ---- Eventos secretos da Floresta Morta (Pesadelo/Inferno). O jogo nunca explica como liberar nada. ----
// Santuario da semente: ficar no circulo enquanto as ondas chegam (um por fase) -> Semente do Bosque.
// Jaula de raizes: um guardiao forte; depois segurar ao lado da jaula -> a cacadora e libertada.
enum { EV_NONE, EV_SHRINE, EV_CAGE };
typedef struct { int type, state; float x, z, progress, wave_t, fail_hp; int prop, decal, guard; } WEvent;
#define MAX_WEVENTS 2
static WEvent s_ev[MAX_WEVENTS];
static int s_ev_count = 0;
static int s_cage_stage = -1;          // fase em que a jaula aparece nesta run (-1 = nenhuma)
static bool s_stage_seed[AREA_COUNT];  // semente desta fase ja pega

static bool pick_event_spot(float *ox, float *oz) {
    AreaData *ad = &g_area_data[g_current_area];
    for (int tries = 0; tries < 400; tries++) {
        int c = 2 + rand() % (s_grid_cols - 4), r = 2 + rand() % (s_grid_rowsN - 4);
        bool ok = true;
        for (int dr = -1; dr <= 1 && ok; dr++) for (int dc = -1; dc <= 1 && ok; dc++) if (s_grid[r + dr][c + dc] == '#') ok = false;
        if (!ok || s_grid[r][c] != '.') continue;
        float x = (c + 0.5f) * TILE - s_half_w, z = s_half_h - (r + 0.5f) * TILE;
        if (vec2_dist_sq(x, z, ad->spawn_x, ad->spawn_z) < 420.0f * 420.0f) continue;
        if (vec2_dist_sq(x, z, ad->exit_x, ad->exit_z) < 200.0f * 200.0f) continue;
        if (!world_spot_free(x, z, 20.0f)) continue;
        bool far = true;
        for (int i = 0; i < s_ev_count; i++) if (vec2_dist_sq(x, z, s_ev[i].x, s_ev[i].z) < 300.0f * 300.0f) far = false;
        if (!far) continue;
        *ox = x; *oz = z;
        return true;
    }
    return false;
}

static void events_setup(void) {
    s_ev_count = 0;
    if (g_campaign != 1 || g_difficulty < DIFF_NIGHTMARE) return;
    if (g_current_area == AREA_FORSAKEN_PATH) {               // comeco da campanha: sorteia a jaula da run
        memset(s_stage_seed, 0, sizeof(s_stage_seed));
        s_cage_stage = (!hero_unlocked(HERO_HUNTRESS) && rand() % 100 < 40) ? rand() % AREA_COUNT : -1;
    }
    float x, z;
    if (!s_stage_seed[g_current_area] && pick_event_spot(&x, &z)) {
        WEvent *e = &s_ev[s_ev_count++];
        *e = (WEvent){ EV_SHRINE, 0, x, z, 0, 0, 0, -1, -1, -1 };
        e->decal = s_scene_prop_count; add_scene_prop(SCENE_HELL_DECAL_RITUAL, x, z, 104.0f, 0.0f);
        e->prop = s_scene_prop_count;  add_scene_prop(SCENE_PATH_SHRINE, x, z + 12.0f, 40.0f, 10.0f);
    }
    if (s_cage_stage == (int)g_current_area && !hero_unlocked(HERO_HUNTRESS) && pick_event_spot(&x, &z)) {
        WEvent *e = &s_ev[s_ev_count++];
        *e = (WEvent){ EV_CAGE, 0, x, z, 0, 0, 0, -1, -1, -1 };
        e->prop = s_scene_prop_count; add_scene_prop(SCENE_HELL_CAGE, x, z, 58.0f, 10.0f);
    }
}

static void event_waves(WEvent *e, float dt, int n) {
    e->wave_t -= dt;
    if (e->wave_t > 0.0f) return;
    e->wave_t = 2.6f;
    int pack = s_pack_seq++;
    for (int k = 0; k < n; k++) {
        int idx = spawn_pack_member(pick_type('A'), e->x, e->z, pack, false, 130.0f, 180.0f);
        if (idx >= 0) enemies_force_chase(idx);
    }
}

static void events_update(float dt) {
    for (int i = 0; i < s_ev_count; i++) {
        WEvent *e = &s_ev[i];
        if (e->state >= 3) continue;                                   // 3 = concluido, 4 = perdido
        float d2 = vec2_dist_sq(g_druid.x, g_druid.z, e->x, e->z);
        if (e->type == EV_SHRINE) {
            bool inside = d2 < 46.0f * 46.0f && g_druid.hp > 0;
            if (inside && e->state == 0) {                              // desperta: um guardiao no meio
                e->state = 1; e->wave_t = 1.0f;
                hud_show_banner(T(S_SHRINE_T), T(S_SHRINE_S), 2.0f);
                sound_play(SFX_BOSS_ENRAGE);
                int idx = spawn_pack_member(ENEMY_TYPE_DEMON, e->x, e->z, s_pack_seq++, true, 90.0f, 120.0f);
                if (idx >= 0) enemies_force_chase(idx);
            }
            if (e->state != 1) continue;
            if (inside) { e->progress += dt / 18.0f; event_waves(e, dt, 4 + g_difficulty); }
            else { e->progress -= dt / 30.0f; if (e->progress < 0.0f) e->progress = 0.0f; }
            if (e->progress >= 1.0f) {
                e->state = 3;
                s_stage_seed[g_current_area] = true;
                if (g_seeds < 3) g_seeds++;
                cards_refresh_stats();                                  // a semente pesa: menos vida maxima
                if (e->decal >= 0) s_scene_props[e->decal].id = scene_skin(SCENE_HELL_DECAL_ASH);
                sound_play(SFX_VICTORY);
                hud_show_banner(T(S_SEED_T), T(S_SEED_S), 3.0f);
            }
        } else if (e->type == EV_CAGE) {
            if (e->state == 0 && d2 < 260.0f * 260.0f) {              // o guardiao da jaula aparece
                e->state = 1;
                int pack = s_pack_seq++;
                e->guard = spawn_pack_member(ENEMY_TYPE_CULTIST, e->x, e->z, pack, true, 40.0f, 70.0f);
                if (e->guard >= 0) { enemies_set_affixes(e->guard, 2); enemies_force_chase(e->guard); }
                for (int k = 0; k < 6; k++) {
                    int idx = spawn_pack_member(pick_type('Z'), e->x, e->z, pack, false, 50.0f, 110.0f);
                    if (idx >= 0) enemies_force_chase(idx);
                }
                sound_play(SFX_BOSS_ENRAGE);
            } else if (e->state == 1) {
                Enemy *en = enemies_get_array();
                if (e->guard < 0 || !en[e->guard].active || en[e->guard].hp <= 0 || !en[e->guard].elite) {
                    e->state = 2; e->wave_t = 1.5f;                     // guardiao caiu: as raizes fecham
                    hud_show_banner(T(S_RESCUE_T), T(S_RESCUE_S), 2.5f);
                    sound_play(SFX_THORN_BURST);
                }
            } else if (e->state == 2) {
                if (g_druid.hp <= 0) {                                  // morreu no resgate: perdeu
                    e->state = 4;
                    if (e->prop >= 0) s_scene_props[e->prop].id = scene_skin(SCENE_HELL_DECAL_BONES);
                    hud_show_message(T(S_RESCUE_FAIL), 4.0f);
                    continue;
                }
                bool near = d2 < 52.0f * 52.0f;
                if (near) e->progress += dt / 20.0f;
                else { e->progress -= dt / 25.0f; if (e->progress < 0.0f) e->progress = 0.0f; }
                event_waves(e, dt, 3 + g_difficulty);
                if (e->progress >= 1.0f) {
                    e->state = 3;
                    if (e->prop >= 0) s_scene_props[e->prop].id = scene_skin(SCENE_HELL_DECAL_CRACK);   // a jaula se abre
                    progress_unlock_hero(HERO_HUNTRESS);
                    sound_play(SFX_VICTORY);
                    hud_show_banner(T(S_RESCUE_OK_T), T(S_RESCUE_OK_S), 4.0f);
                }
            }
        }
    }
}

// ---- Eventos dos baus grandes: o bau fica selado ate o evento em volta ser vencido. ----
enum { CE_AMBUSH, CE_GUARD, CE_VIGIL, CE_TYPES };
typedef struct { int chest, type, state, pack, decal, wave; float x, z, progress, wave_t; } ChestEv;
#define MAX_CHEST_EV 6
static ChestEv s_cev[MAX_CHEST_EV];
#define CEV_VIGIL_R 50.0f

static void chest_ev_add(int chest, float x, float z) {
    if (chest < 0 || s_cev_count >= MAX_CHEST_EV) { items_unlock_chest(chest); return; }
    ChestEv *e = &s_cev[s_cev_count];
    // tipos variados: evita repetir o mesmo tipo no bau seguinte
    int t = rand() % CE_TYPES;
    if (s_cev_count > 0 && t == s_cev[s_cev_count - 1].type) t = (t + 1 + rand() % (CE_TYPES - 1)) % CE_TYPES;
    *e = (ChestEv){ chest, t, 0, -1, -1, 0, x, z, 0.0f, 0.0f };
    int before = s_scene_prop_count;
    add_scene_prop(SCENE_HELL_DECAL_RITUAL, x, z, t == CE_VIGIL ? 104.0f : 70.0f, 0.0f);   // circulo marca o selo
    if (s_scene_prop_count > before) e->decal = before;
    s_cev_count++;
}

static int pack_alive(int pack) {
    if (pack < 0) return 0;
    Enemy *en = enemies_get_array();
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (en[i].active && en[i].pack == pack && en[i].hp > 0 && en[i].state != ENEMY_STATE_DEAD) n++;
    return n;
}

static void chest_ev_spawn(ChestEv *e, int n, char kind, float rmin, float rmax) {
    e->pack = s_pack_seq++;
    for (int k = 0; k < n; k++) {
        int idx = spawn_pack_member(pick_type(kind), e->x, e->z, e->pack, false, rmin, rmax);
        if (idx >= 0) enemies_force_chase(idx);
    }
}

static void chest_ev_finish(ChestEv *e) {
    e->state = 3;
    items_unlock_chest(e->chest);
    if (e->decal >= 0) s_scene_props[e->decal].id = scene_skin(SCENE_HELL_DECAL_ASH);
    sound_play(SFX_THORN_BURST);
    hud_show_message(T(S_CEV_FREE), 2.5f);
}

static void chest_events_update(float dt) {
    int lvl = (int)g_current_area + 2 * (int)g_difficulty;     // 0..6: fase e dificuldade apertam o evento
    for (int i = 0; i < s_cev_count; i++) {
        ChestEv *e = &s_cev[i];
        if (e->state >= 3) continue;
        float d2 = vec2_dist_sq(g_druid.x, g_druid.z, e->x, e->z);
        bool alive = g_druid.hp > 0;
        switch (e->type) {
        case CE_AMBUSH:     // duas levas saem das sombras em volta do bau
            if (e->state == 0) {
                if (!alive || d2 > 80.0f * 80.0f) break;
                e->state = 1; e->wave = 0;
                chest_ev_spawn(e, 5 + lvl, 'Z', 70.0f, 130.0f);
                hud_show_banner(T(S_CEV_AMB_T), T(S_CEV_AMB_S), 2.0f);
                sound_play(SFX_BOSS_ENRAGE);
            } else if (pack_alive(e->pack) == 0) {
                if (e->wave == 0) {
                    e->wave = 1;
                    chest_ev_spawn(e, 4 + lvl, 'A', 110.0f, 170.0f);
                    hud_show_message(T(S_CEV_WAVE), 2.0f);
                } else chest_ev_finish(e);
            }
            break;
        case CE_GUARD: {    // um guardiao amaldicoado (nao e elite: nao conta para o portao) e escolta
            if (e->state == 0) {
                if (!alive || d2 > 110.0f * 110.0f) break;
                e->state = 1;
                e->pack = s_pack_seq++;
                EnemyType gt = (g_current_area == AREA_FORSAKEN_PATH) ? ENEMY_TYPE_CULTIST : ENEMY_TYPE_DEMON;
                int g = spawn_pack_member(gt, e->x, e->z, e->pack, false, 30.0f, 60.0f);
                if (g >= 0) {
                    Enemy *en = enemies_get_array();
                    en[g].max_hp = en[g].hp = en[g].hp * (3 + g_difficulty);
                    en[g].damage += en[g].damage / 3;
                    en[g].exp_reward *= 3;
                    enemies_set_affixes(g, 1 + (g_difficulty > 0));
                    enemies_force_chase(g);
                }
                for (int k = 0; k < 2 + lvl / 2; k++) {
                    int idx = spawn_pack_member(pick_type('S'), e->x, e->z, e->pack, false, 50.0f, 110.0f);
                    if (idx >= 0) enemies_force_chase(idx);
                }
                hud_show_banner(T(S_CEV_GRD_T), T(S_CEV_GRD_S), 2.0f);
                sound_play(SFX_BOSS_ENRAGE);
            } else if (pack_alive(e->pack) == 0) chest_ev_finish(e);
            break; }
        case CE_VIGIL: {    // ficar no circulo enquanto as levas chegam; sair faz o selo voltar
            bool inside = alive && d2 < CEV_VIGIL_R * CEV_VIGIL_R;
            if (e->state == 0) {
                if (!inside) break;
                e->state = 1; e->wave_t = 1.0f;
                hud_show_banner(T(S_CEV_VIG_T), T(S_CEV_VIG_S), 2.0f);
                sound_play(SFX_BOSS_ENRAGE);
            }
            if (inside) e->progress += dt / (12.0f + lvl);
            else { e->progress -= dt / 20.0f; if (e->progress < 0.0f) e->progress = 0.0f; }
            if (inside || e->progress > 0.0f) {
                e->wave_t -= dt;
                if (e->wave_t <= 0.0f) {
                    e->wave_t = 2.8f;
                    int pack = s_pack_seq++;
                    for (int k = 0; k < 3 + lvl / 2; k++) {
                        int idx = spawn_pack_member(pick_type('A'), e->x, e->z, pack, false, 130.0f, 180.0f);
                        if (idx >= 0) enemies_force_chase(idx);
                    }
                }
            }
            if (e->progress >= 1.0f) chest_ev_finish(e);
            break; }
        }
    }
}

// Selos do Altar: purificar = ficar no circulo; levas de monstros chegam enquanto isso.
static void seals_update(float dt) {
    if (s_boss_summoned || g_druid.hp <= 0) return;
    for (int i = 0; i < s_seal_count; i++) {
        Seal *sl = &s_seals[i];
        if (sl->done) continue;
        bool inside = vec2_dist_sq(g_druid.x, g_druid.z, sl->x, sl->z) < SEAL_RADIUS * SEAL_RADIUS;
        if (inside) {
            if (sl->progress <= 0.0f) {
                hud_show_banner(T(S_SEAL_T), T(S_SEAL_S), 2.0f);
                sl->wave_t = 0.8f;
                if (!sl->elite_sent) {       // cada selo tem um guardiao (carta garantida)
                    sl->elite_sent = true;
                    EnemyType et = (rand() % 100 < 60) ? ENEMY_TYPE_DEMON : ENEMY_TYPE_CULTIST;
                    int idx = spawn_pack_member(et, sl->x, sl->z, s_pack_seq++, true, 120.0f, 170.0f);
                    if (idx >= 0) enemies_force_chase(idx);
                }
            }
            sl->progress += dt / SEAL_TIME;
            sl->wave_t -= dt;
            if (sl->wave_t <= 0.0f) {       // leva de monstros vindo de fora do circulo
                sl->wave_t = SEAL_WAVE_T;
                int pack = s_pack_seq++;
                int n = 3 + (s_seals_done);   // cada selo seguinte e mais dificil
                for (int k = 0; k < n; k++) {
                    int idx = spawn_pack_member(pick_type('A'), sl->x, sl->z, pack, false, 140.0f, 190.0f);
                    if (idx >= 0) enemies_force_chase(idx);
                }
            }
        } else if (sl->progress > 0.0f) {
            sl->progress -= dt / (SEAL_TIME * 2.5f);   // fora do circulo o selo se recompoe devagar
            if (sl->progress < 0.0f) sl->progress = 0.0f;
        }
        if (sl->progress >= 1.0f) {
            sl->done = true;
            s_seals_done++;
            if (sl->decal >= 0 && sl->decal < s_scene_prop_count)
                s_scene_props[sl->decal].id = scene_skin(SCENE_HELL_DECAL_ASH);       // o circulo vira cinzas
            if (sl->obelisk >= 0 && sl->obelisk < s_scene_prop_count)
                s_scene_props[sl->obelisk].id = scene_skin(SCENE_HELL_SEAL_OFF);      // o obelisco racha e apaga
            sound_play(SFX_VICTORY);
            if (s_seals_done < s_seal_count) {
                char buf[48];
                snprintf(buf, sizeof(buf), T(S_SEAL_DONE_S), s_seals_done, s_seal_count);
                hud_show_banner(T(S_SEAL_DONE_T), buf, 2.5f);
            }
        }
    }
    if (s_seal_count > 0 && s_seals_done >= s_seal_count) {
        s_boss_summoned = true;
        enemies_spawn(ENEMY_TYPE_BOSS_MALAKOR, s_boss_x, s_boss_z);
        int pack = s_pack_seq++;
        for (int k = 0; k < 6; k++) {         // guarda de honra
            int idx = spawn_pack_member(ENEMY_TYPE_DEMON, s_boss_x, s_boss_z, pack, false, 60.0f, 110.0f);
            if (idx >= 0) enemies_force_chase(idx);
        }
        sound_play(SFX_BOSS_ENRAGE);
        hud_show_banner(T(S_BOSS_UP_T), T(S_BOSS_UP_S), 3.5f);
    }
}

// Lava e fendas de fogo queimam o druida (o proprio dano empurra ele para fora).
static void hazards_update(void) {
    if (g_druid.hp <= 0 || g_druid.state == DRUID_STATE_DODGE) return;   // rolar por cima da lava nao queima
    bool flame_up = ((get_ticks_ms() / 180) % 3) == 2;                       // mesmo ritmo da animacao da fenda
    for (int i = 0; i < s_hazard_count; i++) {
        const Hazard *h = &s_hazards[i];
        if (h->vent && !flame_up) continue;
        if (vec2_dist_sq(g_druid.x, g_druid.z, h->x, h->z) > h->r * h->r) continue;
        druid_take_damage(h->vent ? 14 : 9, h->x, h->z);
        break;
    }
}

bool world_boss_summoned(void) { return s_boss_summoned; }

int world_load_serial(void);

bool world_near_vendor(void) {
    return s_has_vendor && vec2_dist_sq(g_druid.x, g_druid.z, s_vendor_x, s_vendor_z) < 34.0f * 34.0f;
}

// Filtro vermelho de perigo sobre a cena do Altar (pulsa devagar).
void world_draw_screen_filter(void) {
    if (g_current_area != AREA_BLACK_ALTAR) return;
    float t = (float)get_ticks_ms() * 0.0015f;
    int alpha = 64 + (int)(12.0f * sinf(t)) + (s_boss_summoned ? 18 : 0);   // filtro vermelho por cima de tudo
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(g_campaign == 1 ? RGBA32(150, 96, 24, alpha / 2) : g_campaign == 2 ? RGBA32(40, 110, 190, alpha / 2) : RGBA32(170, 0, 6, alpha));   // norte: luz azul-gelo   // floresta: luz ambar suave de fim de tarde
    rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    rdpq_mode_blender(0);
}

// HUD dos selos: 3 marcadores abaixo do minimapa e barra de purificacao quando dentro do circulo.
static void draw_event_bars(void) {
    for (int i = 0; i < s_cev_count; i++) {     // vigilia do bau: barra dourada
        const ChestEv *e = &s_cev[i];
        if (e->type != CE_VIGIL || e->state != 1 || e->progress <= 0.0f) continue;
        int w = 150, bx = (SCREEN_WIDTH - w) / 2, by = 28;
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(12, 8, 2, 255));
        rdpq_fill_rectangle(bx - 1, by - 1, bx + w + 1, by + 7);
        rdpq_set_prim_color(RGBA32(230, 180, 60, 255));
        rdpq_fill_rectangle(bx, by, bx + (int)(w * (e->progress > 1 ? 1 : e->progress)), by + 6);
        return;
    }
    for (int i = 0; i < s_ev_count; i++) {
        const WEvent *e = &s_ev[i];
        if (!((e->type == EV_SHRINE && e->state == 1) || (e->type == EV_CAGE && e->state == 2)) || e->progress <= 0.0f) continue;
        int w = 150, bx = (SCREEN_WIDTH - w) / 2, by = 28;
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(6, 10, 4, 255));
        rdpq_fill_rectangle(bx - 1, by - 1, bx + w + 1, by + 7);
        rdpq_set_prim_color(e->type == EV_SHRINE ? RGBA32(90, 210, 70, 255) : RGBA32(200, 160, 60, 255));
        rdpq_fill_rectangle(bx, by, bx + (int)(w * (e->progress > 1 ? 1 : e->progress)), by + 6);
        break;
    }
}

void world_draw_objectives(void) {
    draw_event_bars();
    if (g_current_area != AREA_BLACK_ALTAR || s_boss_summoned || s_seal_count == 0) return;
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    // icones dos selos (Sprites/Vendedor/v3_selos.png -> tools/import_vendor.py); "off" = selo destruido
    static sprite_t *s_icon[2] = { NULL, NULL };
    if (!s_icon[0]) { s_icon[0] = sprite_load("rom:/hud_seal_on.rgba16.sprite");
                      s_icon[1] = sprite_load("rom:/hud_seal_off.rgba16.sprite"); }
    int x0 = SCREEN_WIDTH - 12 - s_seal_count * 18, y0 = 52;
    rdpq_set_mode_standard();
    rdpq_mode_alphacompare(1);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    for (int i = 0; i < s_seal_count; i++)
        rdpq_sprite_blit(s_icon[s_seals[i].done ? 1 : 0], x0 + i * 18, y0, NULL);
    rdpq_mode_blender(0);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    for (int i = 0; i < s_seal_count; i++) {
        const Seal *sl = &s_seals[i];
        if (sl->done || sl->progress <= 0.0f) continue;
        int w = 150, bx = (SCREEN_WIDTH - w) / 2, by = 28;
        rdpq_set_prim_color(RGBA32(10, 4, 4, 255));
        rdpq_fill_rectangle(bx - 1, by - 1, bx + w + 1, by + 7);
        rdpq_set_prim_color(RGBA32(220, 30, 20, 255));
        rdpq_fill_rectangle(bx, by, bx + (int)(w * (sl->progress > 1 ? 1 : sl->progress)), by + 6);
        break;
    }
}

void world_update(float dt) {
    (void)dt;
    AreaData *ad = &g_area_data[g_current_area];
    if (s_grid && s_grid_cols * s_grid_rowsN <= MAX_GRID_TILES) {   // revela o minimapa em volta do druida
        float pc = (g_druid.x + s_half_w) / TILE, pr = (s_half_h - g_druid.z) / TILE;
        const float R = 3.4f;
        int c0 = (int)(pc - R) , c1 = (int)(pc + R) + 1, r0 = (int)(pr - R), r1 = (int)(pr + R) + 1;
        if (c0 < 0) c0 = 0;
        if (r0 < 0) r0 = 0;
        if (c1 >= s_grid_cols) c1 = s_grid_cols - 1;
        if (r1 >= s_grid_rowsN) r1 = s_grid_rowsN - 1;
        for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) {
            float dx = (c + 0.5f) - pc, dy = (r + 0.5f) - pr;
            if (dx * dx + dy * dy <= R * R) s_explored[r * s_grid_cols + c] = 1;
        }
    }
    if (g_current_area == AREA_FORSAKEN_PATH || g_current_area == AREA_HOLLOW_CRYPT ||
        g_current_area == AREA_BLACK_ALTAR) forsaken_update();
    if (g_current_area == AREA_BLACK_ALTAR) { seals_update(dt); hazards_update(); }
    events_update(dt);
    chest_events_update(dt);

    // Check if exit should be unlocked
    if (!ad->exit_unlocked) {
        if (g_current_area == AREA_FORSAKEN_PATH || g_current_area == AREA_HOLLOW_CRYPT) {
            bool open_gate = (s_elites_total > 0) ? (enemies_count_elites() == 0 && elites_pending() == 0)
                                                   : (enemies_get_active_count() <= 2);
            if (open_gate) world_unlock_exit();
        }
    }
}

void world_unlock_exit(void) {
    AreaData *ad = &g_area_data[g_current_area];
    if (!ad->exit_unlocked) {
        ad->exit_unlocked = true;
        if (g_current_area == AREA_HOLLOW_CRYPT || g_current_area == AREA_FORSAKEN_PATH)   // arco de pedra com o vortice (pintado) no lugar do portal 3D
            add_scene_prop(SCENE_CAVE_PORTAL, ad->exit_x, ad->exit_z, 80.0f, 0.0f);
        sound_play(SFX_VICTORY);
        hud_show_banner(T(S_GATE_T), T(S_GATE_S), 2.5f);
        debugf("[WORLD] Area %d gate unlocked! Portal is open.\n", (int)g_current_area);
    }
}

bool world_check_exit(float x, float z) {
    AreaData *ad = &g_area_data[g_current_area];
    if (!ad->exit_unlocked) return false;

    float d2 = vec2_dist_sq(x, z, ad->exit_x, ad->exit_z);
    return (d2 <= (ad->exit_radius * ad->exit_radius));
}

static bool collide_one(const WorldCollider *c, float *x, float *z, float radius) {
    if (c->type == COL_CIRCLE) {
        float dx = *x - c->x;
        float dz = *z - c->z;
        float d2 = dx * dx + dz * dz;
        float min_d = radius + c->r;
        if (d2 < (min_d * min_d) && d2 > 0.0001f) {
            float dist = sqrtf(d2);
            float push = min_d - dist;
            *x += (dx / dist) * push;
            *z += (dz / dist) * push;
            return true;
        }
        return false;
    }
    // Caixa alinhada aos eixos (cercas): ponto mais proximo do retangulo.
    float nx = clampf(*x, c->x - c->hx, c->x + c->hx);
    float nz = clampf(*z, c->z - c->hz, c->z + c->hz);
    float dx = *x - nx, dz = *z - nz, d2 = dx * dx + dz * dz;
    if (d2 > 0.0001f && d2 < radius * radius) {
        float d = sqrtf(d2), push = radius - d;
        *x += dx / d * push;
        *z += dz / d * push;
        return true;
    }
    if (d2 <= 0.0001f) { // centro dentro da caixa: empurra para a borda mais proxima
        float pl = *x - (c->x - c->hx), pr = (c->x + c->hx) - *x;
        float pd = *z - (c->z - c->hz), pu = (c->z + c->hz) - *z;
        float m = pl; int ax = 0;
        if (pr < m) { m = pr; ax = 1; }
        if (pd < m) { m = pd; ax = 2; }
        if (pu < m) { m = pu; ax = 3; }
        if (ax == 0) *x = c->x - c->hx - radius;
        else if (ax == 1) *x = c->x + c->hx + radius;
        else if (ax == 2) *z = c->z - c->hz - radius;
        else *z = c->z + c->hz + radius;
        return true;
    }
    return false;
}

bool world_collide_circle(float *x, float *z, float radius) {
    AreaData *ad = &g_area_data[g_current_area];
    bool collided = false;

    // 1. World Bounds clamping
    if (*x < ad->min_x + radius) { *x = ad->min_x + radius; collided = true; }
    if (*x > ad->max_x - radius) { *x = ad->max_x - radius; collided = true; }
    if (*z < ad->min_z + radius) { *z = ad->min_z + radius; collided = true; }
    if (*z > ad->max_z - radius) { *z = ad->max_z - radius; collided = true; }

    // 1b. Colisao com paredes da grade (tiles '#')
    if (s_grid) {
        int cc=(int)floorf((*x + s_half_w)/TILE);
        int cr=(int)floorf((s_half_h - *z)/TILE);
        for (int dr=-1; dr<=1; dr++) for (int dc=-1; dc<=1; dc++){
            int tc=cc+dc, tr=cr+dr;
            if (tc<0||tr<0||tc>=s_grid_cols||tr>=s_grid_rowsN) continue;
            if (s_grid[tr][tc] != '#') continue;
            float tx0=tc*TILE - s_half_w, tx1=tx0+TILE;
            float tz1=s_half_h - tr*TILE, tz0=tz1-TILE;
            float nx=*x; if(nx<tx0)nx=tx0; if(nx>tx1)nx=tx1;
            float nz=*z; if(nz<tz0)nz=tz0; if(nz>tz1)nz=tz1;
            float dx=*x-nx, dz=*z-nz, d2=dx*dx+dz*dz;
            if (d2>0.0001f && d2<radius*radius){ float d=sqrtf(d2); float push=radius-d; *x+=dx/d*push; *z+=dz/d*push; collided=true; }
            else if (d2<=0.0001f){ float pl=*x-tx0, prr=tx1-*x, pdo=*z-tz0, pup=tz1-*z; float m=pl; int ax=0;
                if(prr<m){m=prr;ax=1;} if(pdo<m){m=pdo;ax=2;} if(pup<m){m=pup;ax=3;}
                if(ax==0)*x=tx0-radius; else if(ax==1)*x=tx1+radius; else if(ax==2)*z=tz0-radius; else *z=tz1+radius; collided=true; }
        }
    }

    // 2. Obstacles / Props colliders (somente os 3x3 tiles ao redor)
    if (s_grid && s_grid_cols * s_grid_rowsN <= MAX_GRID_TILES) {
        int cc = (int)floorf((*x + s_half_w) / TILE);
        int cr = (int)floorf((s_half_h - *z) / TILE);
        for (int dr = -1; dr <= 1; dr++) for (int dc = -1; dc <= 1; dc++) {
            int tc = cc + dc, tr = cr + dr;
            if (tc < 0 || tr < 0 || tc >= s_grid_cols || tr >= s_grid_rowsN) continue;
            for (int i = s_col_head[tr * s_grid_cols + tc]; i >= 0; i = s_col_next[i]) {
                if (collide_one(&s_colliders[i], x, z, radius)) collided = true;
            }
        }
    } else {
        for (int i = 0; i < s_collider_count; i++) {
            if (collide_one(&s_colliders[i], x, z, radius)) collided = true;
        }
    }

    return collided;
}

// ---------------------------------------------------------------------------
// Portal em 3D (estilo Diablo 2): vortice azul sempre virado para a camera, feito de 3 quads
// translucidos cuja textura gira (UV) em sentidos opostos. A geometria vive no mundo, entao
// acompanha a perspectiva do chao e das paredes (nao cresce nem encolhe em relacao a eles).
// ---------------------------------------------------------------------------
static T3DVertPacked *s_portal_verts = NULL;
static sprite_t *s_portal_tex = NULL;

// Quad no plano perpendicular a camera, em unidades de 1/16 do mundo. 'ang' gira so a textura.
static void portal_quad(int *vc, float hw, float hh, float cy, float ang, uint32_t col) {
    const float d = sqrtf(CAM_REF_HEIGHT * CAM_REF_HEIGHT + CAM_REF_BACK * CAM_REF_BACK);
    const float uy = CAM_REF_BACK / d, uz = CAM_REF_HEIGHT / d;   // "para cima" no plano da tela
    static const int SX[6] = {-1, 1, 1, -1, 1, -1}, SY[6] = {-1, -1, 1, -1, 1, 1};
    uint16_t n = t3d_vert_pack_normal(&(T3DVec3){{0, 1, 0}});
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i < 6; i++) {
        float px = SX[i] * hw, py = cy + SY[i] * hh;
        float lu = SX[i] * 16.0f, lv = -SY[i] * 16.0f;           // textura 32x32, centro em (16,16)
        float s = (16.0f + ca * lu - sa * lv) * 32.0f;            // S10.5
        float t = (16.0f + sa * lu + ca * lv) * 32.0f;
        qset(s_portal_verts, *vc + i, px * 16.0f, py * uy * 16.0f, py * uz * 16.0f, col, n,
             (int16_t)s, (int16_t)t);
    }
    *vc += 6;
}

static void draw_portal_3d(const AreaData *ad) {
    if (!s_portal_verts) s_portal_verts = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 9);
    if (!s_portal_tex) s_portal_tex = sprite_load("rom:/scene_portal_vortex.rgba32.sprite");
    if (!s_portal_verts || !s_portal_tex) return;
    float dx = ad->exit_x - g_druid.x, dz = ad->exit_z - g_druid.z;
    if (fabsf(dx) > 300.0f || dz < -200.0f || dz > 420.0f) return;

    float t = (float)get_ticks_ms() * 0.001f;
    float pulse = 0.82f + 0.18f * sinf(t * 5.0f);
    uint32_t core = ((uint32_t)(255.0f * pulse) << 24) | ((uint32_t)(255.0f * pulse) << 16) | (255u << 8) | 0xFFu;
    int vc = 0;
    portal_quad(&vc, 17.0f, 26.0f, 26.0f,  t * 1.2f,        0xB4DCFFFFu);
    portal_quad(&vc, 14.0f, 21.0f, 26.0f, -t * 2.0f + 1.0f, 0xFFFFFFFFu);
    portal_quad(&vc,  7.0f, 11.0f, 26.0f,  t * 3.1f,        core);

    // Sem neblina e com mistura por alpha so durante o portal; restaura em seguida.
    t3d_fog_set_enabled(false);
    rdpq_mode_fog(0);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    surface_t tex = sprite_get_pixels(s_portal_tex);
    rdpq_tex_upload(TILE0, &tex, NULL);
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_NO_LIGHT);

    T3DMat4FP m;
    float sc[3] = { 1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f };
    float rot[3] = { 0.0f, 0.0f, 0.0f };
    float pos[3] = { ad->exit_x, 0.0f, ad->exit_z };
    t3d_mat4fp_from_srt_euler(&m, sc, rot, pos);
    gfx_matrix_push(&m);
    t3d_vert_load(s_portal_verts, 0, 18);
    t3d_tri_draw_unindexed(0, 6);
    t3d_tri_sync();
    t3d_matrix_pop(1);

    rdpq_mode_blender(0);
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    if (g_dbg_fog) { rdpq_mode_fog(RDPQ_FOG_STANDARD); t3d_fog_set_enabled(true); }
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
}

void world_draw(void) {
    // 0. Caminho dos Condenados: chao em camadas (malha com variacao + trilha/terra/lajes/grama)
    bool cem_floor = ((g_current_area == AREA_FORSAKEN_PATH || g_current_area == AREA_HOLLOW_CRYPT) && s_cem_nxc > 0);
    if (cem_floor) draw_cemetery_floor();

    // 1. Piso: empilha matriz identidade UNCACHED antes de carregar vertices
    //    (sem isso o piso usa a matriz indefinida do ultimo prop).
    if (!cem_floor && s_floor_verts && s_floor_vert_count > 0) {
        bool textured = s_floor_textures[g_campaign * AREA_COUNT + g_current_area] != NULL;
        if (textured) {
            surface_t tex = sprite_get_pixels(s_floor_textures[g_campaign * AREA_COUNT + g_current_area]);
            rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
            rdpq_tex_upload(TILE0, &tex, &(rdpq_texparms_t){
                .s.repeats = REPEAT_INFINITE, .t.repeats = REPEAT_INFINITE
            });
            t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);
        }
        gfx_push_identity();
        uint32_t remaining = s_floor_vert_count;
        uint32_t offset = 0;
        while (remaining > 0) {
            uint32_t chunk = (remaining > 60) ? 60 : remaining;
            if (offset > 0) t3d_tri_sync();
            t3d_vert_load(&s_floor_verts[offset / 2], 0, chunk);
            t3d_tri_draw_unindexed(0, chunk / 3);
            offset += chunk;
            remaining -= chunk;
        }
        t3d_tri_sync();
        t3d_matrix_pop(1);
        if (textured) {
            rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
            t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
        }
    }

    // 2. Paredes 3D: malha vertical texturizada (mesmo pipeline do piso).
    if (s_wall_verts && s_wall_vert_count > 0 && s_wall_tex) {
        surface_t tex = sprite_get_pixels(s_wall_tex);
        rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
        rdpq_tex_upload(TILE0, &tex, &(rdpq_texparms_t){
            .s.repeats = REPEAT_INFINITE, .t.repeats = REPEAT_INFINITE
        });
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);
        gfx_push_identity();
        uint32_t remaining = s_wall_vert_count, offset = 0;
        while (remaining > 0) {
            uint32_t chunk = (remaining > 60) ? 60 : remaining;
            if (offset > 0) t3d_tri_sync();
            t3d_vert_load(&s_wall_verts[offset / 2], 0, chunk);
            t3d_tri_draw_unindexed(0, chunk / 3);
            offset += chunk;
            remaining -= chunk;
        }
        t3d_tri_sync();
        t3d_matrix_pop(1);
        rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
    }

    // 2b. Tampas de topo das paredes (textura de topo), para dar profundidade.
    if (s_wall_top_verts && s_wall_top_vert_count > 0 && s_wall_top_tex) {
        surface_t tex = sprite_get_pixels(s_wall_top_tex);
        rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
        rdpq_tex_upload(TILE0, &tex, &(rdpq_texparms_t){
            .s.repeats = REPEAT_INFINITE, .t.repeats = REPEAT_INFINITE
        });
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);
        gfx_push_identity();
        uint32_t remaining = s_wall_top_vert_count, offset = 0;
        while (remaining > 0) {
            uint32_t chunk = (remaining > 60) ? 60 : remaining;
            if (offset > 0) t3d_tri_sync();
            t3d_vert_load(&s_wall_top_verts[offset / 2], 0, chunk);
            t3d_tri_draw_unindexed(0, chunk / 3);
            offset += chunk;
            remaining -= chunk;
        }
        t3d_tri_sync();
        t3d_matrix_pop(1);
        rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
    }

    // 3. Pentagrama procedural: geometria vermelha deitada no chao (sem textura).
    if (s_penta_verts && s_penta_vert_count > 0) {
        rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH);
        gfx_push_identity();
        uint32_t remaining = s_penta_vert_count, offset = 0;
        while (remaining > 0) {
            uint32_t chunk = (remaining > 60) ? 60 : remaining;
            if (offset > 0) t3d_tri_sync();
            t3d_vert_load(&s_penta_verts[offset / 2], 0, chunk);
            t3d_tri_draw_unindexed(0, chunk / 3);
            offset += chunk;
            remaining -= chunk;
        }
        t3d_tri_sync();
        t3d_matrix_pop(1);
        t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH | gfx_cull_flag());
    }

    // 4. Portal de saida (3D, translucido): por ultimo na malha do mundo.
    if ((g_current_area == AREA_FORSAKEN_PATH || g_current_area == AREA_HOLLOW_CRYPT) && g_area_data[g_current_area].exit_unlocked)
        if (g_current_area != AREA_HOLLOW_CRYPT && g_current_area != AREA_FORSAKEN_PATH) draw_portal_3d(&g_area_data[g_current_area]);   // cripta: arco pintado
}
