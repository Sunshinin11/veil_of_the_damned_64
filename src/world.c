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
#include "cemetery_stage.h"

static bool authored_cemetery(void) {
    return g_campaign == 0 && g_current_area == AREA_FORSAKEN_PATH;
}
static bool authored_crypt(void) {
    return g_campaign == 0 && g_current_area == AREA_HOLLOW_CRYPT;
}
static bool authored_altar(void) {return g_campaign==0 && g_current_area==AREA_BLACK_ALTAR;}
static bool authored_forest(void) {return g_campaign==1 && g_current_area==AREA_FORSAKEN_PATH;}
static bool authored_swamp(void) {return g_campaign==1 && g_current_area==AREA_HOLLOW_CRYPT;}
static bool authored_heart(void) {return g_campaign==1 && g_current_area==AREA_BLACK_ALTAR;}
static bool authored_snow(void) {return g_campaign==2 && g_current_area==AREA_FORSAKEN_PATH;}
static bool authored_ice(void) {return g_campaign==2 && g_current_area==AREA_HOLLOW_CRYPT;}
static bool jarl_castle(void) {return g_campaign==2 && g_current_area==AREA_BLACK_ALTAR;}
static bool expansion_campaign(void) {return g_campaign>=3 && g_campaign<CAMPAIGN_COUNT;}
static bool reserved_boss_floor(int c,int r) {
    if(g_current_area!=AREA_BLACK_ALTAR)return false;
    if(g_campaign==1)return r<=18; // clearing and its two approaches stay free
    if(jarl_castle())return r<=23 || (c>=12 && c<=20);
    return authored_altar() && c>=9 && c<=16 && r>=3 && r<=10;
}
static bool modular_stage(void) {return authored_cemetery() || authored_crypt() || authored_altar() || authored_forest() || authored_swamp() || authored_heart() || authored_snow() || authored_ice() || jarl_castle() || expansion_campaign();}
static AltarPaint altar_paint_type(ScenePropId id) {
    switch(id) {
        case SCENE_HELL_DECAL_RITUAL:return ALT_PAINT_RITUAL;
        case SCENE_HELL_DECAL_BLOOD:case SCENE_DECAL_BLOOD:return ALT_PAINT_BLOOD;
        case SCENE_HELL_DECAL_BONES:case SCENE_DECAL_BONES:return ALT_PAINT_BONES;
        default:return ALT_PAINT_ASH;
    }
}

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
    if(expansion_campaign()) {
        bool desert=g_campaign==3;
        if(id==SCENE_CAVE_PORTAL || id==SCENE_HELL_PORTAL || id==SCENE_PORTAL_PATH || id==SCENE_PORTAL_CRYPT)
            return desert?SCENE_EX_DESERT_EXIT:SCENE_EX_VICT_EXIT;
        if(id==SCENE_HELL_SEAL_OFF)return desert?SCENE_EX_DESERT_STELA:
            (g_current_area==AREA_BLACK_ALTAR?SCENE_CHAPEL_CANDLES:SCENE_EX_VICT_GRAVES);
        return id;
    }
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
        case SCENE_HELL_SEAL_ON:      return SCENE_HEART_V2_TOTEM_ON;
        case SCENE_HELL_SEAL_OFF:     return SCENE_HEART_V2_TOTEM_OFF;
        case SCENE_HELL_ROCKS_A: case SCENE_HELL_ROCKS_B: case SCENE_HELL_ROCKS_C: case SCENE_HELL_ROCKS_D:
            return SCENE_SWAMP_SHROOM;
        case SCENE_HELL_SPIKES_A: case SCENE_HELL_SPIKES_B: case SCENE_HELL_SPIKES_C:
            return SCENE_FOREST_BONE_EFFIGY;
        case SCENE_HELL_FIREVENT: case SCENE_HELL_BRAZIER:return SCENE_SWAMP_SHROOM;
        case SCENE_HELL_HANGED:return SCENE_FOREST_PYRE_STAKE;
        case SCENE_HELL_SKULLS:return SCENE_FOREST_BONE_EFFIGY;
        case SCENE_HELL_STATUE:return SCENE_FOREST_WOOD_CAGE;
        case SCENE_HELL_THRONE:return SCENE_HEART_V2_TOTEM_ON;
        case SCENE_HELL_DECAL_BONES: case SCENE_HELL_DECAL_ASH:return SCENE_DECAL_BONES;
        case SCENE_HELL_DECAL_LAVA: case SCENE_HELL_DECAL_CRACK: case SCENE_HELL_DECAL_RITUAL:
        case SCENE_HELL_DECAL_BLOOD: case SCENE_HELL_LAVA_POOL:return SCENE_DECAL_CRACKS;
        default: return id;
    }
}

static bool heart_prop_fits(float x,float z,float height);
static void add_scene_prop(ScenePropId id, float x, float z, float height, float col_r) {
    if (s_scene_prop_count >= MAX_SCENE_PROPS) return;
    ScenePropId sk = scene_skin(id);
    id = sk;
    if(id==SCENE_FROST_STATUE)return; // Seated statue rejected; preserve the actual royal throne.
    if(authored_heart() && !prop_is_decal(id) && !heart_prop_fits(x,z,height))return;
    if (prop_is_decal(id) && !decal_fits(x, z, height)) return;
    if(authored_altar() && prop_is_decal(id)) {
        int handle=altar_stage_paint(altar_paint_type(id),x,z,height);
        if(handle>=0)s_scene_props[s_scene_prop_count++]=(SceneProp){id,x,0,z,height,handle+1};
        return;
    }
    if(modular_stage() && id>=SCENE_DECAL_BLOOD && id<=SCENE_DECAL_GRAVE) {
        static const CemeteryDecal terrain_decals[]={CEM_DECAL_BLOOD,CEM_DECAL_BONES,CEM_DECAL_CRACKS,CEM_DECAL_OPEN_GRAVE};
        CemeteryDecal type=terrain_decals[id-SCENE_DECAL_BLOOD];
        assertf(cemetery_stage_decal(type,x,z,height),"Cannot place cemetery ground painting");
        return;
    }
    float ground_y=prop_is_decal(id)?cemetery_stage_ground_y(x,z,height)+1:0;
    s_scene_props[s_scene_prop_count++] = (SceneProp){id, x, ground_y, z, height,0};
    if (col_r > 0.0f) add_circle_collider(x, z, col_r);
}

// Floor mesh
#define FLOOR_TILES_X 8
#define FLOOR_TILES_Z 10
static T3DVertPacked *s_floor_verts = NULL;
static int s_floor_vert_count = 0;
static int s_floor_chunk_cols = 0, s_floor_chunk_rows = 0;
static int s_floor_chunk_capacity = 0;

// ===== Mapas em grade (tiles) =====
#define TILE 72.0f
#define FLOOR_CHUNK_TILES 2
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
typedef struct { int16_t min[3], max[3]; } WorldAABB16;
static WorldAABB16 s_wall_bounds[MAX_WALL_FACES];
static WorldAABB16 s_wall_top_bounds[MAX_WALL_FACES];
static sprite_t *s_wall_textures[STAGE_COUNT];
static sprite_t *s_wall_top_textures[STAGE_COUNT];
static sprite_t *s_wall_tex = NULL;
static sprite_t *s_wall_top_tex = NULL;
// Pentagrama procedural (geometria vermelha), nitido em qualquer tamanho.
static T3DVertPacked *s_penta_verts = NULL;
static int s_penta_vert_count = 0;
// Mapa do Caminho dos Condenados: include/map_forsaken.h (gerado por tools/gen_forsaken_map.py)
#include "map_altar.h"   // gerado por tools/gen_altar_map.py
#include "map_heart.h"   // planta exclusiva da Mae-Raiz
#include "map_jarl.h"    // castelo do rei enlouquecido, campanha 3
#include "maps_expansion.h" // six original authored stages, campaigns 4 and 5

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
static float event_speed_factor(void);
float world_ground_grip(float x, float z) {
    if(authored_ice())return ice_stage_grip(x,z);
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
    if(authored_swamp())return swamp_stage_speed(x,z)*event_speed_factor();
    for (int i = 0; i < s_slow_count; i++)
        if (vec2_dist_sq(x, z, s_slow[i].x, s_slow[i].z) < s_slow[i].r * s_slow[i].r) return 0.6f*event_speed_factor();
    return event_speed_factor();
}
// Mapa 3: os 3 SELOS DE SANGUE. Ficar dentro do circulo purifica o selo (hordas atacam enquanto isso);
// com os 3 purificados, Malakor surge no centro do pentagrama.
#define SEAL_COUNT_MAX 3
#define EVENT_HOLD_RADIUS 144.0f // two terrain cells: room to fight and dodge
#define SEAL_RADIUS    EVENT_HOLD_RADIUS
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
typedef struct {float x,z;int index,pack,hp;bool done;} BossTarget;
static struct {BossTarget target[3];int count,done;float timer,wave,warning,bell_x,bell_z;bool ringing,ready;} s_boss_objective;
static void boss_objective_setup(void);
static void boss_objective_update(float dt);
static bool boss_objective_interact(void);
static bool boss_objective_damage(float x,float z,float radius,int damage);
static void boss_objective_defeated(int index);
static float s_vendor_x = 0.0f, s_vendor_z = 0.0f;   // mascate (posicao sorteada por partida)
static bool s_has_vendor = false;
static const char *const *s_grid=NULL;
static int s_grid_cols=0, s_grid_rowsN=0;
static uint8_t s_explored[MAX_GRID_TILES];   // minimapa: tiles ja vistos
static bool s_minimap_on = true;
static float s_half_w=0.0f, s_half_h=0.0f;
static bool tile_blocked(char c){return c=='#' || c=='^';}
static bool tile_walk(char c){ return !tile_blocked(c) && c!=' '; }

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
static void push_quad(T3DVertPacked *base, int *vc, float P[4][3], int16_t ST[4][2],
                      uint32_t col, uint16_t nrm);


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

static void store_aabb16(WorldAABB16 *out, float P[4][3]) {
    for (int axis = 0; axis < 3; axis++) {
        float lo = P[0][axis], hi = lo;
        for (int i = 1; i < 4; i++) {
            if (P[i][axis] < lo) lo = P[i][axis];
            if (P[i][axis] > hi) hi = P[i][axis];
        }
        out->min[axis] = (int16_t)floorf(lo);
        out->max[axis] = (int16_t)ceilf(hi);
    }
}

// Constroi as paredes como BLOCOS SOLIDOS, para terem profundidade (nao papel):
//  - faces laterais (s_wall_verts): um quad vertical por face de tile '#'
//    exposta a um tile caminhavel, com a textura de parede (64x32, cabe TMEM).
//  - tampas de topo (s_wall_top_verts): um quad horizontal no topo de cada tile
//    de parede exposto, com a textura de topo. E o topo visto de cima que da a
//    leitura de bloco macico na camera 3/4.


bool world_wall_tile(int col, int row) {
    if (!s_grid || col<0 || row<0 || col>=s_grid_cols || row>=s_grid_rowsN) return false;
    return s_grid[row][col]=='#'; // Water blocks movement, not the sprite sightline.
}
static bool decal_fits(float x, float z, float w) {
    if (!s_grid) return true;
    float hx = w * 0.5f, hz = w * 0.27f;          // decal 2:1 deitado
    for (int k = 0; k < 9; k++) {
        float px = x + hx * (float)(k % 3 - 1), pz = z + hz * (float)(k / 3 - 1);
        int c = (int)floorf((px + s_half_w) / TILE), r = (int)floorf((s_half_h - pz) / TILE);
        if (c < 0 || r < 0 || c >= s_grid_cols || r >= s_grid_rowsN || tile_blocked(s_grid[r][c])) return false;
    }
    return true;
}

static bool heart_prop_fits(float x,float z,float height) {
    // Reserve the visible base, not merely the sprite centre.
    float hx=fmaxf(12,height*.42f),hz=fmaxf(12,height*.20f);
    for(int k=0;k<9;k++) {
        float px=x+hx*(k%3-1),pz=z+hz*(k/3-1);
        int c=(int)floorf((px+s_half_w)/TILE),r=(int)floorf((s_half_h-pz)/TILE);
        if(c<0 || r<0 || c>=s_grid_cols || r>=s_grid_rowsN || tile_blocked(s_grid[r][c]) || s_grid[r][c]=='=')return false;
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
    return s_grid[r][c]=='^'?3:(s_grid[r][c]=='#'?2:1);
}
void world_minimap_toggle(void) { s_minimap_on = !s_minimap_on; }
static void draw_optional_minimap(int pcol,int prow,int x0,int y0);
void world_draw_minimap(void) {
    if (!s_minimap_on || !s_grid || s_grid_cols * s_grid_rowsN > MAX_GRID_TILES) return;
    const int n = 2 * MM_HALF + 1, size = n * MM_PX;
    const Enemy *boss=enemies_get_boss();
    const int x0 = SCREEN_WIDTH - 16 - size, y0 = (boss && boss->hp>0)?68:12;
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
                rdpq_set_prim_color(k==3?RGBA32(39,95,77,230):k == 2 ? RGBA32(150, 140, 125, 230) : RGBA32(60, 56, 68, 205));
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
    draw_optional_minimap(pcol,prow,x0,y0);
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
static int spawn_event_member(EnemyType type,float x,float z,int pack,int variant,float lo,float hi) {
    for(int k=0;k<32;k++) {
        float a=frand()*6.2831853f,r=frange(lo,hi),px=x+cosf(a)*r,pz=z+sinf(a)*r;
        if(world_spot_free(px,pz,14))return enemies_spawn_variant(type,px,pz,pack,variant);
    }
    return -1;
}
static int spawn_miniboss_member(float x,float z,int pack) {
    int origin=rand()%5,phase=1+rand()%2;
    for(int k=0;k<32;k++) {
        float a=frand()*6.2831853f,r=frange(35,100),px=x+cosf(a)*r,pz=z+sinf(a)*r;
        if(world_spot_free(px,pz,20))return enemies_spawn_miniboss(origin,phase,px,pz,pack);
    }
    return -1;
}

static bool on_trail(float x, float z, float margin);
static void events_setup(void);
static void optional_events_setup(void);
static float event_speed_factor(void);
static int s_cev_count = 0;
static bool clear_of_trail(float *x, float *z, float r);
static bool s_cem_reserved[MG_ROWS*MG_COLS];

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
        case 'T': case 'X':
            if (authored_cemetery()) {
                if (s_collider_count<MAX_COLLIDERS && cemetery_stage_place(ch=='T'?CEM_GRAVE:CEM_CROSS,wx,wz,0))
                    add_circle_collider(wx,wz,10.0f);
            } else add_scene_prop(ch=='T'?SCENE_GRAVESTONE:SCENE_GRAVE_CROSS,
                wx+frange(-5,5),wz+frange(-4,4),ch=='T'?20.0f:22.0f,5.0f);
            return true;
        case 'U': add_scene_prop(SCENE_TOMB_STATUE, wx, wz, 34.0f, 8.0f); return true;
        // 'u': estatua na frente de um trecho de cerca (fecha o tile inteiro, sem bloco invisivel)
        case 'u': if (authored_cemetery()) {
                      if (s_collider_count<MAX_COLLIDERS && cemetery_stage_place(CEM_FENCE,wx,wz,0))
                          add_box_collider(wx,wz,36,6);
                  } else {
                      add_scene_prop(SCENE_GRAVE_FENCE, wx, wz, 44.0f, 0.0f);
                      add_box_collider(wx, wz, TILE * 0.5f + 1.0f, 8.0f);
                  }
                  add_scene_prop(SCENE_TOMB_STATUE, wx, wz - 9.0f, 34.0f, 0.0f); return true;
        case 'G': add_scene_prop(SCENE_GREAT_TREE, wx, wz, 165.0f, 16.0f); return true;
        case 'R': add_scene_prop(SCENE_TREE_DEAD, wx + frange(-14, 14), wz + frange(-12, 12), frange(76, 92), 10.0f); return true;
        case 'K': add_scene_prop(SCENE_DEAD_BUSH, wx + frange(-16, 16), wz + frange(-14, 14), frange(22, 30), 0.0f); return true;
        case 'J': if (authored_cemetery()) {
                      if (s_collider_count<MAX_COLLIDERS && cemetery_stage_place(CEM_FENCE,wx,wz,0))
                          add_box_collider(wx,wz,36,6);
                  } else {
                      add_scene_prop(SCENE_GRAVE_FENCE, wx, wz, 44.0f, 0.0f);
                      add_box_collider(wx,wz,TILE*0.5f+1.0f,8.0f);
                  }
                  return true;
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

static int s_forest_foliage;
static uint8_t s_forest_foliage_room[MG_CH][MG_CW];
static void forest_undergrowth(float x,float z,int count) {
    static const float offsets[3][2]={{-24,-12},{23,-17},{8,25}};
    int mx=(int)floorf((x+s_half_w)/TILE)/MG_STEP,my=(int)floorf((s_half_h-z)/TILE)/MG_STEP;
    if(mx<0 || mx>=MG_CW || my<0 || my>=MG_CH)return;
    for(int i=0;i<count && i<3 && s_forest_foliage<120 && s_forest_foliage_room[my][mx]<8;i++) {
        float px=x+offsets[i][0],pz=z+offsets[i][1];
        if(on_trail(px,pz,12) || !world_spot_free(px,pz,4))continue;
        int before=s_scene_prop_count;
        bool live=mx==MG_CW-1 && my<2 && i==0;
        ScenePropId plant=live?SCENE_FOREST_FERN:((i==2)?SCENE_DEAD_BUSH:SCENE_FOREST_DRYGRASS);
        add_scene_prop(plant,px,pz,live?frange(17,22):frange(12,20),0);
        s_forest_foliage+=s_scene_prop_count-before;
        s_forest_foliage_room[my][mx]+=s_scene_prop_count-before;
    }
}
static void forest_path_fences(void) {
    if(!authored_forest())return;
    const FloorQuad *floor;int nf=mapgen_floor(&floor),total=0;
    uint8_t per_room[MG_CH][MG_CW]={0};
    for(int i=0;i<nf;i++) {
        const FloorQuad *f=&floor[i];
        if(f->kind!=1 || i%3!=0)continue;
        bool fan=false;
        for(int j=0;j<nf;j++)if(i!=j && floor[j].kind==1 && floor[j].x[0]==f->x[0] && floor[j].z[0]==f->z[0]){fan=true;break;}
        if(fan)continue;
        float sx=(f->x[0]+f->x[1])*.5f,sz=(f->z[0]+f->z[1])*.5f;
        float ex=(f->x[2]+f->x[5])*.5f,ez=(f->z[2]+f->z[5])*.5f;
        float dx=ex-sx,dz=ez-sz,len=sqrtf(dx*dx+dz*dz);
        if(len<1)continue;
        dx/=len;dz/=len;
        float half_width=hypotf(f->x[1]-f->x[0],f->z[1]-f->z[0])*.5f;
        float offset=half_width*.68f+17;
        for(int side=-1;side<=1;side+=2) {
            float x=(sx+ex)*.5f-dz*side*offset,z=(sz+ez)*.5f+dx*side*offset;
            int c=(int)floorf((x+s_half_w)/TILE),r=(int)floorf((s_half_h-z)/TILE);
            if(c<1 || r<1 || c>=s_grid_cols-1 || r>=s_grid_rowsN-1)continue;
            int mx=c/MG_STEP,my=r/MG_STEP;
            if(mx>=MG_CW || my>=MG_CH || per_room[my][mx]>=6)continue;
            bool free=true;
            for(int k=-1;k<=1;k++) {
                float px=x+dx*k*28,pz=z+dz*k*28;
                if(!world_spot_free(px,pz,6) || !forest_stage_spot_free(px,pz,9)){free=false;break;}
            }
            if(!free)continue;
            for(int rr=1;rr<s_grid_rowsN-1 && free;rr++)for(int cc=1;cc<s_grid_cols-1;cc++) {
                char marker=s_grid[rr][cc];
                if(marker!='E' && marker!='P' && marker!='C')continue;
                float px=(cc+.5f)*TILE-s_half_w,pz=s_half_h-(rr+.5f)*TILE;
                float clearance=marker=='C'?95:140;
                if(vec2_dist_sq(x,z,px,pz)<clearance*clearance){free=false;break;}
            }
            if(!free || s_collider_count>=MAX_COLLIDERS)continue;
            float yaw=atan2f(dz,dx);
            if(forest_stage_place(FOREST_PROP_BROKEN_FENCE,x,z,yaw,1)) {
                add_box_collider(x,z,fabsf(dx)*29+fabsf(dz)*5,fabsf(dz)*29+fabsf(dx)*5);
                per_room[my][mx]++;total++;
            }
        }
    }
    debugf("[FOREST] Placed %d visible trail-side fence fragments\n",total);
}
static bool forest_place(AreaData *ad,char ch,float x,float z) {
    float yaw=frange(0,6.2831853f);
    switch(ch) {
        case 'R':case 'G': {
            float scale=ch=='G'?1.35f:frange(.78f,1.02f),radius=22*scale;
            if(!clear_of_trail(&x,&z,radius+10) || !world_spot_free(x,z,radius+5) ||
               !forest_stage_spot_free(x,z,radius+8) || s_collider_count>=MAX_COLLIDERS)return true;
            if(forest_stage_place((ForestProp)(rand()%3),x,z,yaw,scale)) {
                add_circle_collider(x,z,radius);
                forest_undergrowth(x,z,ch=='G'?3:2);
            }
            return true;
        }
        case 'V':case 'W':case 'B':case 'I':case 'M': {
            bool log=ch=='V' || ch=='W';
            if(!clear_of_trail(&x,&z,30) || !world_spot_free(x,z,28) ||
               !forest_stage_spot_free(x,z,32) || s_collider_count>=MAX_COLLIDERS)return true;
            if(forest_stage_place(log?FOREST_PROP_LOG:FOREST_PROP_STONE,x,z,yaw,1)) {
                // A circle conservatively encloses the rotated log or stone.
                add_circle_collider(x,z,log?29:23);
            }
            return true;
        }
        case 'K':
            if(!on_trail(x,z,14))forest_undergrowth(x,z,2);
            return true;
        case 'H':
            if(clear_of_trail(&x,&z,12))add_scene_prop(pick3(SCENE_CAVE_MUSH_A,3),x,z,frange(14,20),0);
            return true;
        case 'U':case 'p':
            if(clear_of_trail(&x,&z,23) && world_spot_free(x,z,20) && forest_stage_spot_free(x,z,30))add_scene_prop(SCENE_PATH_SHRINE,x,z,44,14);
            return true;
        case 'T':case 'X':case 'J':case 'u':case 'o':
            // No cemetery fences, tombs or open graves in the woodland.
            if(world_spot_free(x,z,18) && forest_stage_spot_free(x,z,25) && !on_trail(x,z,24) && s_collider_count<MAX_COLLIDERS &&
               forest_stage_place(FOREST_PROP_STUMP,x,z,yaw,1))add_circle_collider(x,z,15);
            return true;
        default:return forsaken_place(ad,ch,x,z);
    }
}

static void forest_ritual_sites(void) {
    if(!authored_forest())return;
    AreaData *ad=&g_area_data[g_current_area];
    int ritual_sites=0;
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;
        if(!mapgen_visual_cell(mx,my,&cell))continue;
        // Every clearing can receive a ritual prop; occasional larger groups
        // receive a second piece. Keep the center and through-path reserved.
        int room_sites=0,room_goal=1+((mx+my*MG_CW)%4==0);
        if(ritual_sites<24)for(int j=0;j<20 && room_sites<room_goal;j++) {
            int k=(j+cell.index*3)%20;
            int pc=k<6?k:k<11?5:k<16?15-k:0;
            int pr=k<6?0:k<11?k-5:k<16?5:20-k;
            int c=mx*MG_STEP+1+pc,r=my*MG_STEP+1+pr;
            if(!strchr(".KRHV",s_grid[r][c]))continue;
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            if(on_trail(x,z,34) || !world_spot_free(x,z,38) || !forest_stage_spot_free(x,z,38) ||
               vec2_dist_sq(x,z,ad->spawn_x,ad->spawn_z)<130*130 ||
               vec2_dist_sq(x,z,ad->exit_x,ad->exit_z)<105*105)continue;
            bool occupied=false;
            const Chest *chests=items_get_chests();
            for(int n=0;n<MAX_CHESTS;n++)if(chests[n].active && vec2_dist_sq(x,z,chests[n].x,chests[n].z)<90*90){occupied=true;break;}
            // Chests/vendor are instantiated later. Reserve all their raw
            // marker positions as well during this early scenery pass.
            for(int rr=1;rr<s_grid_rowsN-1 && !occupied;rr++)for(int cc=1;cc<s_grid_cols-1;cc++) {
                if(s_grid[rr][cc]!='C')continue;
                float cx=(cc+.5f)*TILE-s_half_w,cz=s_half_h-(rr+.5f)*TILE;
                if(vec2_dist_sq(x,z,cx,cz)<100*100){occupied=true;break;}
            }
            if(occupied || (s_has_vendor && vec2_dist_sq(x,z,s_vendor_x,s_vendor_z)<100*100) ||
               s_scene_prop_count>=MAX_SCENE_PROPS || s_collider_count>=MAX_COLLIDERS)continue;
            for(int n=0;n<s_scene_prop_count;n++) {
                const SceneProp *p=&s_scene_props[n];
                if(p->id>=SCENE_FOREST_WOOD_CAGE && p->id<=SCENE_FOREST_BONE_EFFIGY &&
                   vec2_dist_sq(x,z,p->x,p->z)<90*90){occupied=true;break;}
            }
            if(occupied)continue;
            int kind=ritual_sites%3;
            static const ScenePropId props[3]={SCENE_FOREST_WOOD_CAGE,SCENE_FOREST_PYRE_STAKE,SCENE_FOREST_BONE_EFFIGY};
            static const float heights[3]={48,72,60},radii[3]={23,17,16};
            add_scene_prop(props[kind],x,z,heights[kind],radii[kind]);
            if(decal_fits(x+20,z-20,30))add_scene_prop(SCENE_DECAL_BONES,x+20,z-20,30,0);
            forest_undergrowth(x,z,2);
            ritual_sites++;
            room_sites++;
        }
    }
    debugf("[FOREST] Reserved %d primitive ritual sites before trees\n",ritual_sites);
}
static void forest_clearings(void) {
    if(!authored_forest())return;
    static const int slots[8][2]={{0,1},{5,4},{0,4},{5,1},{1,0},{4,5},{4,0},{1,5}};
    AreaData *ad=&g_area_data[g_current_area];
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;
        if(!mapgen_visual_cell(mx,my,&cell))continue;
        int made=0;
        for(int j=0;j<8 && made<2;j++) {
            int k=(j+cell.index*3)%8,c=mx*MG_STEP+1+slots[k][0],r=my*MG_STEP+1+slots[k][1];
            if(s_grid[r][c]!='.')continue;
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            if(on_trail(x,z,32) || !world_spot_free(x,z,32) || !forest_stage_spot_free(x,z,34) ||
               vec2_dist_sq(x,z,ad->spawn_x,ad->spawn_z)<120*120 ||
               vec2_dist_sq(x,z,ad->exit_x,ad->exit_z)<100*100)continue;
            bool occupied=false;
            const Chest *chests=items_get_chests();
            for(int n=0;n<MAX_CHESTS;n++)if(chests[n].active && vec2_dist_sq(x,z,chests[n].x,chests[n].z)<80*80){occupied=true;break;}
            if(occupied || (s_has_vendor && vec2_dist_sq(x,z,s_vendor_x,s_vendor_z)<90*90) || s_collider_count>=MAX_COLLIDERS)continue;
            bool log=((cell.index+mx+my+made)%3)==0;
            bool fence=!log && ((cell.index+mx+made)%3)!=1;
            if(forest_stage_place(log?FOREST_PROP_LOG:fence?FOREST_PROP_BROKEN_FENCE:FOREST_PROP_STUMP,x,z,frange(0,6.2831853f),1)) {
                add_circle_collider(x,z,(log || fence)?29:15);
                forest_undergrowth(x,z,3);
                add_scene_prop(pick3(SCENE_CAVE_MUSH_A,3),x-19,z-16,14,0);
                if((cell.index+mx+my)%4==0)add_scene_prop(SCENE_DECAL_BONES,x,z-27,30,0);
                if((cell.index+mx+my)%3==0 && decal_fits(x-17,z+18,44))
                    forest_stage_web(x-17,z+18,44);
                made++;
            }
        }
    }
}

static bool crypt_tomb(float x,float z,bool opened) {
    if(s_collider_count>=MAX_COLLIDERS || s_scene_prop_count>=MAX_SCENE_PROPS)return false;
    // Reuse the original closed/open painted tombs, including the displaced lid.
    add_scene_prop(opened?SCENE_CAVE_SARCO_B:SCENE_CAVE_SARCO_A,x,z,34,0);
    add_box_collider(x,z,26,18);
    return true;
}

static void crypt_burial_groups(void) {
    if(!authored_crypt())return;
    // Perimeter pairs leave both central rows/columns and every doorway clear.
    static const int slots[8][2]={{0,1},{0,4},{5,1},{5,4},{1,0},{4,0},{1,5},{4,5}};
    int total=0;
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;
        if(!mapgen_visual_cell(mx,my,&cell))continue;
        int made=0,goal=4+(cell.index&1);
        for(int j=0;j<8 && made<goal;j++) {
            int k=(j+(cell.index%4)*2)%8;
            int c=mx*MG_STEP+1+slots[k][0],r=my*MG_STEP+1+slots[k][1];
            if(r>=s_grid_rowsN || c>=s_grid_cols || !strchr(".HVBWIMko",s_grid[r][c]))continue;
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            AreaData *ad=&g_area_data[g_current_area];
            if(vec2_dist_sq(x,z,ad->spawn_x,ad->spawn_z)<100*100 ||
               vec2_dist_sq(x,z,ad->exit_x,ad->exit_z)<100*100 || !world_spot_free(x,z,30))continue;
            bool near_chest=false;
            const Chest *ch=items_get_chests();
            for(int n=0;n<MAX_CHESTS;n++)if(ch[n].active && vec2_dist_sq(x,z,ch[n].x,ch[n].z)<64*64) {near_chest=true;break;}
            if(near_chest || (s_has_vendor && vec2_dist_sq(x,z,s_vendor_x,s_vendor_z)<72*72))continue;
            if(crypt_tomb(x,z,((mx+my+k+cell.index)%3)==0)){made++;total++;}
        }
    }
    debugf("[CRYPT] Added %d painted tombs in burial groups; open/closed variants\n",total);
}

static void crypt_ruins(void) {
    if(!authored_crypt())return;
    static const int slots[12][2]={{0,0},{5,5},{5,0},{0,5},{0,1},{5,4},{0,4},{5,1},{1,0},{4,5},{4,0},{1,5}};
    int total=0;
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;
        if(!mapgen_visual_cell(mx,my,&cell))continue;
        int made=0,goal=1+(cell.index&1);
        for(int j=0;j<12 && made<goal;j++) {
            int k=(j+cell.index*3+mx+my)%12;
            int c=mx*MG_STEP+1+slots[k][0],r=my*MG_STEP+1+slots[k][1];
            if(r>=s_grid_rowsN || c>=s_grid_cols || !strchr(".HVBWIMko",s_grid[r][c]))continue;
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            AreaData *ad=&g_area_data[g_current_area];
            if(vec2_dist_sq(x,z,ad->spawn_x,ad->spawn_z)<100*100 ||
               vec2_dist_sq(x,z,ad->exit_x,ad->exit_z)<100*100 || !world_spot_free(x,z,35))continue;
            bool occupied=false;
            const Chest *ch=items_get_chests();
            for(int n=0;n<MAX_CHESTS;n++)if(ch[n].active && vec2_dist_sq(x,z,ch[n].x,ch[n].z)<72*72) {occupied=true;break;}
            if(occupied || (s_has_vendor && vec2_dist_sq(x,z,s_vendor_x,s_vendor_z)<80*80))continue;
            if(s_collider_count>=MAX_COLLIDERS)return;
            int variant=(cell.index+k+mx)%3;bool turn=((k+my)&1)!=0;
            if(crypt_stage_place((CryptProp)(CRYPT_PROP_RUBBLE_A+variant),x,z,turn?1.5707963268f:0)) {
                add_box_collider(x,z,turn?20:28,turn?28:20);made++;total++;
            }
        }
    }
    debugf("[CRYPT] Placed %d 3D ruin groups after tombs/events\n",total);
}

static bool crypt_place(AreaData *ad, char ch, float wx, float wz) {
    if(authored_crypt()) {
        switch(ch) {
            case 'L':
                if(s_collider_count<MAX_COLLIDERS && crypt_stage_place(CRYPT_PROP_PILLAR,wx,wz,0))add_box_collider(wx,wz,12,12);
                return true;
            case 'Q': case 'T': case 'X':
                crypt_tomb(wx,wz,((int)(wx/TILE)+(int)(wz/TILE))%3==0);
                return true;
            case 'A':
                if(s_collider_count<MAX_COLLIDERS && crypt_stage_place(CRYPT_PROP_ALTAR,wx,wz,0))add_box_collider(wx,wz,24,17);
                return true;
            case 'F':add_scene_prop(SCENE_CRYPT_BRAZIER,wx,wz,34,8);return true;
            case 'U':add_scene_prop(SCENE_TOMB_STATUE,wx,wz,40,8);return true;
            case '~':case 'Y':case 'b':add_scene_prop(SCENE_DECAL_BONES,wx,wz,40,0);return true;
            case 'v':add_scene_prop(SCENE_DECAL_BLOOD,wx,wz,48,0);return true;
            case 'k':case 'o':case 'V':case 'W':case 'B':case 'I':case 'M':
                add_scene_prop(SCENE_DECAL_CRACKS,wx,wz,44,0);return true;
            case 'H':case 'G':case 'R':case 'K':return true;
            case 'E':
                ad->spawn_x=wx;ad->spawn_z=wz;ad->spawn_yaw=1.5707963f;
                return true;
            case 'P':
                ad->exit_x=wx;ad->exit_z=wz;ad->exit_radius=40;
                if(s_collider_count<MAX_COLLIDERS && crypt_stage_place(CRYPT_PROP_ALTAR,wx,wz+TILE*1.25f,0))
                    add_box_collider(wx,wz+TILE*1.25f,24,17);
                return true;
            default:return forsaken_place(ad,ch,wx,wz);
        }
    }
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

// Authored execution grounds around Malakor. Coordinates use grid cell centers;
// tall silhouettes stay by the rails or behind the throne, outside the fight.
static void altar_battle_dressing(void) {
    if(!authored_altar())return;
    static const struct {
        ScenePropId id;
        float col,row,height,radius;
    } props[]={
        {SCENE_HELL_STATUE,     9.4f, 1.7f,64,12},
        {SCENE_HELL_STATUE,    16.0f, 1.7f,64,12},
        {SCENE_HELL_BRAZIER,   10.3f, 1.6f,42, 8},
        {SCENE_HELL_BRAZIER,   15.1f, 1.6f,42, 8},
        {SCENE_HELL_HANGED,     8.1f, 2.7f,76,10},
        {SCENE_HELL_HANGED,    17.0f, 8.6f,76,10},
        {SCENE_HELL_SKULLS,    17.0f, 2.7f,52, 5},
        {SCENE_HELL_SKULLS,     8.1f, 6.2f,52, 5},
        {SCENE_HELL_SKULLS,     8.1f,10.9f,46, 5},
        {SCENE_HELL_SKULLS,    17.0f,11.1f,46, 5},
        {SCENE_CULTIST_BODY,    8.6f, 3.1f,15, 0},
        {SCENE_TRAVELER_BODY,  16.5f, 3.5f,15, 0},
        {SCENE_CULTIST_BODY,   16.5f, 7.2f,16, 0},
        {SCENE_TRAVELER_BODY,   8.5f, 9.2f,14, 0},
        {SCENE_CULTIST_BODY,   14.1f, 1.6f,14, 0},
        {SCENE_BONE_PILE,       8.5f, 5.0f,24, 0},
        {SCENE_BONE_PILE,      16.5f, 5.0f,24, 0},
        {SCENE_BONE_PILE,       8.6f, 7.1f,22, 0},
        {SCENE_BONE_PILE,      16.5f,10.1f,22, 0},
        {SCENE_BONE_PILE,      10.7f, 1.0f,22, 0},
        {SCENE_BONE_PILE,      14.7f, 2.3f,20, 0},
        {SCENE_HELL_DECAL_BONES, 8.6f, 2.0f,42, 0},
        {SCENE_HELL_DECAL_BLOOD, 8.6f, 3.6f,44, 0},
        {SCENE_HELL_DECAL_BONES,16.5f, 6.6f,42, 0},
        {SCENE_HELL_DECAL_BLOOD,16.5f, 7.7f,46, 0},
        {SCENE_HELL_DECAL_BONES, 8.6f, 9.7f,42, 0},
        {SCENE_HELL_DECAL_BLOOD,13.9f, 1.0f,40, 0},
        // Low remnants inside the fight: no blockers or tall silhouettes.
        {SCENE_CULTIST_BODY,   10.2f, 4.6f,14, 0},
        {SCENE_TRAVELER_BODY,  14.7f, 8.7f,14, 0},
        {SCENE_BONE_PILE,      11.0f, 9.5f,18, 0},
        {SCENE_BONE_PILE,      15.0f, 5.5f,18, 0},
        {SCENE_HELL_DECAL_BLOOD,10.2f, 4.9f,38, 0},
        {SCENE_HELL_DECAL_BLOOD,14.7f, 9.0f,36, 0},
        {SCENE_HELL_DECAL_BONES,13.6f, 3.9f,34, 0},
    };
    int added=0;
    for(unsigned i=0;i<sizeof(props)/sizeof(props[0]);i++) {
        float x=(props[i].col+.5f)*TILE-s_half_w;
        float z=s_half_h-(props[i].row+.5f)*TILE;
        if(!world_spot_free(x,z,fmaxf(props[i].radius,12)))continue;
        bool decal=prop_is_decal(props[i].id),occupied=false;
        if(!decal)for(int j=0;j<s_scene_prop_count;j++) {
            const SceneProp *p=&s_scene_props[j];
            if(!p->ground_handle && !prop_is_decal(p->id) &&
               vec2_dist_sq(x,z,p->x,p->z)<24*24) {occupied=true;break;}
        }
        if(occupied)continue;
        int before=s_scene_prop_count;
        add_scene_prop(props[i].id,x,z,props[i].height,props[i].radius);
        added+=s_scene_prop_count-before;
    }
    debugf("[ALTAR] Added %d perimeter execution-ground details\n",added);
}

static bool altar_place(AreaData *ad, char ch, float wx, float wz) {
    if(jarl_castle()) {
        switch(ch) {
            case 'Q':add_scene_prop(SCENE_ICE_FROZEN_WARRIOR,wx,wz,58,9);return true;
            case 'p':add_circle_collider(wx,wz,32);return true; // mesh belongs to castle kit
            case 't':add_box_collider(wx,wz,60,34);return true;
            case 'n':return true; // Outdoor standards do not belong inside the castle.
            case 'u':return true; // Retired seated statue; no invisible collider.
            case 'g':add_scene_prop(SCENE_FROST_CAGE,wx,wz,60,10);return true;
            case 'h':add_scene_prop(SCENE_FROST_HANGED,wx,wz,60,10);return true;
            case 'H':add_scene_prop(SCENE_FROST_THRONE,wx,wz,104,0);add_box_collider(wx,wz,30,18);return true;
            case 'F':add_scene_prop(SCENE_FROST_BRAZIER,wx,wz,40,9);return true;
            default:break;
        }
    }
    if(authored_altar()) {
        int c=(int)((wx+s_half_w)/TILE),r=(int)((s_half_h-wz)/TILE);
        bool combat=r>=3 && r<=10 && c>=9 && c<=16;
        if(combat && strchr("Fghx",ch))wx+=(c<13?-TILE:TILE);
        switch(ch) {
            case 'M':
                if(s_collider_count<MAX_COLLIDERS && altar_stage_place(ALT_PROP_PYLON,wx,wz,0))add_box_collider(wx,wz,14,14);
                return true;
            case 'H':
                if(s_collider_count<MAX_COLLIDERS && altar_stage_place(ALT_PROP_THRONE,wx,wz,3.14159265f))add_box_collider(wx,wz,35,26);
                return true;
            case 'g':
                if(s_collider_count<MAX_COLLIDERS && altar_stage_place(ALT_PROP_CAGE,wx,wz,0)) {
                    add_box_collider(wx,wz,15,13);
                    add_scene_prop(SCENE_CULTIST_BODY,wx,wz,12,0);
                }
                return true;
            case 'L':return true; // lava now occupies the abyss outside the platform
            case 'f':
                if(r>=27 || (r>=13 && r<=18))wx-=TILE;
                add_scene_prop(SCENE_HELL_FIREVENT,wx,wz,40,0);add_hazard(wx,wz,16,true);return true;
            case 'F':add_scene_prop(SCENE_HELL_BRAZIER,wx,wz,40,8);return true;
            case 'h':add_scene_prop(SCENE_HELL_HANGED,wx,wz,70,10);return true;
            case 'x':add_scene_prop(SCENE_HELL_SKULLS,wx,wz,48,5);return true;
            default:break;
        }
    }
    switch (ch) {
        case '*':
            if(g_campaign==2)return true;
            if (s_seal_count < SEAL_COUNT_MAX) {
                Seal *sl = &s_seals[s_seal_count++];
                *sl = (Seal){wx, wz, 0.0f, 0.0f, false, false, -1, -1};
                int n0 = s_scene_prop_count;
                if(g_campaign==0)add_scene_prop(SCENE_HELL_DECAL_RITUAL, wx, wz, 104.0f, 0.0f);
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
    if(authored_snow()) {
        switch(ch) {
            case 'R':case 'G':
                if(clear_of_trail(&wx,&wz,34) && world_spot_free(wx,wz,27) && forest_stage_spot_free(wx,wz,32))add_scene_prop(pick3(SCENE_SNOW_PINE_A,3),wx,wz,frange(84,100),21);
                return true;
            case 'J':
                if(clear_of_trail(&wx,&wz,32) && world_spot_free(wx,wz,27) && forest_stage_spot_free(wx,wz,32))add_scene_prop(SCENE_SNOW_PALISADE,wx,wz,44,24);
                return true;
            case 'V': {
                float radius=24;
                if(!clear_of_trail(&wx,&wz,radius+10) || !world_spot_free(wx,wz,radius+4) || !forest_stage_spot_free(wx,wz,radius+8) || s_collider_count>=MAX_COLLIDERS)return true;
                SnowProp prop=(SnowProp)(rand()%3);
                float yaw=frange(0,6.2831853f);
                if(snow_stage_place(prop,wx,wz,yaw,1))add_circle_collider(wx,wz,radius);
                return true;
            }
            case 'K':
                if(!on_trail(wx,wz,12) && world_spot_free(wx,wz,13) && forest_stage_spot_free(wx,wz,15))add_scene_prop(SCENE_SNOW_V1_SHRUB,wx,wz,frange(18,27),0);
                return true;
            case 'B':
                if(clear_of_trail(&wx,&wz,38) && world_spot_free(wx,wz,34) && forest_stage_spot_free(wx,wz,38)) {
                    add_scene_prop(SCENE_SNOW_V1_CART,wx,wz,35,0);add_box_collider(wx,wz,28,17);
                }
                return true;
            case 'U':case 'N':
                if(clear_of_trail(&wx,&wz,25) && world_spot_free(wx,wz,22) && forest_stage_spot_free(wx,wz,26))add_scene_prop(ch=='U'?SCENE_SNOW_RUNESTONE:SCENE_SNOW_BANNER,wx,wz,ch=='U'?46:54,9);
                return true;
            case 'T':case 'X':case 'u':case 'p':
                if(clear_of_trail(&wx,&wz,32) && world_spot_free(wx,wz,29) && forest_stage_spot_free(wx,wz,34) && s_collider_count<MAX_COLLIDERS && snow_stage_place(SNOW_PROP_RUIN,wx,wz,frange(0,6.2831853f),1))add_circle_collider(wx,wz,29);
                return true;
            case 'Y':case '~':
                add_scene_prop(SCENE_DECAL_BONES,wx,wz,38,0);return true;
            default:break; // Preserve the existing ice, decals, fire and gameplay markers below.
        }
    }
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
        case 'A': return true; // Retired skeletal mammoth; no invisible collider.
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
    if(authored_swamp()) {
        switch(ch) {
            case 'R':case 'G':
                if(clear_of_trail(&wx,&wz,36) && world_spot_free(wx,wz,30) && forest_stage_spot_free(wx,wz,36) && s_collider_count<MAX_COLLIDERS &&
                   swamp_stage_place((SwampProp)(rand()%3),wx,wz,frange(0,6.2831853f)))add_circle_collider(wx,wz,25);
                return true;
            case 'V':
                if(clear_of_trail(&wx,&wz,32) && world_spot_free(wx,wz,30) && forest_stage_spot_free(wx,wz,32) && s_collider_count<MAX_COLLIDERS &&
                   swamp_stage_place(SWAMP_PROP_LOG,wx,wz,frange(0,6.2831853f)))add_circle_collider(wx,wz,29);
                return true;
            case 'h':
                if(clear_of_trail(&wx,&wz,40) && world_spot_free(wx,wz,32) && forest_stage_spot_free(wx,wz,40) && s_collider_count<MAX_COLLIDERS &&
                   swamp_stage_place(SWAMP_PROP_SHELTER,wx,wz,0))add_box_collider(wx,wz,29,24);
                return true;
            case 'W':case 'w':
                if(swamp_stage_pool(wx,wz,ch=='W'?68:43,false)) {
                    add_scene_prop(SCENE_SWAMP_REEDS_A,wx+36,wz-24,27,0);
                    add_scene_prop(SCENE_SWAMP_REEDS_B,wx-34,wz+22,24,0);
                }
                return true;
            case 'm':case 'k':case 'o':swamp_stage_pool(wx,wz,44,true);return true;
            case 'K':
                if(!on_trail(wx,wz,15))add_scene_prop((rand()&1)?SCENE_SWAMP_REEDS_A:SCENE_SWAMP_REEDS_B,wx,wz,frange(24,34),0);
                return true;
            case 'B':
                if(clear_of_trail(&wx,&wz,30) && world_spot_free(wx,wz,26) && forest_stage_spot_free(wx,wz,30))add_scene_prop(SCENE_SWAMP_ROWBOAT,wx,wz,25,23);
                return true;
            case 'U':case 'F':
                if(clear_of_trail(&wx,&wz,22) && world_spot_free(wx,wz,18) && forest_stage_spot_free(wx,wz,24))add_scene_prop(ch=='U'?SCENE_SWAMP_TOTEM:SCENE_SWAMP_LANTERN,wx,wz,ch=='U'?52:44,8);
                return true;
            case 'H':add_scene_prop(SCENE_SWAMP_SHROOM,wx,wz,19,0);return true;
            case 'Y':case 'b':add_scene_prop(SCENE_DECAL_BONES,wx,wz,38,0);return true;
            case 'g':case 'v':add_scene_prop(SCENE_DECAL_BLOOD,wx,wz,42,0);return true;
            case 'l':
                if(swamp_stage_speed(wx,wz)<1)add_scene_prop(SCENE_DECAL_GRAVE,wx,wz,38,0);
                return true;
            default:return forsaken_place(ad,ch,wx,wz);
        }
    }
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
static void swamp_dressing(void) {
    if(!authored_swamp())return;
    AreaData *ad=&g_area_data[g_current_area];
    int boats=0,rituals=0,fences=0;
    static const ScenePropId ritual[3]={SCENE_FOREST_WOOD_CAGE,SCENE_FOREST_PYRE_STAKE,SCENE_FOREST_BONE_EFFIGY};
    static const float heights[3]={48,72,60},radii[3]={23,17,16};
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;if(!mapgen_visual_cell(mx,my,&cell))continue;
        // Reserve these silhouettes before trees, independently of rare B markers.
        // First sweep boats, then ritual pieces, then horizontal fence fragments.
        for(int pass=0;pass<3;pass++) {
            int made=0,goal=pass==0?(boats<3 || (mx+my*MG_CW)%4==0):pass==1?2:4;
            for(int j=0;j<36 && made<goal;j++) {
                int k=(j+cell.index*5)%36,c=mx*MG_STEP+1+k%6,r=my*MG_STEP+1+k/6;
                if(!strchr(".KRHVWwmkolgYb",s_grid[r][c]))continue;
                float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
                float radius=pass==0?43:pass==1?38:33;
                if(on_trail(x,z,pass==2?10:20) || !world_spot_free(x,z,radius) || !forest_stage_spot_free(x,z,radius) ||
                   vec2_dist_sq(x,z,ad->spawn_x,ad->spawn_z)<120*120 || vec2_dist_sq(x,z,ad->exit_x,ad->exit_z)<105*105)continue;
                bool reserved=false;
                for(int rr=1;rr<s_grid_rowsN-1 && !reserved;rr++)for(int cc=1;cc<s_grid_cols-1;cc++) {
                    if(s_grid[rr][cc]!='C')continue;
                    float px=(cc+.5f)*TILE-s_half_w,pz=s_half_h-(rr+.5f)*TILE;
                    if(vec2_dist_sq(x,z,px,pz)<100*100){reserved=true;break;}
                }
                if(reserved || s_collider_count>=MAX_COLLIDERS || s_scene_prop_count>=MAX_SCENE_PROPS)continue;
                if(pass==0) {
                    // Wide hull footprint and both ends are reserved against trees.
                    if(!world_spot_free(x-32,z,18) || !world_spot_free(x+32,z,18))continue;
                    add_scene_prop(SCENE_SWAMP_ROWBOAT,x,z,30,0);
                    add_box_collider(x,z,42,14);boats++;
                } else if(pass==1) {
                    int kind=rituals%3;
                    add_scene_prop(ritual[kind],x,z,heights[kind],radii[kind]);rituals++;
                } else {
                    if(!swamp_stage_place(SWAMP_PROP_FENCE,x,z,0))continue;
                    add_box_collider(x,z,30,6);fences++;
                }
                made++;
            }
        }
    }
    debugf("[SWAMP] Reserved boats=%d ritual sprites=%d horizontal fences=%d\n",boats,rituals,fences);
}
static void swamp_wetlands(void) {
    if(!authored_swamp())return;
    // Seed paired wetlands from the same random room choices, preserving all
    // dry through-routes. The renderer owns the exact shared slow polygons.
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;if(!mapgen_visual_cell(mx,my,&cell) || cell.category==3)continue;
        for(int k=0;k<2;k++) {
            int c=mx*MG_STEP+(k?5:2),r=my*MG_STEP+(k?5:2);
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            if(swamp_stage_pool(x,z,92,false)) {
                add_scene_prop(k?SCENE_SWAMP_REEDS_A:SCENE_SWAMP_REEDS_B,x-35,z-46,29,0);
                add_scene_prop(SCENE_SWAMP_REEDS_A,x+42,z+31,24,0);
                if(!on_trail(x,z,8))add_scene_prop(SCENE_DECAL_GRAVE,x,z,38,0);
            }
        }
        float x=(mx*MG_STEP+2.5f)*TILE-s_half_w,z=s_half_h-(my*MG_STEP+5.5f)*TILE;
        swamp_stage_pool(x,z,62,true);
    }
    const FloorQuad *floor;int nf=mapgen_floor(&floor),decks=0;
    uint8_t per_room[MG_CH][MG_CW]={0};
    for(int i=0;i<nf;i++) {
        const FloorQuad *f=&floor[i];if(f->kind!=1 || i%4!=0)continue;
        bool fan=false;
        for(int j=0;j<nf;j++)if(i!=j && floor[j].kind==1 && floor[j].x[0]==f->x[0] && floor[j].z[0]==f->z[0]){fan=true;break;}
        if(fan)continue;
        float sx=(f->x[0]+f->x[1])*.5f,sz=(f->z[0]+f->z[1])*.5f;
        float ex=(f->x[2]+f->x[5])*.5f,ez=(f->z[2]+f->z[5])*.5f;
        float x=(sx+ex)*.5f,z=(sz+ez)*.5f,dx=ex-sx,dz=ez-sz,len=hypotf(dx,dz);
        if(len<1)continue;
        dx/=len;dz/=len;
        int mx=(int)((x+s_half_w)/(MG_STEP*TILE)),my=(int)((s_half_h-z)/(MG_STEP*TILE));
        if(mx<0 || my<0 || mx>=MG_CW || my>=MG_CH || per_room[my][mx]>=2)continue;
        bool free=true;
        for(int k=-1;k<=1;k++)if(!world_spot_free(x+dx*k*32,z+dz*k*32,23) || !forest_stage_spot_free(x+dx*k*32,z+dz*k*32,23)){free=false;break;}
        if(free && swamp_stage_place(SWAMP_PROP_WALKWAY,x,z,atan2f(dz,dx))){per_room[my][mx]++;decks++;}
    }
    debugf("[SWAMP] Placed %d short timber walkways\n",decks);
}
static void snow_setpieces(void) {
    if(!authored_snow())return;
    AreaData *ad=&g_area_data[g_current_area];
    int carts=0,ruins=0;
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;if(!mapgen_visual_cell(mx,my,&cell))continue;
        int made=0;
        for(int j=0;j<36 && made<2;j++) {
            int k=(j+cell.index*5)%36,c=mx*MG_STEP+1+k%6,r=my*MG_STEP+1+k/6;
            if(!strchr(".RKVBJ",s_grid[r][c]))continue;
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            if(on_trail(x,z,36) || !world_spot_free(x,z,38) || !forest_stage_spot_free(x,z,40) ||
               vec2_dist_sq(x,z,ad->spawn_x,ad->spawn_z)<125*125 || vec2_dist_sq(x,z,ad->exit_x,ad->exit_z)<125*125)continue;
            bool reserved=false;
            for(int rr=1;rr<s_grid_rowsN-1 && !reserved;rr++)for(int cc=1;cc<s_grid_cols-1;cc++)if(s_grid[rr][cc]=='C') {
                float px=(cc+.5f)*TILE-s_half_w,pz=s_half_h-(rr+.5f)*TILE;
                if(vec2_dist_sq(x,z,px,pz)<100*100){reserved=true;break;}
            }
            if(reserved || s_collider_count>=MAX_COLLIDERS)continue;
            if(made==0) {
                static const ScenePropId frozen[]={SCENE_ICE_FROZEN_WARRIOR,SCENE_ICE_FROZEN_WOLF,SCENE_FROST_HANGED,SCENE_FROST_SKULLS};
                static const float heights[]={58,38,62,58},radii[]={16,23,12,13};
                int kind=(mx+my*MG_CW)%4;
                add_scene_prop(frozen[kind],x,z,heights[kind],radii[kind]);
            } else if(carts<4 && (mx+my*MG_CW)%3==0) {
                add_scene_prop(SCENE_SNOW_V1_CART,x,z,35,0);add_box_collider(x,z,28,17);carts++;
            } else {
                if(!snow_stage_place(SNOW_PROP_RUIN,x,z,(mx+my)%2?1.5707963268f:0,1))continue;
                add_circle_collider(x,z,30);ruins++;
            }
            add_scene_prop(SCENE_SNOW_V1_SHRUB,x+35,z-22,20,0);made++;
        }
    }
    debugf("[SNOW] Reserved carts=%d ruins=%d before trees\n",carts,ruins);
}
static void heart_cave_dressing(void) {
    if(!authored_heart())return;
    int fungi=0,cocoons=0;
    for(int r=1;r<s_grid_rowsN-1;r++)for(int c=1;c<s_grid_cols-1;c++) {
        if(s_grid[r][c]!='.')continue;
        bool edge=tile_blocked(s_grid[r-1][c]) || tile_blocked(s_grid[r+1][c]) || tile_blocked(s_grid[r][c-1]) || tile_blocked(s_grid[r][c+1]);
        if(!edge)continue;
        unsigned h=(unsigned)c*73856093u^(unsigned)r*19349663u;
        float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
        if(!world_spot_free(x,z,18))continue;
        if(fungi<48 && h%3==0) {
            add_scene_prop(SCENE_SWAMP_SHROOM,x,z,19,0);
            add_scene_prop(SCENE_SWAMP_SHROOM,x+12,z-8,12,0);fungi++;
        } else if(cocoons<6 && h%11==0 && r>18) {
            // Low fungus replaces the legacy orange hanging-body silhouette.
            add_scene_prop(SCENE_SWAMP_SHROOM,x,z,24,0);cocoons++;
        }
    }
    static const struct {ScenePropId id;float c,r,height,radius;} props[]={
        {SCENE_SWAMP_SHROOM,15,9,24,0},
        {SCENE_SWAMP_SHROOM,19,12,22,0},
        {SCENE_SWAMP_SHROOM,20,9,15,0},
        {SCENE_DECAL_CRACKS,19,8,36,0},
        {SCENE_DECAL_CRACKS,15,10,34,0}
    };
    for(unsigned i=0;i<sizeof(props)/sizeof(props[0]);i++) {
        float x=(props[i].c+.5f)*TILE-s_half_w,z=s_half_h-(props[i].r+.5f)*TILE;
        if(world_spot_free(x,z,fmaxf(props[i].radius,12)))add_scene_prop(props[i].id,x,z,props[i].height,props[i].radius);
    }
}
static void forsaken_populate(void) {
    int slots[MAX_MARKERS], ns = 0;
    for (int i = 0; i < s_marker_count; i++) if (s_markers[i].ch == 'C') slots[ns++] = i;
    for (int i = ns - 1; i > 0; i--) { int j = rand() % (i + 1); int t = slots[i]; slots[i] = slots[j]; slots[j] = t; }
    int open_n = ns < 3 ? ns : 3;
    if (ns <= open_n && ns >= 2) open_n = ns - 1;   // garante uma vaga para o mascate
    // Baus grandes vem selados: cada um tem um evento (emboscada, guardiao ou vigilia) que precisa ser vencido.
    s_cev_count = 0;
    for (int i = 0; i < open_n; i++) {
        float cx = s_markers[slots[i]].x, cz = s_markers[slots[i]].z;
        items_add_chest_ex(cx, cz, false, false); // selected events attach after all maps finish loading
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
        if(reserved_boss_floor(c,r))continue;
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
        if (VEIL_LEVEL_REVIEW && VEIL_REVIEW_HIDE_MONSTERS) continue; // optional empty-map inspection
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
        if (authored_cemetery() && s_cem_reserved[r*s_grid_cols+c]) continue;
        if(reserved_boss_floor(c,r))continue;
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
            else if (roll < 8 && !on_trail(wx, wz, 14.0f)) {
                if(world_spot_free(wx,wz,17) && forest_stage_spot_free(wx,wz,20))snow_stage_place(SNOW_PROP_ROCK_C,wx,wz,frange(0,6.2831853f),.65f);
            }
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
        if(authored_forest()) {
            if(roll<3)add_scene_prop(roll==0?SCENE_DECAL_BLOOD:SCENE_DECAL_BONES,wx,wz,frange(26,38),0);
            else if(roll<10 && !on_trail(wx,wz,18)) {
                if(roll<8)forest_undergrowth(wx,wz,2);
                else add_scene_prop(pick3(SCENE_CAVE_MUSH_A,3),wx,wz,frange(14,22),0);
            }
            continue;
        }
        if(authored_crypt()) {
            if(roll<4)add_scene_prop((ScenePropId)(SCENE_DECAL_BLOOD+rand()%3),wx,wz,frange(30,42),0);
            continue;
        }
        if (s_crypt_theme) {   // legacy cave scenery remains available to other themes
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
    if(VEIL_LEVEL_REVIEW && VEIL_REVIEW_HIDE_MONSTERS)return;
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
    if ((!authored_cemetery() && !authored_forest() && !authored_swamp() && !authored_snow() && s_cem_nxc <= 0) || s_cem_layers <= 0) return false;
    for (int o = 0; o < 5; o++) {
        float px = x + off[o][0] * margin, pz = z + off[o][1] * margin;
        for (int i = 0; i < s_fl_n; i++) {
            const FloorQuad *f = &s_fl[i];
            if ((authored_forest() || authored_swamp() || authored_snow())?f->kind==3:f->kind!=0) continue;
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

// Place compact authored groups inside the existing random rooms. Cosmetic corner
// markers may be replaced, but gameplay markers and the through-path are reserved.
static bool cemetery_corner(int c,int r,float *x,float *z) {
    if(c<1 || r<1 || c+1>=s_grid_cols-1 || r+1>=s_grid_rowsN-1)return false;
    for(int y=r;y<r+2;y++)for(int xx=c;xx<c+2;xx++) {
        if(s_cem_reserved[y*s_grid_cols+xx] || !strchr(".RTXKUobvk",s_grid[y][xx]))return false;
    }
    *x=(c+1)*TILE-s_half_w;*z=s_half_h-(r+1)*TILE;
    // Sample the complete footprint, not only its centre, against the stone path.
    for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++)
        if(on_trail(*x+dx*50,*z+dz*43,12))return false;
    return true;
}
static void cemetery_reserve_corner(int c,int r) {
    for(int y=r;y<r+2;y++)for(int x=c;x<c+2;x++)s_cem_reserved[y*s_grid_cols+x]=true;
}
static void cemetery_compose(void) {
    memset(s_cem_reserved,0,sizeof(s_cem_reserved));
    int crypts=0,plots=0;
    static const int dc[4]={0,4,0,4},dr[4]={0,0,4,4};
    for(int my=0;my<MG_CH;my++)for(int mx=0;mx<MG_CW;mx++) {
        MapVisualCell cell;
        if(!mapgen_visual_cell(mx,my,&cell) || !cell.used)continue;
        unsigned order=(mx*7u+my*13u+cell.index*3u)&3u;
        int made=0;
        for(int k=0;k<4 && made<2;k++) {
            int corner=(order+k)&3;
            int c=mx*MG_STEP+1+dc[corner],r=my*MG_STEP+1+dr[corner];
            float x,z;
            if(!cemetery_corner(c,r,&x,&z))continue;
            bool crypt=made==0 && crypts<4 && ((mx+my+cell.index)%3==0 || crypts<2);
            if(crypt) {
                if(s_collider_count>=MAX_COLLIDERS || !cemetery_stage_place(CEM_CRYPT,x,z,corner<2?3.14159265f:0))continue;
                add_box_collider(x,z,48,42);crypts++;
            } else {
                if(s_collider_count+5>MAX_COLLIDERS)continue;
                // Two ordered graves inside a U-shaped iron enclosure, open to the south.
                bool fit=cemetery_stage_place(CEM_GRAVE,x-17,z+12,0);
                fit=cemetery_stage_place(CEM_CROSS,x+17,z+12,0)&&fit;
                fit=cemetery_stage_place(CEM_FENCE,x,z+36,0)&&fit;
                fit=cemetery_stage_place(CEM_FENCE,x-36,z,1.57079633f)&&fit;
                fit=cemetery_stage_place(CEM_FENCE,x+36,z,1.57079633f)&&fit;
                assertf(fit,"Cemetery burial group capacity");
                add_circle_collider(x-17,z+12,10);add_circle_collider(x+17,z+12,10);
                add_box_collider(x,z+36,36,6);add_box_collider(x-36,z,6,36);add_box_collider(x+36,z,6,36);
                plots++;
            }
            cemetery_reserve_corner(c,r);made++;
        }
    }
    debugf("[CEM V3] composed %d crypts and %d burial plots in random rooms\n",crypts,plots);
}

static bool cem_visible(float x0, float z0, float x1, float z1) {
    // Conservative camera-follow guard: the viewport eases behind the player,
    // so keep the proven near-player window while its frustum catches distant
    // visible cells. This prevents holes as the camera settles or turns.
    bool guard = x1 > g_druid.x - 300.0f && x0 < g_druid.x + 300.0f &&
                 z1 > g_druid.z - 160.0f && z0 < g_druid.z + 300.0f;
    if (guard) return true;
    T3DViewport *vp = t3d_viewport_get();
    if (!vp) return true;
    T3DVec3 min = {{x0, -8.0f, z0}}, max = {{x1, 180.0f, z1}};
    return t3d_frustum_vs_aabb(&vp->viewFrustum, &min, &max);
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

static bool world_quad_visible(const T3DViewport *vp, const WorldAABB16 *bounds) {
    return !vp || t3d_frustum_vs_aabb_s16(&vp->viewFrustum, bounds->min, bounds->max);
}

static void draw_culled_quad_mesh(T3DVertPacked *verts, int vert_count,
                                  const WorldAABB16 *bounds, const T3DViewport *vp) {
    int quads = vert_count / 6, run = -1;
    for (int q = 0; q <= quads; q++) {
        bool visible = q < quads && world_quad_visible(vp, &bounds[q]);
        if (visible) {
            if (run < 0) run = q;
        } else if (run >= 0) {
            cem_draw_run(verts, run, q - run);
            run = -1;
        }
    }
}

static void draw_culled_floor_chunks(const T3DViewport *vp) {
    int count = s_floor_chunk_cols * s_floor_chunk_rows, run = -1;
    for (int q = 0; q <= count; q++) {
        bool visible = false;
        if (q < count && vp) {
            int cx = q % s_floor_chunk_cols, cz = q / s_floor_chunk_cols;
            float x0 = -s_half_w + cx * FLOOR_CHUNK_TILES * TILE;
            float z0 = -s_half_h + cz * FLOOR_CHUNK_TILES * TILE;
            float x1 = fminf(x0 + FLOOR_CHUNK_TILES * TILE, s_half_w);
            float z1 = fminf(z0 + FLOOR_CHUNK_TILES * TILE, s_half_h);
            T3DVec3 min = {{x0, -2.0f, z0}}, max = {{x1, 2.0f, z1}};
            visible = t3d_frustum_vs_aabb(&vp->viewFrustum, &min, &max);
        } else if (q < count) {
            visible = true;
        }
        if (visible) {
            if (run < 0) run = q;
        } else if (run >= 0) {
            cem_draw_run(s_floor_verts, run, q - run);
            run = -1;
        }
    }
}

static void cem_begin_layer(sprite_t *spr) {
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
    rdpq_sprite_upload(TILE0, spr, &(rdpq_texparms_t){
        .s.repeats = REPEAT_INFINITE, .t.repeats = REPEAT_INFINITE
    });
    gfx_push_identity();
}



void world_init(void) {
    // The title and character-selection screens do not draw a world. Delay map
    // generation until the player confirms a character, avoiding a full map
    // build that would immediately be discarded and built again.
    debugf("[WORLD] World system ready; map generation deferred until start.\n");
}

static void expansion_grid(AreaData *ad,const char *const *map,int rows,int cols) {
    bool desert=g_campaign==3;
    expansion_stage_build(map,rows,cols,g_campaign,g_current_area);
    ad->clear_color=desert?RGBA32(78,63,44,255):RGBA32(22,27,33,255);
    ad->fog_color=desert?RGBA32(151,127,91,255):RGBA32(57,67,78,255);
    ad->spawn_yaw=1.5707963f;ad->exit_radius=40;ad->exit_x=ad->exit_z=99999;
    s_has_vendor=false;s_fl_n=0;s_cem_layers=0;events_setup();
    static const float dh[6]={57,29,70,72,28,53.3333f},vh[6]={78,45,42,70,66,34};
    static const float chapel_h[6]={30,22,19,27,34,46};
    bool chapel=!desert && g_current_area==AREA_BLACK_ALTAR;
    for(int r=0;r<rows;r++)for(int c=0;c<cols;c++) {
        char ch=map[r][c];float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
        if(ch=='E'){ad->spawn_x=x;ad->spawn_z=z;}
        else if(ch=='O'){ad->exit_x=x;ad->exit_z=z;}
        else if(ch=='B'){s_boss_x=x;s_boss_z=z;}
        else if(ch=='C')items_add_chest(x,z,0);
        else if(ch=='p')add_circle_collider(x,z,23);
        else if(ch=='t')add_box_collider(x,z,desert?25:36,desert?39:14);
        else if(ch=='*') {
            assertf(s_seal_count<SEAL_COUNT_MAX,"Too many expansion seals");
            Seal *sl=&s_seals[s_seal_count++];*sl=(Seal){x,z,0,0,false,false,-1,-1};
            // These coordinates remain available to the new mission logic;
            // the old three capture props are no longer created here.
        } else if(ch>='0' && ch<='5') {
            int k=ch-'0';ScenePropId id=(ScenePropId)((desert?SCENE_EX_DESERT_STELA:chapel?SCENE_CHAPEL_CANDLES:SCENE_EX_VICT_LAMP)+k);
            add_scene_prop(id,x,z,desert?dh[k]:chapel?chapel_h[k]:vh[k],0);
            if(chapel) {
                if(k==2 || k==4 || k==5)add_box_collider(x,z,k==4?20:k==5?14:12,k==5?10:k==4?8:7);
                else add_circle_collider(x,z,k==0?3:k==1?8:6);
            } else {
                if(k==1 || (!desert && k==2))add_box_collider(x,z,desert?16:29,12);
                else if(k!=4 || !desert)add_circle_collider(x,z,k==0?9:12);
            }
        } else if(ch=='f') {
            add_scene_prop(SCENE_EX_VICT_FENCE,x,z,34,0);add_box_collider(x,z,29,5);
        }
    }
    if(g_current_area==AREA_BLACK_ALTAR)assertf(s_seal_count==3,"Boss stage needs three reachable wards");
}
static void load_grid(const char *const *map, int rows, int cols) {
    AreaId a = g_current_area; AreaData *ad = &g_area_data[a];
    s_grid = map; s_grid_rowsN = rows; s_grid_cols = cols;
    s_half_w = cols*TILE*0.5f; s_half_h = rows*TILE*0.5f;
    ad->min_x=-s_half_w; ad->max_x=s_half_w; ad->min_z=-s_half_h; ad->max_z=s_half_h;
    for (int i=0; i<cols*rows && i<MAX_GRID_TILES; i++) s_col_head[i]=-1;
    memset(s_explored, 0, sizeof(s_explored));
    s_marker_count=0; s_cev_count=0; s_ambush_count=0; s_spawner_count=0; s_elites_total=0; s_elites_shown=0; s_pack_seq=1;
    assertf(modular_stage(),"Missing authored stage renderer");
    s_cem_nxc = 0; s_cem_layers = 0;
    s_swamp_theme = (a == AREA_HOLLOW_CRYPT && g_campaign == 1);
    s_ice_theme = (a == AREA_HOLLOW_CRYPT && g_campaign == 2);
    s_snow_theme = (a == AREA_FORSAKEN_PATH && g_campaign == 2);
    s_crypt_theme = (a == AREA_HOLLOW_CRYPT && !s_swamp_theme && !s_ice_theme);
    s_slip_count = 0;
    s_altar_theme = (a == AREA_BLACK_ALTAR);
    s_slow_count = 0;
    s_seal_count = 0; s_seals_done = 0; s_boss_summoned = false; s_hazard_count = 0;
    if(expansion_campaign()){expansion_grid(ad,map,rows,cols);return;}
    if (authored_cemetery() || authored_forest() || authored_swamp() || authored_snow()) {
        if(authored_forest())forest_stage_build(map,rows,cols);
        else if(authored_swamp())swamp_stage_build(map,rows,cols);
        else if(authored_snow())snow_stage_build(map,rows,cols);
        else cemetery_stage_build(map,rows,cols);
        s_fl_n=mapgen_floor(&s_fl);s_cem_layers=s_fl_n;
        for (int i=0;i<s_fl_n;i++) {
            int x0=s_fl[i].x[0],x1=x0,z0=s_fl[i].z[0],z1=z0;
            for(int k=1;k<6;k++) {
                if(s_fl[i].x[k]<x0)x0=s_fl[i].x[k];
                if(s_fl[i].x[k]>x1)x1=s_fl[i].x[k];
                if(s_fl[i].z[k]<z0)z0=s_fl[i].z[k];
                if(s_fl[i].z[k]>z1)z1=s_fl[i].z[k];
            }
            s_cem_bb[i][0]=x0;s_cem_bb[i][1]=z0;s_cem_bb[i][2]=x1;s_cem_bb[i][3]=z1;
        }
        if(authored_forest()) {
            ad->clear_color=RGBA32(13,22,23,255);ad->fog_color=RGBA32(31,46,46,255);
        } else if(authored_swamp()) {
            ad->clear_color=RGBA32(14,23,22,255);ad->fog_color=RGBA32(30,44,40,255);
        } else if(authored_snow()) {
            ad->clear_color=RGBA32(104,116,124,255);ad->fog_color=RGBA32(151,162,168,255);
        } else {
            ad->clear_color=RGBA32(22,27,26,255);ad->fog_color=RGBA32(42,49,47,255);
            cemetery_compose();
        }
    }
    else if(jarl_castle()) {
        jarl_stage_build(map,rows,cols);
        ad->clear_color=RGBA32(24,33,43,255);ad->fog_color=RGBA32(67,84,100,255);
    }
    else if(authored_ice()) {
        ice_stage_build(map,rows,cols);
        ad->clear_color=RGBA32(34,47,59,255);ad->fog_color=RGBA32(79,104,120,255);
    }
    else if(authored_crypt()) {
        crypt_stage_build(map,rows,cols);
        ad->clear_color=RGBA32(13,16,20,255);ad->fog_color=RGBA32(23,28,34,255);
    }
    else if(authored_altar()) {
        altar_stage_build(map,rows,cols);
        ad->clear_color=RGBA32(20,9,8,255);ad->fog_color=RGBA32(36,13,10,255);
    }
    else if(authored_heart()) {
        heart_stage_build(map,rows,cols);
        ad->clear_color=RGBA32(12,21,22,255);ad->fog_color=RGBA32(26,40,39,255);
    }

    ScenePropId fire = (a==AREA_FORSAKEN_PATH)?SCENE_CAMPFIRE : (a==AREA_HOLLOW_CRYPT)?SCENE_CRYPT_BRAZIER : SCENE_INFERNAL_BRAZIER;
    ScenePropId body = (a==AREA_HOLLOW_CRYPT)?SCENE_CULTIST_BODY : SCENE_TRAVELER_BODY;
    ScenePropId portal = (a==AREA_FORSAKEN_PATH)?SCENE_PORTAL_PATH : (a==AREA_HOLLOW_CRYPT)?SCENE_PORTAL_CRYPT : SCENE_PORTAL_ALTAR;
    EnemyType cyc[3] = { ENEMY_TYPE_SKELETON, ENEMY_TYPE_DEMON, ENEMY_TYPE_CULTIST };
    int pminr=9999,pmaxr=-1,pminc=9999,pmaxc=-1, ecount=0;

    // Reserve visible fences before the tree/prop pass, so they do not lose
    // all available roadside positions to trees placed earlier in the load.
    forest_path_fences();
    if(authored_forest() || authored_swamp() || authored_snow()) {
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++) {
            float x=(c+.5f)*TILE-s_half_w,z=s_half_h-(r+.5f)*TILE;
            if(map[r][c]=='E'){ad->spawn_x=x;ad->spawn_z=z;}
            if(map[r][c]=='P'){ad->exit_x=x;ad->exit_z=z;}
        }
        if(authored_forest())forest_ritual_sites();
        else if(authored_snow())snow_setpieces();
        else {
            swamp_dressing();
            for(int r=0;r<rows;r++)for(int c=0;c<cols;c++)if(map[r][c]=='h' || map[r][c]=='B')
                swamp_place(ad,map[r][c],(c+.5f)*TILE-s_half_w,s_half_h-(r+.5f)*TILE);
            swamp_wetlands();
        }
    }
    for (int r=0;r<rows;r++) for (int c=0;c<cols;c++){
        char ch=map[r][c];
        if(authored_swamp() && (ch=='h' || ch=='B'))continue;
        if(authored_heart() && (ch=='^' || ch=='='))continue;
        if(authored_cemetery() && s_cem_reserved[r*cols+c])continue;
        float wx=(c+0.5f)*TILE - s_half_w;
        float wz=s_half_h - (r+0.5f)*TILE;
        if (authored_forest() && forest_place(ad,ch,wx,wz))continue;
        if (authored_snow() && snow_place(ad,ch,wx,wz))continue;
        if (a == AREA_FORSAKEN_PATH && forsaken_place(ad, ch, wx, wz)) continue;
        if (a == AREA_HOLLOW_CRYPT && g_campaign == 1 && swamp_place(ad, ch, wx, wz)) continue;
        if (a == AREA_HOLLOW_CRYPT && g_campaign == 2 && ice_place(ad, ch, wx, wz)) continue;
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
    if (pmaxr>=0 && !authored_altar()){
        float cx=((pminc+pmaxc+1)*0.5f)*TILE - s_half_w;
        float cz=s_half_h - ((pminr+pmaxr+1)*0.5f)*TILE;
        float half=(float)(pmaxc-pminc+1)*TILE*0.5f;
        build_penta(cx, cz, half); // other themes retain their existing marking
    }
    if (a == AREA_FORSAKEN_PATH || a == AREA_HOLLOW_CRYPT || a == AREA_BLACK_ALTAR) { forsaken_populate(); events_setup(); forsaken_scatter(ad); }
    crypt_burial_groups();
    crypt_ruins();
    altar_battle_dressing();
    forest_clearings();
    heart_cave_dressing();
}

void world_load_area(AreaId area) {
    uint64_t load_started = get_ticks();
    cemetery_stage_release();
    if (area >= AREA_COUNT) area = AREA_FORSAKEN_PATH;
    g_current_area = area;
    s_minimap_on=true;
    s_collider_count = 0;
    s_grid = NULL;
    s_scene_prop_count = 0;
    s_forest_foliage = 0;
    memset(s_forest_foliage_room,0,sizeof(s_forest_foliage_room));
    s_wall_vert_count = 0;
    s_wall_top_vert_count = 0;
    s_penta_vert_count = 0;

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
            // Woodland now owns its cold materials; no global olive recolor.
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

    if(expansion_campaign()) {
        int index=(g_campaign-3)*AREA_COUNT+area;
        ExpansionPlan plan=EX_PLANS[index][area==AREA_BLACK_ALTAR?0:rand()%3];
        // Exploration retains authored alternative routes and random handedness.
        // Boss plants are fixed and deliberately different from the first boss.
        static char grid[39][40];static const char *lines[39];
        bool mirror=area!=AREA_BLACK_ALTAR && (rand()&1);
        for(int r=0;r<plan.rows;r++) {
            for(int c=0;c<plan.cols;c++)grid[r][c]=plan.grid[r][mirror?plan.cols-1-c:c];
            grid[r][plan.cols]=0;lines[r]=grid[r];
        }
        load_grid(lines,plan.rows,plan.cols);
    }
    else if (area == AREA_FORSAKEN_PATH || area == AREA_HOLLOW_CRYPT) {   // mapa novo a cada partida
        int mr = 0, mc = 0;
        int kind = (area == AREA_HOLLOW_CRYPT ? 1 : 0) + (g_campaign == 1 ? 2 : g_campaign==2?4:0);
        const char *const *m = mapgen_build(kind, &mr, &mc);
        load_grid(m, mr, mc);
    }
    else if (area == AREA_BLACK_ALTAR) {
        if(g_campaign==1)load_grid(MAP_HEART,MAP_HEART_ROWS,MAP_HEART_COLS);
        else if(jarl_castle())load_grid(MAP_JARL,MAP_JARL_ROWS,MAP_JARL_COLS);
        else load_grid(MAP_ALT,MAP_ALT_ROWS,MAP_ALT_COLS);
    }

    boss_objective_setup();
    optional_events_setup();
    // (re)posiciona o jogador na entrada definida pela grade
    druid_reset_at(ad->spawn_x, ad->spawn_z);
    g_druid.yaw = ad->spawn_yaw; g_druid.target_yaw = ad->spawn_yaw;
    hud_show_banner(area_name(g_campaign * AREA_COUNT + area), area_subtitle(g_campaign * AREA_COUNT + area), 3.0f);
    if ((area == AREA_FORSAKEN_PATH || area == AREA_HOLLOW_CRYPT) && s_elites_total > 0) {
        char buf[80];
        snprintf(buf, sizeof(buf), area == AREA_HOLLOW_CRYPT ? T(S_GUARD_PORTAL) : T(S_GUARD_MAUS), s_elites_total);
        hud_show_message(buf, 6.0f);
    } else if (area == AREA_BLACK_ALTAR) {
        hud_show_message(T(g_campaign==4?S_BOBJ_CITY:g_campaign==3?S_BOBJ_DESERT:g_campaign==2?S_BOBJ_SNOW:g_campaign==1?S_BOBJ_FOREST:S_SEALS_MSG),6.0f);
    }
    sprites_preload_area();   // Expansion Pak: monstros ja na memoria antes da primeira horda
    debugf("[WORLD LOAD] campaign=%d area=%d props=%d total=%lu us\n",
           g_campaign, (int)area, s_scene_prop_count,
           (unsigned long)((get_ticks() - load_started) * 1000000 / TICKS_PER_SECOND));
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
        if(reserved_boss_floor(c,r))continue;
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
            bool inside = d2 < EVENT_HOLD_RADIUS * EVENT_HOLD_RADIUS && g_druid.hp > 0;
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
                bool near = d2 < EVENT_HOLD_RADIUS * EVENT_HOLD_RADIUS;
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
enum { CE_AMBUSH, CE_GUARD, CE_VIGIL, CE_BREAK, CE_COURIER, CE_SOULS, CE_PACT, CE_RESCUE, CE_HUNT, CE_MINI, CE_TYPES };
typedef struct {float x,z;int hp;bool active;} EventNode;
typedef struct {
    int chest,type,state,pack,decal,wave,remaining,carrier;
    float x,z,progress,wave_t;
    EventNode nodes[6];
} ChestEv;
#define MAX_CHEST_EV 3
static ChestEv s_cev[MAX_CHEST_EV];
#define CEV_VIGIL_R EVENT_HOLD_RADIUS

static const StrId event_titles[CE_TYPES]={S_CEV_AMB_T,S_CEV_GRD_T,S_CEV_VIG_T,
    S_EVENT_BREAK,S_EVENT_COURIER,S_EVENT_SOULS,S_EVENT_PACT,S_EVENT_RESCUE,S_EVENT_HUNT,S_EVENT_MINI};
static bool event_uses_chest(int type) {return type<=CE_VIGIL || type==CE_COURIER;}
static void draw_optional_minimap(int pcol,int prow,int x0,int y0) {
    for(int i=0;i<s_cev_count;i++) {
        const ChestEv *e=&s_cev[i];if(e->state>=3)continue;
        float x=e->x,z=e->z;bool hunt=e->type==CE_HUNT && e->state==1 && e->carrier>=0;
        if(hunt) {
            const Enemy *en=&enemies_get_array()[e->carrier];
            if(en->active && en->pack==e->pack && en->hp>0){x=en->x;z=en->z;}
        }
        int c=(int)((x+s_half_w)/TILE),r=(int)((s_half_h-z)/TILE);
        if(c<0 || r<0 || c>=s_grid_cols || r>=s_grid_rowsN)continue;
        if(e->state==0 && !s_explored[r*s_grid_cols+c])continue;
        int dx=pcol-c,dy=r-prow;
        if(e->state==0 && (abs(dx)>MM_HALF || abs(dy)>MM_HALF))continue;
        if(dx<-MM_HALF)dx=-MM_HALF;
        if(dx>MM_HALF)dx=MM_HALF;
        if(dy<-MM_HALF)dy=-MM_HALF;
        if(dy>MM_HALF)dy=MM_HALF;
        int sx=x0+(dx+MM_HALF)*MM_PX,sy=y0+(dy+MM_HALF)*MM_PX;
        rdpq_set_prim_color(hunt?RGBA32(230,85,70,255):RGBA32(220,175,75,255));
        rdpq_fill_rectangle(sx-1,sy-1,sx+3,sy+3);
    }
}
static float event_speed_factor(void) {
    for(int i=0;i<s_cev_count;i++)if(s_cev[i].type==CE_PACT && s_cev[i].state==1)return .85f;
    return 1;
}
static void optional_events_setup(void) {
    int pool[CE_TYPES];for(int i=0;i<CE_TYPES;i++)pool[i]=i;
    for(int i=CE_TYPES-1;i>0;i--){int j=rand()%(i+1),t=pool[i];pool[i]=pool[j];pool[j]=t;}
    int candidates[MAX_CHESTS],n=0;
    const Chest *ch=items_get_chests();
    for(int i=0;i<MAX_CHESTS;i++)if(ch[i].active && !ch[i].small && !ch[i].opened)candidates[n++]=i;
    for(int i=n-1;i>0;i--){int j=rand()%(i+1),t=candidates[i];candidates[i]=candidates[j];candidates[j]=t;}
    s_cev_count=0;
    int needed=MAX_CHEST_EV-s_ev_count;
    for(int i=0;i<needed;i++) {
        float x,z;int ci=-1;
        if(i<n){ci=candidates[i];x=ch[ci].x;z=ch[ci].z;}
        else if(!pick_event_spot(&x,&z)) {
            bool found=false;
            for(int r=1;r<s_grid_rowsN-1 && !found;r++)for(int c=1;c<s_grid_cols-1;c++) {
                if(tile_blocked(s_grid[r][c]))continue;
                x=(c+.5f)*TILE-s_half_w;z=s_half_h-(r+.5f)*TILE;
                bool free=world_spot_free(x,z,16) && vec2_dist_sq(x,z,g_area_data[g_current_area].spawn_x,g_area_data[g_current_area].spawn_z)>140*140;
                for(int k=0;k<s_cev_count;k++)if(vec2_dist_sq(x,z,s_cev[k].x,s_cev[k].z)<180*180)free=false;
                if(free){found=true;break;}
            }
            assertf(found,"No reachable position for three map events");
        }
        ChestEv *e=&s_cev[s_cev_count++];
        *e=(ChestEv){.chest=ci,.type=pool[i],.pack=-1,.decal=-1,.carrier=-1,.x=x,.z=z};
        bool visible=event_uses_chest(e->type);
        if(ci>=0)items_prepare_event_chest(ci,visible);
        else if(visible)e->chest=items_add_chest_ex(x,z,false,true);
        if(!visible)e->chest=-1;
    }
    assertf(s_cev_count+s_ev_count==3,"Every map must have three optional events");
    debugf("[EVENTS] campaign=%d area=%d optional=%d secrets=%d\n",g_campaign,g_current_area,s_cev_count,s_ev_count);
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
    if(e->state>=3)return;
    if(e->chest<0 && cards_offer_pending()>=0)return;
    e->state = 3;
    if(e->chest>=0)items_unlock_chest(e->chest);
    else {
        cards_grant_random();
        items_spawn_drop(DROP_HEALTH_POTION,e->x-12,e->z);
        items_spawn_drop(DROP_SPIRIT_VIAL,e->x+12,e->z);
        for(int k=0;k<6;k++)items_spawn_drop(DROP_ESSENCE,e->x+(k-3)*6,e->z+14);
        druid_gain_exp(160);
    }
    if (e->decal >= 0) s_scene_props[e->decal].id = scene_skin(SCENE_HELL_DECAL_ASH);
    sound_play(SFX_THORN_BURST);
    hud_show_message(T(e->chest>=0?S_CEV_FREE:S_EVENT_DONE), 2.5f);
}

static int event_node_count(const ChestEv *e) {
    int n=0;for(int k=0;k<6;k++)if(e->nodes[k].active)n++;return n;
}
static bool event_place_nodes(ChestEv *e,int n) {
    memset(e->nodes,0,sizeof(e->nodes));
    for(int k=0;k<n;k++) {
        bool placed=false;
        for(int t=0;t<160;t++) {
            float a=frand()*6.2831853f,r=frange(65,190),x=e->x+cosf(a)*r,z=e->z+sinf(a)*r;
            if(!world_spot_free(x,z,14))continue;
            bool free=true;
            for(int j=0;j<k;j++)if(vec2_dist_sq(x,z,e->nodes[j].x,e->nodes[j].z)<55*55)free=false;
            if(!free)continue;
            e->nodes[k]=(EventNode){x,z,45+20*g_difficulty,true};placed=true;break;
        }
        if(!placed){memset(e->nodes,0,sizeof(e->nodes));return false;}
    }
    return true;
}
static bool event_start(ChestEv *e) {
    if(VEIL_LEVEL_REVIEW && VEIL_REVIEW_HIDE_MONSTERS) {
        hud_show_message(T(S_EVENT_REVIEW),2);return false;
    }
    if((e->type==CE_BREAK || e->type==CE_RESCUE) && !event_place_nodes(e,3))return false;
    e->pack=s_pack_seq++;e->remaining=e->type==CE_SOULS?6:0;
    int amount=e->type==CE_COURIER || e->type==CE_HUNT || e->type==CE_MINI?1:e->type==CE_SOULS?6:5+g_difficulty;
    int spawned=0;
    for(int k=0;k<amount;k++) {
        EnemyType type=e->type==CE_COURIER?ENEMY_TYPE_SKELETON:e->type==CE_HUNT?ENEMY_TYPE_DEMON:pick_type('Z');
        int variant=e->type==CE_COURIER?EV_COURIER:e->type==CE_HUNT?EV_CHAMPION:EV_STANDARD;
        int idx=e->type==CE_MINI?spawn_miniboss_member(e->x,e->z,e->pack):
            variant==EV_STANDARD?spawn_pack_member(type,e->x,e->z,e->pack,false,70,160):
            spawn_event_member(type,e->x,e->z,e->pack,variant,e->type==CE_HUNT?260:70,e->type==CE_HUNT?450:160);
        if(idx<0)continue;
        spawned++;if(e->type!=CE_HUNT)enemies_force_chase(idx);
        if(e->type==CE_COURIER)e->carrier=idx;
        if(e->type==CE_MINI)e->carrier=idx;
        if(e->type==CE_HUNT) {
            e->carrier=idx;
            enemies_set_affixes(idx,1+(g_difficulty>0));
        }
    }
    if(!spawned){memset(e->nodes,0,sizeof(e->nodes));return false;}
    e->state=1;e->wave_t=2;
    hud_show_banner(T(event_titles[e->type]),T(e->type==CE_MINI?S_EVENT_HELP_MINI:(StrId)(S_EVENT_HELP_BREAK+e->type-CE_BREAK)),3.5f);
    if(e->type==CE_MINI){const Enemy *mini=&enemies_get_array()[e->carrier];hud_show_banner(campaign_boss_name(enemy_campaign(mini),mini->boss_phase==2),T(S_EVENT_MINI),3.5f);}
    sound_play(SFX_BOSS_ENRAGE);
    return true;
}
bool world_interact_event(void) {
    if(g_druid.hp<=0)return false;
    if(boss_objective_interact())return true;
    for(int i=0;i<s_cev_count;i++) {
        ChestEv *e=&s_cev[i];
        if(e->type>=CE_BREAK && e->state==0 && vec2_dist_sq(g_druid.x,g_druid.z,e->x,e->z)<70*70) {
            event_start(e);return true;
        }
    }
    return false;
}
bool world_event_damage_targets(float x,float z,float radius,int damage) {
    if(damage<=0)return false;
    bool hit=boss_objective_damage(x,z,radius,damage);
    for(int i=0;i<s_cev_count;i++) {
        ChestEv *e=&s_cev[i];
        if(e->state!=1 || (e->type!=CE_BREAK && e->type!=CE_RESCUE))continue;
        if(e->type==CE_RESCUE && pack_alive(e->pack)>0)continue;
        for(int k=0;k<3;k++) {
            EventNode *n=&e->nodes[k];
            if(n->active && vec2_dist_sq(x,z,n->x,n->z)<(radius+13)*(radius+13)) {
                n->hp-=damage;if(n->hp<=0)n->active=false;hit=true;
            }
        }
    }
    return hit;
}
void world_event_enemy_defeated(int index) {
    boss_objective_defeated(index);
    Enemy *en=&enemies_get_array()[index];
    for(int i=0;i<s_cev_count;i++) {
        ChestEv *e=&s_cev[i];if(e->state!=1 || e->pack!=en->pack)continue;
        if(e->type==CE_COURIER && e->carrier==index) {
            e->carrier=-1;e->nodes[0]=(EventNode){en->x,en->z,0,true};
        }
        if(e->type==CE_SOULS && e->remaining>0)for(int k=0;k<6;k++)if(!e->nodes[k].active) {
            e->nodes[k]=(EventNode){en->x,en->z,0,true};e->remaining--;break;
        }
    }
}
bool world_event_enemy_update(int index,float dt) {
    Enemy *en=&enemies_get_array()[index];
    for(int i=0;i<s_cev_count;i++) {
        ChestEv *e=&s_cev[i];
        if(e->state!=1 || e->type!=CE_COURIER || e->carrier!=index || en->pack!=e->pack)continue;
        if(en->root_timer>0 || en->state==ENEMY_STATE_HURT)return false;
        float angle=atan2f(en->z-g_druid.z,en->x-g_druid.x),step=enemy_move_speed(EB_FAST)*dt;
        if(vec2_dist_sq(en->x,en->z,e->x,e->z)>280*280)angle=atan2f(e->z-en->z,e->x-en->x);
        for(int k=0;k<8;k++) {
            float a=angle+k*.785398f,x=en->x+cosf(a)*step,z=en->z+sinf(a)*step,tx=x,tz=z;
            world_collide_circle(&tx,&tz,en->radius);
            if(vec2_dist_sq(x,z,tx,tz)>1)continue;
            en->vx=cosf(a)*enemy_move_speed(EB_FAST);en->vz=sinf(a)*enemy_move_speed(EB_FAST);en->yaw=a;
            en->state=ENEMY_STATE_CHASE;en->anim_timer+=dt*10;break;
        }
        return true;
    }
    return false;
}

static void new_event_update(ChestEv *e,float dt) {
    if(e->state==0) {
        if(vec2_dist_sq(g_druid.x,g_druid.z,e->x,e->z)<70*70)
            hud_show_message(T(e->type==CE_PACT?S_EVENT_ACCEPT:S_EVENT_START),.2f);
        return;
    }
    if(g_druid.hp<=0 && e->type==CE_PACT){e->state=4;return;}
    if(g_druid.hp<=0)return;
    int alive=pack_alive(e->pack);
    if(e->type==CE_BREAK) {if(event_node_count(e)==0 && alive==0)chest_ev_finish(e);}
    else if(e->type==CE_RESCUE) {
        if(alive==0 && event_node_count(e)==0 && vec2_dist_sq(g_druid.x,g_druid.z,e->x,e->z)<40*40)chest_ev_finish(e);
    } else if(e->type==CE_COURIER || e->type==CE_SOULS) {
        for(int k=0;k<6;k++)if(e->nodes[k].active && vec2_dist_sq(g_druid.x,g_druid.z,e->nodes[k].x,e->nodes[k].z)<24*24)e->nodes[k].active=false;
        if(e->type==CE_COURIER) {if(e->carrier<0 && !event_node_count(e))chest_ev_finish(e);}
        else if(e->remaining==0 && !event_node_count(e) && vec2_dist_sq(g_druid.x,g_druid.z,e->x,e->z)<55*55)chest_ev_finish(e);
        else if(alive==0 && e->remaining>0) {
            // Capacity shortages are retried; a quest never completes from a failed spawn.
            e->wave_t-=dt;if(e->wave_t<=0) {
                e->wave_t=2;
                int idx=spawn_pack_member(pick_type('Z'),e->x,e->z,e->pack,false,80,150);
                if(idx>=0)enemies_force_chase(idx);
            }
        }
    } else if(alive==0)chest_ev_finish(e);
}

static void chest_events_update(float dt) {
    int lvl = (int)g_current_area + 2 * (int)g_difficulty;     // 0..6: fase e dificuldade apertam o evento
    for (int i = 0; i < s_cev_count; i++) {
        ChestEv *e = &s_cev[i];
        if (e->state >= 3) continue;
        float d2 = vec2_dist_sq(g_druid.x, g_druid.z, e->x, e->z);
        bool alive = g_druid.hp > 0;
        if(VEIL_LEVEL_REVIEW && VEIL_REVIEW_HIDE_MONSTERS) {
            if(alive && d2<70*70)hud_show_message(T(S_EVENT_REVIEW),.2f);
            continue;
        }
        if(e->type>=CE_BREAK){new_event_update(e,dt);continue;}
        switch (e->type) {
        case CE_AMBUSH:     // duas levas saem das sombras em volta do bau
            if (e->state == 0) {
                if (!alive || d2 > 80.0f * 80.0f) break;
                e->state = 1; e->wave = 0;
                chest_ev_spawn(e, 5 + lvl, 'Z', 70.0f, 130.0f);
                if(pack_alive(e->pack)==0){e->state=0;break;}
                hud_show_banner(T(S_CEV_AMB_T), T(S_CEV_AMB_S), 2.0f);
                sound_play(SFX_BOSS_ENRAGE);
            } else if (pack_alive(e->pack) == 0) {
                if (e->wave == 0) {
                    e->wave = 1;
                    chest_ev_spawn(e, 4 + lvl, 'A', 110.0f, 170.0f);
                    if(pack_alive(e->pack)==0)e->wave=0;
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
                int g = spawn_event_member(gt,e->x,e->z,e->pack,EV_CHEST,30,60);
                if (g >= 0) {
                    enemies_set_affixes(g, 1 + (g_difficulty > 0));
                    enemies_force_chase(g);
                }
                for (int k = 0; k < 2 + lvl / 2; k++) {
                    int idx = spawn_pack_member(pick_type('S'), e->x, e->z, e->pack, false, 50.0f, 110.0f);
                    if (idx >= 0) enemies_force_chase(idx);
                }
                if(pack_alive(e->pack)==0){e->state=0;break;}
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

static int boss_reachable_cells(uint16_t *cells) {
    // Flood fill actual free cell centers; no guardian in an isolated room.
    static uint8_t visited[MAX_GRID_TILES];
    memset(visited,0,sizeof(visited));
    int c=(int)((g_area_data[g_current_area].spawn_x+s_half_w)/TILE);
    int r=(int)((s_half_h-g_area_data[g_current_area].spawn_z)/TILE);
    if(c<0 || r<0 || c>=s_grid_cols || r>=s_grid_rowsN)return 0;
    int head=0,n=1;cells[0]=r*s_grid_cols+c;visited[cells[0]]=1;
    while(head<n) {
        int cell=cells[head++],cc=cell%s_grid_cols,rr=cell/s_grid_cols;
        const int dc[4]={1,-1,0,0},dr[4]={0,0,1,-1};
        for(int k=0;k<4;k++) {
            int x=cc+dc[k],z=rr+dr[k];
            if(x<0 || z<0 || x>=s_grid_cols || z>=s_grid_rowsN)continue;
            int j=z*s_grid_cols+x;
            if(visited[j] || !tile_walk(s_grid[z][x]))continue;
            float wx=(x+.5f)*TILE-s_half_w,wz=s_half_h-(z+.5f)*TILE;
            if(!world_spot_free(wx,wz,12))continue;
            float px=(cc+.5f)*TILE-s_half_w,pz=s_half_h-(rr+.5f)*TILE;
            bool clear=true;
            for(int step=1;step<4;step++)if(!world_spot_free(px+(wx-px)*step/4,pz+(wz-pz)*step/4,12))clear=false;
            if(clear){visited[j]=1;cells[n++]=j;}
        }
    }
    return n;
}
static void boss_objective_setup(void) {
    memset(&s_boss_objective,0,sizeof(s_boss_objective));
    for(int i=0;i<3;i++)s_boss_objective.target[i].index=-1;
    if(g_current_area!=AREA_BLACK_ALTAR || g_campaign==0)return;
    if(g_campaign==1) {
        s_boss_objective.count=s_seal_count;
        for(int i=0;i<s_seal_count;i++)s_boss_objective.target[i]=(BossTarget){
            .x=s_seals[i].x,.z=s_seals[i].z,.index=-1,.hp=180*(g_difficulty+1)};
    } else if(g_campaign==2 || g_campaign==3) {
        static uint16_t cells[MAX_GRID_TILES];int n=boss_reachable_cells(cells);
        s_boss_objective.count=g_campaign==2?1:3;
        for(int i=0;i<s_boss_objective.count;i++) {
            bool found=false;
            for(int pass=0;pass<2 && !found;pass++)for(int t=0;t<n;t++) {
                int j=rand()%(n-t),cell=cells[j];cells[j]=cells[n-t-1];cells[n-t-1]=cell;
                float x=(cell%s_grid_cols+.5f)*TILE-s_half_w,z=s_half_h-(cell/s_grid_cols+.5f)*TILE;
                float gap=pass?140:240;
                bool free=vec2_dist_sq(x,z,g_area_data[g_current_area].spawn_x,g_area_data[g_current_area].spawn_z)>gap*gap &&
                    vec2_dist_sq(x,z,s_boss_x,s_boss_z)>120*120;
                for(int k=0;k<i;k++)if(vec2_dist_sq(x,z,s_boss_objective.target[k].x,s_boss_objective.target[k].z)<gap*gap)free=false;
                if(!free)continue;
                s_boss_objective.target[i]=(BossTarget){.x=x,.z=z,.index=-1,.pack=s_pack_seq++};found=true;break;
            }
            assertf(found,"No reachable boss guardian location");
        }
    } else {
        s_boss_objective.timer=30;
        s_boss_objective.bell_x=s_seals[0].x;s_boss_objective.bell_z=s_seals[0].z;
        // Existing reliquary is a temporary visual socket for the future bell.
        add_scene_prop(SCENE_EX_VICT_RELIQUARY,s_boss_objective.bell_x,s_boss_objective.bell_z,48,0);
    }
}
bool world_boss_guardian(int index) {
    if(g_current_area!=AREA_BLACK_ALTAR || (g_campaign!=2 && g_campaign!=3) || index<0 || index>=MAX_ENEMIES)return false;
    const Enemy *en=&enemies_get_array()[index];
    for(int i=0;i<s_boss_objective.count;i++) {
        const BossTarget *t=&s_boss_objective.target[i];
        if(t->index==index && t->pack==en->pack && en->active)return true;
    }
    return false;
}
static void boss_objective_defeated(int index) {
    if(!world_boss_guardian(index))return;
    for(int i=0;i<s_boss_objective.count;i++) {
        BossTarget *t=&s_boss_objective.target[i];
        if(t->index!=index || t->done)continue;
        t->done=true;s_boss_objective.done++;sound_play(SFX_THORN_BURST);
    }
}
static bool boss_objective_damage(float x,float z,float radius,int damage) {
    if(g_current_area!=AREA_BLACK_ALTAR || g_campaign!=1 || s_boss_summoned || damage<=0)return false;
    bool hit=false;
    for(int i=0;i<s_boss_objective.count;i++) {
        BossTarget *t=&s_boss_objective.target[i];
        if(t->done || vec2_dist_sq(x,z,t->x,t->z)>(radius+17)*(radius+17))continue;
        t->hp-=damage;hit=true;
        if(t->hp<=0) {
            t->done=true;s_boss_objective.done++;s_seals[i].done=true;
            if(s_seals[i].obelisk>=0)s_scene_props[s_seals[i].obelisk].id=scene_skin(SCENE_HELL_SEAL_OFF);
            sound_play(SFX_THORN_BURST);
        }
    }
    return hit;
}
static bool boss_objective_interact(void) {
    if(g_current_area!=AREA_BLACK_ALTAR || g_campaign!=4 || s_boss_summoned || s_boss_objective.ringing || s_boss_objective.ready)return false;
    if(vec2_dist_sq(g_druid.x,g_druid.z,s_boss_objective.bell_x,s_boss_objective.bell_z)>65*65)return false;
    if(VEIL_LEVEL_REVIEW && VEIL_REVIEW_HIDE_MONSTERS) {hud_show_message(T(S_EVENT_REVIEW),2);return true;}
    s_boss_objective.ringing=true;s_boss_objective.timer=30;s_boss_objective.wave=.5f;
    hud_show_banner(T(S_BELL_RINGS),T(S_BELL_SURVIVE),3);sound_play(SFX_UI_SELECT);return true;
}
static void boss_objective_update(float dt) {
    if(s_boss_summoned)return;
    if(g_druid.hp<=0) {
        if(g_campaign==4){
            s_boss_objective.ringing=false;s_boss_objective.ready=false;s_boss_objective.timer=30;
            Enemy *en=enemies_get_array();
            for(int i=0;i<MAX_ENEMIES;i++)if(en[i].active && en[i].pack==-700)en[i].active=false;
        }
        return;
    }
    if(g_campaign==2 || g_campaign==3)for(int i=0;i<s_boss_objective.count;i++) {
        BossTarget *t=&s_boss_objective.target[i];if(t->done || t->index>=0)continue;
        int idx=enemies_spawn_variant(g_campaign==2?ENEMY_TYPE_SKELETON:ENEMY_TYPE_CULTIST,t->x,t->z,t->pack,g_campaign==2?EV_THRONE:EV_PRIEST);
        if(idx<0)continue; // hidden enemies or full pool never count as kills
        t->index=idx;Enemy *en=&enemies_get_array()[idx];
        if(g_campaign==3)en->boss_special_cooldown=3600; // guardians attack; no support-heal loop
        if(g_difficulty>0)enemies_set_affixes(idx,1);
    }
    if(g_campaign==4 && !s_boss_objective.ringing && !s_boss_objective.ready &&
       vec2_dist_sq(g_druid.x,g_druid.z,s_boss_objective.bell_x,s_boss_objective.bell_z)<65*65)
        hud_show_message(T(S_BELL_TOUCH),.2f);
    if(g_campaign==4 && s_boss_objective.ringing) {
        s_boss_objective.timer=fmaxf(0,s_boss_objective.timer-dt);
        if(s_boss_objective.timer>0) {
            s_boss_objective.wave-=dt;
            if(s_boss_objective.wave<=0) {
                s_boss_objective.wave=4;
                // No more than twelve mission reinforcements alive at once.
                int alive=0;const Enemy *en=enemies_get_array();
                for(int i=0;i<MAX_ENEMIES;i++)if(en[i].active && en[i].pack==-700 && en[i].hp>0)alive++;
                for(int k=0;k<4 && alive<12;k++) {
                    int idx=spawn_pack_member(pick_type('A'),g_druid.x,g_druid.z,-700,false,170,230);
                    if(idx>=0){enemies_force_chase(idx);alive++;}
                }
            }
        } else s_boss_objective.ringing=false;
    }
    bool done=g_campaign==4?s_boss_objective.timer<=0:
        s_boss_objective.count>0 && s_boss_objective.done==s_boss_objective.count;
    if(!done)return;
    if(!s_boss_objective.ready) {
        s_boss_objective.ready=true;s_boss_objective.warning=2.5f;
        hud_show_banner(campaign_boss_name(g_campaign,false),T(S_BOSS_APPROACH),2.5f);return;
    }
    s_boss_objective.warning-=dt;if(s_boss_objective.warning>0)return;
    int idx=enemies_spawn(ENEMY_TYPE_BOSS_MALAKOR,s_boss_x,s_boss_z);
    if(idx<0)return;
    s_boss_summoned=true;Enemy *en=&enemies_get_array()[idx];
    en->state=ENEMY_STATE_RECOVERY;en->state_timer=1;en->attack_cooldown=1;
    sound_play(SFX_BOSS_ENRAGE);
}
const char *world_boss_objective_text(void) {
    static char text[64];
    if(g_current_area!=AREA_BLACK_ALTAR || g_campaign==0 || s_boss_summoned)return "";
    if(g_campaign==4) {
        if(!s_boss_objective.ringing)return "";
        snprintf(text,sizeof(text),T(S_BELL_SECONDS),(int)ceilf(s_boss_objective.timer));
    } else snprintf(text,sizeof(text),T(g_campaign==1?S_CORRUPTION_COUNT:S_GUARDIAN_COUNT),s_boss_objective.done,s_boss_objective.count);
    return text;
}

// Cemetery only: its original three capture seals remain unchanged.
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
        if(enemies_spawn(ENEMY_TYPE_BOSS_MALAKOR,s_boss_x,s_boss_z)<0)return;
        s_boss_summoned = true;
        int pack = s_pack_seq++;
        for (int k = 0; k < 6; k++) {         // guarda de honra
            int idx = spawn_pack_member(ENEMY_TYPE_DEMON, s_boss_x, s_boss_z, pack, false, 60.0f, 110.0f);
            if (idx >= 0) enemies_force_chase(idx);
        }
        sound_play(SFX_BOSS_ENRAGE);
        hud_show_banner(campaign_boss_name(g_campaign,false), T(S_BOSS_AWAKENS), 3.5f);
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
    if(authored_altar() || authored_heart() || jarl_castle() || expansion_campaign())return; // local materials own scene lighting
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
    for(int i=0;i<s_cev_count;i++) {
        const ChestEv *e=&s_cev[i];
        if(e->type!=CE_MINI || e->state!=1 || e->carrier<0)continue;
        const Enemy *en=&enemies_get_array()[e->carrier];
        if(!en->active || en->pack!=e->pack || en->hp<=0)continue;
        int w=150,bx=(SCREEN_WIDTH-w)/2,by=28;
        rdpq_set_mode_standard();rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(16,8,18,255));rdpq_fill_rectangle(bx-1,by-1,bx+w+1,by+7);
        rdpq_set_prim_color(RGBA32(170,85,130,255));rdpq_fill_rectangle(bx,by,bx+(int)(w*(float)en->hp/en->max_hp),by+6);
        return;
    }
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

// A thin world-space boundary shares its exact radius with the objective logic.
// Keep the small painted ritual at the center; no enlarged textures or new assets.
static void draw_event_ring(T3DViewport *vp,float x,float z,float radius,color_t color) {
    if(vec2_dist_sq(g_druid.x,g_druid.z,x,z)>320.0f*320.0f)return;
    const int segments=24;
    float y=cemetery_stage_ground_y(x,z,0)+2.0f;
    rdpq_set_prim_color(color);
    for(int k=0;k<segments;k++) {
        float a=k*6.2831853f/segments,b=(k+1)*6.2831853f/segments;
        float ca=cosf(a),sa=sinf(a),cb=cosf(b),sb=sinf(b);
        float inner=fmaxf(1,radius-4.0f),outer=radius;
        T3DVec3 world[4]={{{x+ca*inner,y,z+sa*inner}},{{x+ca*outer,y,z+sa*outer}},
                         {{x+cb*outer,y,z+sb*outer}},{{x+cb*inner,y,z+sb*inner}}};
        float verts[4][3];bool visible=true;
        for(int i=0;i<4;i++) {
            T3DVec3 p;t3d_viewport_calc_viewspace_pos(vp,&p,&world[i]);
            verts[i][0]=p.v[0];verts[i][1]=p.v[1];
            if(p.v[2]<-1 || p.v[2]>1)visible=false;
            verts[i][2]=clampf(p.v[2]*.5f+.5f-1.0f/32768.0f,0,1);
        }
        if(!visible)continue;
        rdpq_triangle(&TRIFMT_ZBUF,verts[0],verts[1],verts[2]);
        rdpq_triangle(&TRIFMT_ZBUF,verts[0],verts[2],verts[3]);
    }
}
void world_draw_event_zones(T3DViewport *vp) {
    rdpq_mode_push();rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_zbuf(true,false);rdpq_mode_zoverride(false,0,0);
    for(int i=0;i<s_cev_count;i++)if(s_cev[i].type==CE_VIGIL && s_cev[i].state<3)
        draw_event_ring(vp,s_cev[i].x,s_cev[i].z,EVENT_HOLD_RADIUS,RGBA32(150,112,48,255));
    for(int i=0;i<s_ev_count;i++)if(s_ev[i].state<3 &&
        (s_ev[i].type==EV_SHRINE || (s_ev[i].type==EV_CAGE && s_ev[i].state==2)))
        draw_event_ring(vp,s_ev[i].x,s_ev[i].z,EVENT_HOLD_RADIUS,s_ev[i].type==EV_SHRINE?
                           RGBA32(89,130,61,255):RGBA32(150,112,48,255));
    if(!s_boss_summoned && g_campaign==0)for(int i=0;i<s_seal_count;i++)if(!s_seals[i].done)
        draw_event_ring(vp,s_seals[i].x,s_seals[i].z,EVENT_HOLD_RADIUS,RGBA32(150,65,50,255));
    if(!s_boss_summoned && g_current_area==AREA_BLACK_ALTAR) {
        if(g_campaign==1)for(int i=0;i<s_boss_objective.count;i++)if(!s_boss_objective.target[i].done)
            draw_event_ring(vp,s_boss_objective.target[i].x,s_boss_objective.target[i].z,22,RGBA32(130,95,45,255));
        if(g_campaign==2 || g_campaign==3)for(int i=0;i<s_boss_objective.count;i++) {
            const BossTarget *t=&s_boss_objective.target[i];
            if(t->done || t->index<0)continue;
            const Enemy *en=&enemies_get_array()[t->index];
            if(en->active && en->pack==t->pack)draw_event_ring(vp,en->x,en->z,22,RGBA32(160,100,45,255));
        }
        if(g_campaign==4 && !s_boss_objective.ringing && !s_boss_objective.ready)
            draw_event_ring(vp,s_boss_objective.bell_x,s_boss_objective.bell_z,25,RGBA32(150,112,48,255));
    }
    for(int i=0;i<s_cev_count;i++) {
        ChestEv *e=&s_cev[i];if(e->type<CE_BREAK || e->state>=3)continue;
        draw_event_ring(vp,e->x,e->z,24,RGBA32(150,112,48,255));
        for(int k=0;k<6;k++)if(e->nodes[k].active)
            draw_event_ring(vp,e->nodes[k].x,e->nodes[k].z,13,
                e->type==CE_SOULS?RGBA32(100,190,160,255):e->type==CE_COURIER?
                RGBA32(220,175,65,255):RGBA32(160,65,110,255));
        if(e->type==CE_HUNT && e->state==1 && e->carrier>=0) {
            Enemy *en=&enemies_get_array()[e->carrier];
            if(en->active && en->pack==e->pack && en->hp>0)
                draw_event_ring(vp,en->x,en->z,22,RGBA32(190,65,50,255));
        }
    }
    rdpq_mode_pop();
}

void world_draw_objectives(void) {
    draw_event_bars();
    if (g_current_area != AREA_BLACK_ALTAR || g_campaign!=0 || s_boss_summoned || s_seal_count == 0) return;
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
    if (g_current_area == AREA_BLACK_ALTAR) {
        if(g_campaign==0)seals_update(dt);else boss_objective_update(dt);
        hazards_update();
    }
    events_update(dt);
    chest_events_update(dt);

    // Check if exit should be unlocked
    if (!ad->exit_unlocked) {
        if (g_current_area == AREA_FORSAKEN_PATH || g_current_area == AREA_HOLLOW_CRYPT) {
            bool open_gate = VEIL_LEVEL_REVIEW || ((s_elites_total > 0) ? (enemies_count_elites() == 0 && elites_pending() == 0)
                                                   : (enemies_get_active_count() <= 2));
            if (open_gate) world_unlock_exit();
        }
    }
}

void world_unlock_exit(void) {
    AreaData *ad = &g_area_data[g_current_area];
    if (!ad->exit_unlocked) {
        ad->exit_unlocked = true;
        if (g_current_area == AREA_HOLLOW_CRYPT || g_current_area == AREA_FORSAKEN_PATH)   // arco de pedra com o vortice (pintado) no lugar do portal 3D
            add_scene_prop(authored_crypt()?SCENE_PORTAL_CRYPT:SCENE_CAVE_PORTAL, ad->exit_x, ad->exit_z, 80.0f, 0.0f);
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
        if(d2<=0.0001f) {
            // Coincident centers are occupied too. This also fixes placement
            // queries that previously accepted a prop at a tree's exact center.
            *x=c->x+min_d;return true;
        }
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
            if (!tile_blocked(s_grid[tr][tc])) continue;
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



void world_draw(void) {
    assertf(modular_stage(),"Stage has no authored renderer");
    if(authored_altar())for(int i=0;i<s_scene_prop_count;i++)
        if(s_scene_props[i].ground_handle>0)altar_stage_paint_set(s_scene_props[i].ground_handle-1,altar_paint_type(s_scene_props[i].id));
    cemetery_stage_draw(g_druid.x,g_druid.z);
}
