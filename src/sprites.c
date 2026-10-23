#include "sprites.h"
#include "sprite_norm.h"
#include "anim_frames.h"
#include "run_video_art.h"
#include "idle_video_art.h"
#include "druid.h"
#include "enemies.h"
#include "spells.h"
#include "items.h"
#include "world.h"
#include "huntress.h"
#include "barbarian.h"
#include "barbarian_attack.h"
#include "barbarian_combo_art.h"
#include "barbarian_warcry_art.h"
#include "barbarian_skill_art.h"
#include "barbarian_skills.h"
#include "cards.h"
#include <stdint.h>

#define CACHE_BASE 192       // quadros na memoria (console de 4 MB)
#define CACHE_EXPANDED 384   // com Expansion Pak (8 MB): o dobro, sem recarregar da ROM no meio da horda
#define CACHE_MAX CACHE_EXPANDED
#define DRAW_MAX 320
#define VISION_ENABLED  1   // visao limitada ao redor do druida + nevoa que deriva (estilo Diablo/Silent Hill)
#define VISION_DIAMETER 440.0f   // px: diametro da escuridao; a area clara e ~30% disso (menor = visao mais curta)
#define DRUID_HEIGHT 38.0f   // altura do druida em unidades de mundo (inimigos comuns = 34; guardioes = 34 x 1.3)
#define DRUID_ROLL_SCALE 0.82f // rolagem/esquiva do druida (os quadros pareciam grandes)
#define DRUID_IDLE_FPS   12.0f   // idle do Druida: 12 quadros de video a 12 fps (Sprites/Druida_Video_12_Frames)
#define DRUID_BASE_BOOST 1.18f  // idle, diagonais e demais poses: mesma escala aparente da corrida vertical
#define DRUID_SIDE_BOOST 1.20f  // corrida lateral alinhada com as corridas verticais
#define DRUID_FRONT_BOOST 1.20f  // as corridas verticais tem corpo mais estreito que a corrida lateral
#define DRUID_BACK_BOOST 1.20f
#define DRUID_BACK_WIDTH_BOOST RUN_BACK_WIDTH // a capa vista de costas e ainda mais estreita
#define BEAST_HEIGHT 44.0f   // altura do lobisomem
#define FOG_PUFFS_ENABLED 0   // as manchas claras de nevoa que derivam (desligadas); a escuridao das bordas fica
#define SHADOWS_ENABLED 0   // sombras desligadas: faziam os sprites parecerem flutuar
#define FOOT_LIFT 2.0f   // o chao em camadas fica 1-4 unidades acima da base: ancora os pes no meio disso
typedef enum {
    DR_IDLE, DR_WALK, DR_WALK_ALT, DR_RUN, DR_RUN_ALT, DR_ATTACK, DR_CAST, DR_ESCAPE, DR_HURT, DR_DEATH, DR_TRANSFORM,
    BE_IDLE, BE_WALK, BE_RUN, BE_ATTACK, BE_CAST, BE_ESCAPE, BE_HURT, BE_DEATH,
    SK_IDLE, SK_WALK, SK_ATTACK, SK_HURT,
    DE_IDLE, DE_WALK, DE_ATTACK, DE_HURT,
    CU_IDLE, CU_WALK, CU_ATTACK, CU_HURT,
    MA_IDLE, MA_WALK, MA_ATTACK, MA_HURT, MA_DEATH, MA_ENRAGE,
    WO_IDLE, WO_RUN, WO_ATTACK, WO_SUMMON,
    FX_BOLT, FX_THORN, FX_DARK, ITEM, CHEST,
    DR_MOVE_SIDE, DR_MOVE_FRONT, BE_MOVE_SIDE, DR_MOVE_BACK,
    DR_NRUN_S, DR_NRUN_N, DR_NRUN_W, DR_NRUN_SW,   // corrida nova (video, tools/import_druid_run.py): frente, costas, esquerda, diagonal esq-frente
    MH_IDLE, MH_WALK, MH_ATTACK, MH_HURT, MH_DEATH, MH_ENRAGE,   // Malakor (prancheta nova, fase 1)
    VK_IDLE, VK_WALK, VK_ATTACK, VK_HURT, VK_DEATH, VK_ENRAGE,   // Varkhul (Malakor transformado, fase 2)
    ESSENCE,                                                     // essencia demoniaca (drop)
    HU_IDLE, HU_RUN, HU_ATTACK, HU_HURT, HU_DEATH, HU_ROLL,      // Cacadora (Sprites/DLC, tools/import_huntress.py)
    HAWK, HU_FOCUS, HU_ROOTS, HU_CAMO, ARROW,                    // gaviao, efeitos e flecha (8 direcoes no quadro)
    SY_IDLE, SY_WALK, SY_ATTACK, SY_HURT, SY_DEATH, SY_ENRAGE,   // Sylvara (chefe da Floresta Morta, fase 1)
    RM_IDLE, RM_WALK, RM_ATTACK, RM_HURT, RM_DEATH, RM_ENRAGE,   // Mae-Raiz (fase 2)
    JA_IDLE, JA_WALK, JA_ATTACK, JA_HURT, JA_DEATH, JA_ENRAGE,   // Jarl Hrimgar (chefe do Norte, fase 1)
    DG_IDLE, DG_WALK, DG_ATTACK, DG_HURT, DG_DEATH, DG_ENRAGE,   // Draugr de Gelo (fase 2)
    BA_IDLE, BA_RUN, BA_ATTACK, BA_ATTACK2, BA_SPIN, BA_SHOUT, BA_HURT, BA_DEATH, BA_ROLL,   // Barbaro (Sprites/Barbaro)
    BA_FX_SHOUT, BA_FX_IMPACT,                                   // onda do grito e impacto do salto
    BA_RUN8,                                                     // corrida nova do Barbaro (8 direcoes completas, sem flip)
    BA_COMBO1, BA_COMBO2, BA_COMBO3,                              // tres golpes, quatro direcoes completas, sem flip
    BA_WARCRY,                                                  // grito/berserker: S, E, N, W
    BA_LEAP, BA_WHIRLWIND,                                     // salto4 direcoes e giro360 aprovados
    DR_RUN8,                                                   // corrida aprovada: S, SE, E, NE, N, NW, W, SW
    ANIM_COUNT
} Anim;

static const char *names[ANIM_COUNT] = {
    "mage_idle","mage_walk","mage_walk_alt","mage_run","mage_run_alt","mage_attack","mage_cast","mage_escape",
    "mage_hurt","mage_death","mage_transform",
    "werewolf_idle","werewolf_walk","werewolf_run","werewolf_attack","werewolf_cast",
    "werewolf_escape","werewolf_hurt","werewolf_death",
    "skeleton_idle","skeleton_walk","skeleton_attack","skeleton_hurt",
    "demon_idle","demon_walk","demon_attack","demon_hurt",
    "cultist_idle","cultist_walk","cultist_attack","cultist_hurt",
    "malakor_idle","malakor_walk","malakor_attack","malakor_hurt","malakor_death","malakor_enrage",
    "wolf_idle","wolf_run","wolf_attack","wolf_summon",
    "spirit_bolt","thorn_burst","dark_orb","item","chest",
    "mage_run_left","mage_run_front","werewolf_run_left","mage_run_back",
    "mage_nrun_s","mage_nrun_n","mage_nrun_w","mage_nrun_sw",
    "malakor_hd_idle","malakor_hd_walk","malakor_hd_attack","malakor_hd_hurt","malakor_hd_death","malakor_hd_enrage",
    "varkhul_idle","varkhul_walk","varkhul_attack","varkhul_hurt","varkhul_death","varkhul_enrage",
    "essence",
    "hunt_idle","hunt_run","hunt_attack","hunt_hurt","hunt_death","hunt_roll",
    "hawk","hunt_focus","hunt_roots","hunt_camo","arrow",
    "sylvara_idle","sylvara_walk","sylvara_attack","sylvara_hurt","sylvara_death","sylvara_enrage",
    "rootmother_idle","rootmother_walk","rootmother_attack","rootmother_hurt","rootmother_death","rootmother_enrage",
    "jarl_idle","jarl_walk","jarl_attack","jarl_hurt","jarl_death","jarl_enrage",
    "draugr_idle","draugr_walk","draugr_attack","draugr_hurt","draugr_death","draugr_enrage",
    "barb_idle","barb_run","barb_attack","barb_attack2","barb_spin","barb_shout","barb_hurt","barb_death","barb_roll",
    "barb_fx_shout","barb_fx_impact",
    "barb_run8",
    "barb_combo1","barb_combo2","barb_combo3","barb_warcry","barb_leap","barb_whirlwind",
    "mage_run8"
};
// A arte do chefe da floresta ja foi importada? (senao usa o Malakor/Varkhul pintados de musgo)
static bool forest_boss_art(void) {
    static int has = -1;
    if (has < 0) { FILE *f = fopen("rom:/spr_sylvara_idle_d0_f0.rgba16.sprite", "r"); has = f != NULL; if (f) fclose(f); }
    return has == 1;
}
static bool north_boss_art(void) {
    static int has = -1;
    if (has < 0) { FILE *f = fopen("rom:/spr_jarl_idle_d0_f0.rgba16.sprite", "r"); has = f != NULL; if (f) fclose(f); }
    return has == 1;
}
// Arte propria do chefe da campanha atual (senao: Malakor/Varkhul com a tinta da campanha)
static bool campaign_boss_art(void) { return (g_campaign == 1 && forest_boss_art()) || (g_campaign == 2 && north_boss_art()); }
static const char *ui_names[SPR_UI_COUNT] = {
    "hud_icon_0","hud_icon_1","hud_icon_2","hud_icon_3","hud_icon_4","hud_icon_5",
    "hud_orb_0","hud_orb_1","hud_belt","hud_boss","hud_banner",
    "hud_state_0","hud_state_1","hud_state_2","hud_state_3",
    "hud_hicon_0","hud_hicon_1","hud_hicon_2","hud_hicon_3","hud_hicon_4",
    "hud_bicon_0","hud_bicon_1","hud_bicon_2","hud_bicon_3","hud_bicon_4"
};
typedef struct { int key; uint32_t used; sprite_t *sprite; uint32_t bytes; } Cache;
// occl: 0 = sem oclusao, 1 = some atras de muros, 2 = vira silhueta atras de muros (druida/lobos)
typedef struct { Anim anim; int dir, frame; bool flip; float x,y,scale,order; sprite_t *direct; bool centered; uint32_t tint;
                 float wx, wz; uint8_t occl; } Draw;
static Cache cache[CACHE_MAX];
static int s_cache_size = CACHE_BASE;
static uint32_t s_cache_bytes, s_cache_budget;
#define CACHE_BUDGET_BASE (512u * 1024u)
#define CACHE_BUDGET_EXPANDED (2048u * 1024u)
// Maior textura dinamica atual:112x112 RGBA32. Reserva inclui metadados e margem.
#define CACHE_LOAD_RESERVE (64u * 1024u)
// Indice direto chave -> vaga (+1). Antes cada sprite procurava a chave nas 192 vagas, todo quadro.
#define KEY_MAX (128 * 8 * 16)
static uint16_t s_key_slot[KEY_MAX];
static uint8_t s_key_missing[KEY_MAX / 8];
_Static_assert(ANIM_COUNT * 8 * 16 <= KEY_MAX, "KEY_MAX pequeno demais");   // arquivo que nao existe: nao tenta abrir de novo a cada quadro
static Draw draws[DRAW_MAX];
static sprite_t *ui[SPR_UI_COUNT];
static sprite_t *scene[SCENE_PROP_COUNT];
static sprite_t *scene_frames[SCENE_PROP_COUNT][3];
static const char *scene_names[SCENE_PROP_COUNT] = {
    "tree_dead", "gravestone", "chapel_arch", "campfire", "traveler_body",
    "crypt_brazier", "cultist_body", "stained_glass", "black_altar",
    "malakor_throne", "pentagram", "monolith", "infernal_brazier",
    "wall_path", "wall_crypt", "wall_altar", "portal_path",
    "portal_crypt", "portal_altar", "crypt_chains", "grave_fence",
    "path_shrine", "crypt_pillar", "sarcophagus", "crypt_altar",
    "great_tree", "grave_cross", "tomb_statue", "crypt_entrance", "dead_bush",
    "bone_pile", "cobweb", "cell_bars", "skull_niche", "wall_chains",
    "decal_blood", "decal_bones", "decal_cracks", "decal_open_grave",
    "stalagmite", "cave_rocks", "crystal",
    "cave_stalag_a", "cave_stalag_b", "cave_stalag_c", "cave_rocks_a", "cave_rocks_b", "cave_rocks_c", "cave_rocks_d", "cave_crystal_a", "cave_crystal_b", "cave_crystal_c", "cave_mush_a", "cave_mush_b", "cave_mush_c", "cave_sarco_a", "cave_sarco_b", "cave_altar", "cave_brazier", "cave_statue_a", "cave_statue_b", "cave_bones_a", "cave_bones_b", "cave_corpse", "cave_ribcage", "cave_decal_blood", "cave_decal_bones", "cave_decal_cracks", "cave_decal_grave", "cave_decal_water", "cave_decal_web", "cave_portal", "cave_entrance", "cave_throne",
    "hell_seal_on", "hell_seal_off", "hell_rocks_a", "hell_rocks_b", "hell_rocks_c", "hell_rocks_d", "hell_spikes_a", "hell_spikes_b", "hell_spikes_c", "hell_firevent", "hell_brazier", "hell_hanged", "hell_skulls", "hell_cage", "hell_statue", "hell_throne", "hell_decal_lava", "hell_decal_crack", "hell_decal_ritual", "hell_decal_bones", "hell_decal_ash", "hell_decal_blood", "hell_lava_pool", "hell_portal", "vendor",
    "swamp_tree_a", "swamp_tree_b", "swamp_tree_c", "swamp_reeds_a", "swamp_reeds_b", "swamp_shroom", "swamp_log", "swamp_boat", "swamp_totem", "swamp_hut", "swamp_bones", "swamp_lantern", "swamp_decal_pool", "swamp_decal_pool_s", "swamp_decal_mud", "swamp_decal_lily", "swamp_decal_algae", "swamp_decal_bones", "forest_portal", "heart_totem_on", "heart_totem_off", "heart_roots_a", "heart_roots_b", "heart_roots_c", "heart_thorns_a", "heart_thorns_b", "heart_thorns_c", "heart_fungus", "heart_cocoon", "heart_skulls", "heart_stone", "heart_carcass", "heart_throne", "heart_spore", "heart_decal_poison", "heart_decal_crack", "heart_decal_circle", "heart_decal_leaves", "heart_decal_bones", "heart_decal_sap", "heart_poison_pool",
    "snow_pine_a", "snow_pine_b", "snow_pine_c", "snow_rock_a", "snow_rock_b", "snow_runestone", "snow_cart", "snow_corpse", "snow_banner", "snow_palisade", "snow_bones", "snow_fire", "snow_decal_ice", "snow_decal_drift", "snow_decal_blood", "snow_decal_tracks", "snow_decal_frozen", "snow_decal_bones", "ice_portal", "ice_stalag_a", "ice_stalag_b", "ice_stalag_c", "ice_crystal_a", "ice_crystal_b", "ice_crystal_c", "ice_frozen_warrior", "ice_frozen_wolf", "ice_mammoth", "ice_bones", "ice_brazier", "ice_decal_crack", "ice_decal_frozen", "ice_decal_glow", "ice_decal_bones", "ice_decal_snow", "ice_decal_blood", "frost_seal_on", "frost_seal_off", "frost_spikes_a", "frost_spikes_b", "frost_spikes_c", "frost_rocks_a", "frost_rocks_b", "frost_rocks_c", "frost_vent", "frost_hanged", "frost_skulls", "frost_statue", "frost_cage", "frost_throne", "frost_brazier", "frost_decal_ice", "frost_decal_crack", "frost_decal_circle", "frost_decal_bones", "frost_decal_snow", "frost_decal_blood", "frost_pool"
};
static bool scene_is_animated(ScenePropId id) {
    return id==SCENE_CAMPFIRE || id==SCENE_CRYPT_BRAZIER ||
           id==SCENE_INFERNAL_BRAZIER || id==SCENE_CAVE_BRAZIER || id==SCENE_CAVE_PORTAL ||
           id==SCENE_HELL_FIREVENT || id==SCENE_HELL_BRAZIER || id==SCENE_HELL_PORTAL || id==SCENE_VENDOR ||
           id==SCENE_SWAMP_LANTERN || id==SCENE_FOREST_PORTAL || id==SCENE_HEART_FUNGUS || id==SCENE_HEART_SPORE ||
           id==SCENE_SNOW_FIRE || id==SCENE_ICE_PORTAL || id==SCENE_ICE_BRAZIER || id==SCENE_FROST_VENT || id==SCENE_FROST_BRAZIER ||
           (id>=SCENE_PORTAL_PATH && id<=SCENE_PORTAL_ALTAR);
}
static int draw_count;
static uint32_t frame_number;
static bool ready;

// Escala FIXA para todos os sprites: pixels por unidade de mundo medidos numa camera de
// referencia ja assentada (mesmos valores de main.c). Antes cada tipo de sprite usava uma
// regra diferente (perspectiva ao vivo ou a camera que atrasa ao andar), e por isso eles
// cresciam e encolhiam enquanto o jogador se movia.
static float s_ref_ppu = 1.73f;               // cenario: px por unidade (segmento de 100u, igual ao original)
static T3DViewport s_ref_vp;                   // camera de referencia (jogador parado, camera assentada)
static float s_norm[ANIM_COUNT][4];             // fator de tamanho por (animacao, direcao), de sprite_norm.h
static uint8_t s_next_occl = 0;               // modo de oclusao do proximo emit()
static float s_next_shadow = 0.0f;           // largura da sombra do proximo emit() (0 = sem sombra)
static sprite_t *s_shadow_spr, *s_glow_fire, *s_glow_gold, *s_glow_blue, *s_vision_spr, *s_fog_spr;

static void init_reference_scale(void) {
    T3DViewport *ref_p = &s_ref_vp;
    *ref_p = t3d_viewport_create();
    T3DViewport ref = *ref_p;
    t3d_viewport_set_perspective(&ref, CAM_REF_FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 20.0f, 500.0f);
    T3DVec3 eye = {{0.0f, CAM_TARGET_Y+CAM_REF_HEIGHT, -CAM_REF_BACK}};
    T3DVec3 target = {{0.0f, CAM_TARGET_Y, 0.0f}};
    T3DVec3 up = {{0.0f, 1.0f, 0.0f}};
    t3d_viewport_look_at(&ref,&eye,&target,&up);
    *ref_p = ref;
    T3DVec3 a,b;
    t3d_viewport_calc_viewspace_pos(ref_p,&a,&(T3DVec3){{0.0f,0.0f,0.0f}});
    t3d_viewport_calc_viewspace_pos(ref_p,&b,&(T3DVec3){{0.0f,100.0f,0.0f}});
    float ppu = fabsf(a.v[1]-b.v[1])/100.0f;
    if (ppu > 0.2f && ppu < 4.0f) s_ref_ppu = ppu;
    debugf("[SPRITES] escala fixa: %.3f px/unidade\n", s_ref_ppu);
}

// Altura em pixels de um objeto de 'height' unidades, medida no ponto de referencia (onde o
// jogador fica na tela). Mesma formula de antes, mas sem depender de onde o objeto esta.
static float ref_px_height_raw(float height);
static float ref_px_height(float height) {   // a camera de referencia e fixa: guarda os ultimos resultados
    static float hk[8], hv[8]; static int n = 0, next = 0;
    for (int i = 0; i < n; i++) if (hk[i] == height) return hv[i];
    float v = ref_px_height_raw(height);
    hk[next] = height; hv[next] = v; next = (next + 1) & 7; if (n < 8) n++;
    return v;
}
static float ref_px_height_raw(float height) {
    T3DVec3 a,b;
    t3d_viewport_calc_viewspace_pos(&s_ref_vp,&a,&(T3DVec3){{0.0f,0.0f,0.0f}});
    t3d_viewport_calc_viewspace_pos(&s_ref_vp,&b,&(T3DVec3){{0.0f,height,0.0f}});
    return fabsf(a.v[1]-b.v[1]);
}

static bool scene_is_decal(ScenePropId id) {
    return id==SCENE_PENTAGRAM || (id>=SCENE_DECAL_BLOOD && id<=SCENE_DECAL_GRAVE) ||
           (id>=SCENE_CAVE_DECAL_BLOOD && id<=SCENE_CAVE_DECAL_WEB) ||
           (id>=SCENE_HELL_DECAL_LAVA && id<=SCENE_HELL_LAVA_POOL) ||
           (id>=SCENE_SWAMP_DECAL_POOL && id<=SCENE_SWAMP_DECAL_BONES) ||
           (id>=SCENE_HEART_DECAL_POISON && id<=SCENE_HEART_POISON_POOL) ||
           (id>=SCENE_SNOW_DECAL_ICE && id<=SCENE_SNOW_DECAL_BONES) ||
           (id>=SCENE_ICE_DECAL_CRACK && id<=SCENE_ICE_DECAL_BLOOD) ||
           (id>=SCENE_FROST_DECAL_ICE && id<=SCENE_FROST_POOL);
}
static bool scene_is_fire(ScenePropId id) {
    return id==SCENE_CAMPFIRE || id==SCENE_SNOW_FIRE || id==SCENE_CRYPT_BRAZIER || id==SCENE_INFERNAL_BRAZIER || id==SCENE_CAVE_BRAZIER ||
           id==SCENE_HELL_BRAZIER || id==SCENE_HELL_FIREVENT || id==SCENE_HELL_LAVA_POOL || id==SCENE_HELL_SEAL_ON;
}
// Largura da sombra = altura do prop * fator (0 = sem sombra)
static const float scene_shadow_k[SCENE_PROP_COUNT] = {
    [SCENE_TREE_DEAD]=.36f, [SCENE_GRAVESTONE]=.50f, [SCENE_CRYPT_BRAZIER]=.9f,
    [SCENE_BLACK_ALTAR]=.75f, [SCENE_MALAKOR_THRONE]=.8f, [SCENE_MONOLITH]=.5f,
    [SCENE_INFERNAL_BRAZIER]=.9f, [SCENE_GRAVE_FENCE]=.55f, [SCENE_PATH_SHRINE]=.65f,
    [SCENE_CRYPT_PILLAR]=.34f, [SCENE_SARCOPHAGUS]=.95f, [SCENE_CRYPT_ALTAR]=.85f,
    [SCENE_GREAT_TREE]=.46f, [SCENE_GRAVE_CROSS]=.46f, [SCENE_TOMB_STATUE]=.44f,
    [SCENE_CRYPT_ENTRANCE]=.7f, [SCENE_DEAD_BUSH]=.9f, [SCENE_BONE_PILE]=1.0f,
    [SCENE_STALAGMITE]=.40f, [SCENE_CAVE_ROCKS]=1.0f, [SCENE_CRYSTAL]=.8f,
};
// Largura da poca de luz das fogueiras/braseiros
static float scene_glow_width(ScenePropId id) {
    if (id==SCENE_HELL_LAVA_POOL) return 150.0f;
    if (id==SCENE_HELL_SEAL_ON || id==SCENE_HELL_FIREVENT) return 95.0f;
    return id==SCENE_CAMPFIRE ? 130.0f : id==SCENE_CRYPT_BRAZIER ? 105.0f : 115.0f;
}

// So desenha o que cai na janela visivel do mundo. Sem isso, objetos MUITO atras da camera
// projetam espelhados para dentro da tela, lotam a lista de desenho e o personagem e os
// inimigos (inseridos depois) deixam de ser desenhados.
static bool in_view(float x, float z) {
    return fabsf(x-g_druid.x)<400.0f && z>g_druid.z-190.0f && z<g_druid.z+450.0f;
}
// Pegada real de cada sprite de cenario (as raizes de uma arvore nao ficam no centro da imagem):
// centro horizontal e largura das linhas de baixo com pixels opacos, e quanto vazio ha abaixo.
typedef struct { bool done; float cx, w, drop; } SceneFoot;
static SceneFoot s_foot[SCENE_PROP_COUNT];
static bool foot_pixel_opaque(const uint8_t *row, int x, tex_format_t fmt, const uint16_t *palette) {
    if (fmt == FMT_CI8) return (palette[row[x]] & 1u) != 0;
    if (fmt == FMT_RGBA16) return (((const uint16_t *)row)[x] & 1u) != 0;
    return row[x * 4 + 3] > 96;
}
static void analyze_foot(int id, sprite_t *spr) {
    SceneFoot *ft=&s_foot[id];
    ft->done=true; ft->cx=0.0f; ft->w=(float)spr->width*0.6f; ft->drop=0.0f;
    surface_t t=sprite_get_pixels(spr);
    tex_format_t fmt=surface_get_format(&t);
    const uint16_t *palette=sprite_get_palette(spr);
    if (fmt!=FMT_RGBA16 && fmt!=FMT_RGBA32 && fmt!=FMT_CI8) return;
    if (fmt==FMT_CI8 && !palette) return;
    int w=t.width, h=t.height, bottom=-1;
    for (int y=h-1;y>=0 && bottom<0;y--) {
        for (int x=0;x<w;x++) {
            const uint8_t *row=(const uint8_t*)t.buffer+y*t.stride;
            bool op=foot_pixel_opaque(row,x,fmt,palette);
            if (op) { bottom=y; break; }
        }
    }
    if (bottom<0) return;
    int band=h/8; if (band<3) band=3;
    int minx=w, maxx=-1;
    for (int y=bottom;y>bottom-band && y>=0;y--) {
        const uint8_t *row=(const uint8_t*)t.buffer+y*t.stride;
        for (int x=0;x<w;x++) {
            bool op=foot_pixel_opaque(row,x,fmt,palette);
            if (op) { if (x<minx) minx=x; if (x>maxx) maxx=x; }
        }
    }
    if (maxx<minx) return;
    ft->cx=((float)(minx+maxx+1)*0.5f)-(float)w*0.5f;
    ft->w=(float)(maxx-minx+1);
    ft->drop=(float)(h-1-bottom);
}
static void add_shadow_px(float x, float y, float width_px) {
    if (!SHADOWS_ENABLED || !s_shadow_spr || draw_count>=DRAW_MAX) return;
    float scale = clampf(width_px/(float)s_shadow_spr->width, .1f, 6.0f);
    y -= 0.5f*(float)(s_shadow_spr->height/2)*scale;
    draws[draw_count++]=(Draw){0,0,0,false,x,y,scale,-1.0e8f+y,s_shadow_spr,true,0,0.0f,0.0f,0};
}
static void add_shadow(float x, float y, float width_units) {
    if (!SHADOWS_ENABLED || !s_shadow_spr || draw_count>=DRAW_MAX) return;
    float scale = clampf(width_units*s_ref_ppu/(float)s_shadow_spr->width, .1f, 6.0f);
    y -= 0.5f*(float)(s_shadow_spr->height/2)*scale;   // sobe a sombra: so uma fatia aparece abaixo dos pes
    draws[draw_count++]=(Draw){0,0,0,false,x,y,scale,-1.0e8f+y,s_shadow_spr,true,0,0.0f,0.0f,0};
}
static void add_glow(sprite_t *spr, float x, float y, float width_units) {
    if (!spr || draw_count>=DRAW_MAX) return;
    if (x<-140 || x>SCREEN_WIDTH+140 || y<-100 || y>SCREEN_HEIGHT+100) return;
    float scale = clampf(width_units*s_ref_ppu/(float)spr->width, .1f, 8.0f);
    draws[draw_count++]=(Draw){0,0,0,false,x,y,scale,-9.0e8f+y,spr,true,0,0.0f,0.0f,0};
}

// Animacoes do jogador com versao suave (arquivos spr_<nome>_s_d*_f0..NF_PLAYER-1).
static bool smooth_anim(Anim a) { return (a>=DR_IDLE && a<=BE_DEATH) || a==BE_MOVE_SIDE || (a>=HU_IDLE && a<=HU_ROLL) || (a>=BA_IDLE && a<=BA_ROLL) || a==BA_RUN8 || a==DR_RUN8 || (a>=BA_COMBO1 && a<=BA_WHIRLWIND); }
static int anim_nf(Anim a) {
    if (a==DR_MOVE_SIDE) return NF_DR_MOVE_SIDE;
    if (a==DR_MOVE_FRONT) return NF_DR_MOVE_FRONT;
    if (a==DR_MOVE_BACK) return NF_DR_MOVE_BACK;
    if (a==BE_MOVE_SIDE) return NF_PLAYER;
    if (a==ARROW) return 8;
    return smooth_anim(a)?NF_PLAYER:4;
}
static bool cache_pinned(const Cache *entry) {
    if (g_hero!=HERO_BARBARIAN) return false;
    int a=entry->key/(8*16),d=(entry->key/16)&7;
    if (d!=barb_combo_direction(g_druid.yaw)) return false;
    if (g_druid.state==DRUID_STATE_ATTACK) return a==BA_COMBO1+(g_druid.combo>2?2:g_druid.combo);
    return g_druid.state==DRUID_STATE_CAST && g_druid.warcry_kind && a==BA_WARCRY;
}
static void cache_release(int slot) {
    Cache *c=&cache[slot];
    if (!c->sprite) return;
    if (c->key>=0 && c->key<KEY_MAX) s_key_slot[c->key]=0;
    s_cache_bytes-=c->bytes;
    sprite_free(c->sprite);memset(c,0,sizeof(*c));
}
static int cache_oldest(void) {
    int slot=-1;uint32_t oldest=UINT32_MAX;
    for (int i=0;i<s_cache_size;i++) {
        if (!cache[i].sprite || cache_pinned(&cache[i])) continue;
        // O RDP usa tres buffers: texturas recentes ainda podem estar em uso.
        if (frame_number>cache[i].used+3 && cache[i].used<oldest) {oldest=cache[i].used;slot=i;}
    }
    return slot;
}
static sprite_t *frame_sprite(Anim anim, int dir, int frame) {
    if (dir < 0 || dir >= 8 || frame < 0 || frame >= 16) return NULL;
    int key = (anim * 8 + dir) * 16 + frame;   // ate 8 direcoes e 16 quadros por animacao
    if (key < 0 || key >= KEY_MAX) return NULL;
    if (s_key_slot[key]) {
        Cache *c = &cache[s_key_slot[key] - 1];
        if (c->sprite && c->key == key) { c->used = frame_number; return c->sprite; }
        s_key_slot[key] = 0;
    }
    if (s_key_missing[key >> 3] & (1 << (key & 7))) return NULL;
    int slot=-1;
    for (int i=0;i<s_cache_size;i++) {
        if (!cache[i].sprite) { slot=i; break; }
    }
    // Limitar bytes, nao apenas quantidade. Antes os48 quadros do combo/grito
    // ficavam presos, e sprite_load alocava o novo ANTES de liberar o antigo.
    while (slot<0 || s_cache_bytes+CACHE_LOAD_RESERVE>s_cache_budget) {
        int victim=cache_oldest();
        if (victim<0) return NULL;
        cache_release(victim);if (slot<0) slot=victim;
    }
    char path[96];
    const char *format = (anim>=WO_IDLE && anim<=WO_SUMMON) || anim==BA_LEAP || anim==BA_WHIRLWIND ? "rgba32" : "rgba16";
    const char *suffix=smooth_anim(anim)?"_s":"";
    snprintf(path,sizeof(path),"rom:/spr_%s%s_d%d_f%d.%s.sprite",names[anim],suffix,dir,frame,format);
    sprite_t *result=sprite_load(path);
    if (!result && suffix[0]) {   // sem a versao suave: cai para o quadro original (4 por animacao)
        snprintf(path,sizeof(path),"rom:/spr_%s_d%d_f%d.%s.sprite",names[anim],dir,frame*4/anim_nf(anim),format);
        result=sprite_load(path);
    }
    if (!result) { debugf("[SPRITES] missing %s\n",path); s_key_missing[key >> 3] |= (uint8_t)(1 << (key & 7)); return NULL; }
    unsigned bits=TEX_FORMAT_BITDEPTH(sprite_get_format(result));
    uint32_t bytes=(uint32_t)result->width*result->height*bits/8u+1024u;
    cache[slot]=(Cache){key,frame_number,result,bytes};s_cache_bytes+=bytes;
    s_key_slot[key] = (uint16_t)(slot + 1);
    return result;
}
// Carrega o combo durante a entrada da fase; ao mudar de direcao, completa um
// quadro por atualizacao em repouso, sem bloquear o botao de ataque.
static void warm_barb_combo(bool complete) {
    if (!ready || g_hero!=HERO_BARBARIAN) return;
    int dir=barb_combo_direction(g_druid.target_yaw);
    for (int a=BA_COMBO1;a<=BA_WARCRY;a++) for (int f=0;f<BARB_COMBO_FRAMES;f++) {
        int key=(a*8+dir)*16+f;
        if (s_key_slot[key]) {
            Cache *c=&cache[s_key_slot[key]-1];
            if (c->sprite && c->key==key) continue;
        }
        // Precarregar so quando ha folga; nao expulsar os monstros para reservar
        // uma animacao ociosa, nem renovar artificialmente sua idade no LRU.
        if (s_cache_bytes+CACHE_LOAD_RESERVE>s_cache_budget*3/4) return;
        frame_sprite((Anim)a,dir,f);
        if (!complete) return;
    }
}
// Com Expansion Pak: carrega de uma vez os quadros dos monstros e efeitos da fase (durante o fade),
// assim a horda nao precisa ler sprites da ROM no meio da luta.
void sprites_preload_area(void) {
    if (!ready) return;
    warm_barb_combo(true);
    if (s_cache_size <= CACHE_BASE) return;
    static const Anim pre[] = { SK_IDLE, SK_WALK, SK_ATTACK, SK_HURT, DE_IDLE, DE_WALK, DE_ATTACK, DE_HURT,
                                CU_IDLE, CU_WALK, CU_ATTACK, CU_HURT };
    for (unsigned a = 0; a < sizeof(pre) / sizeof(pre[0]); a++)
        for (int d = 0; d < 4; d++) for (int f = 0; f < 4; f++) frame_sprite(pre[a], d, f);
    for (int f = 0; f < 4; f++) { frame_sprite(FX_BOLT, 0, f); frame_sprite(FX_DARK, 0, f); frame_sprite(ITEM, 0, f); frame_sprite(ESSENCE, 0, f); }
}

void sprites_init(void) {
    memset(cache,0,sizeof(cache)); memset(ui,0,sizeof(ui));
    memset(s_key_slot,0,sizeof(s_key_slot)); memset(s_key_missing,0,sizeof(s_key_missing));
    s_cache_size = is_memory_expanded() ? CACHE_EXPANDED : CACHE_BASE;
    s_cache_budget=is_memory_expanded()?CACHE_BUDGET_EXPANDED:CACHE_BUDGET_BASE;
    s_cache_bytes=0;
    debugf("[SPRITES] cache de %d quadros (%s)\n", s_cache_size, s_cache_size > CACHE_BASE ? "Expansion Pak" : "4 MB");
    debugf("[SPRITES] limite de texturas: %lu KiB\n",(unsigned long)(s_cache_budget/1024));
    frame_number=1;
    init_reference_scale();
    for (int a=0;a<ANIM_COUNT;a++) for (int d=0;d<4;d++) s_norm[a][d]=1.0f;
    for (unsigned n=0;n<sizeof(SPRITE_NORM)/sizeof(SPRITE_NORM[0]);n++)
        for (int a=0;a<ANIM_COUNT;a++)
            if (!strcmp(names[a],SPRITE_NORM[n].name))
                for (int d=0;d<4;d++) s_norm[a][d]=SPRITE_NORM[n].s[d];
    for (int d=0;d<4;d++) s_norm[DR_IDLE][d]=1.0f;   // idle novo (4 direcoes): ja sai normalizado por tools/import_druid_idle12.py
    for (int a=DR_IDLE;a<=DR_TRANSFORM;a++)
        for (int d=0;d<4;d++) s_norm[a][d]*=DRUID_BASE_BOOST;
    for (int d=0;d<4;d++) s_norm[DR_IDLE][d]=1.0f; // video idle already shares the run body size
    for (int d=0;d<4;d++) s_norm[DR_ESCAPE][d]*=DRUID_ROLL_SCALE;
    s_norm[DR_NRUN_S][0]=s_norm[DR_NRUN_N][0]=s_norm[DR_NRUN_W][0]=s_norm[DR_NRUN_SW][0]=DRUID_BASE_BOOST;   // corrida nova: ja normalizada pelo importador
    s_norm[DR_MOVE_BACK][0]*=DRUID_BACK_BOOST*RUN_SCALE_BACK;
    s_norm[DR_MOVE_FRONT][0]*=DRUID_FRONT_BOOST*RUN_SCALE_FRONT;
    s_norm[DR_MOVE_SIDE][0]*=DRUID_SIDE_BOOST*RUN_SCALE_SIDE;
    s_shadow_spr=sprite_load("rom:/scene_shadow.rgba32.sprite");
    s_glow_fire=sprite_load("rom:/scene_glow_fire.rgba32.sprite");
    s_glow_gold=sprite_load("rom:/scene_glow_gold.rgba32.sprite");
    s_glow_blue=sprite_load("rom:/scene_glow_blue.rgba32.sprite");
    s_vision_spr=sprite_load("rom:/scene_vision.rgba32.sprite");
    s_fog_spr=sprite_load("rom:/scene_fog_puff.rgba32.sprite");
    ready=frame_sprite(DR_IDLE,0,0)!=NULL;
    debugf("[SPRITES] %s\n",ready?"ready":"3D fallback");
}
bool sprites_druid_ready(void) { return ready; }

static int facing(float yaw, bool *flip) {
    float dx=cosf(yaw), dz=sinf(yaw);
    *flip=dx>0.0f;
    if (dz>0.62f) return 3;
    if (dz<-.72f) return 0;
    if (dz<-.12f) return 1;
    return 2;
}
static int loop(float clock, float fps) {
    int n=(int)(clock*fps);
    return (n<0?0:n)%4;
}
// Quadros do ciclo de corrida/andar (0 = nao e ciclo de movimento).
static int move_frames(Anim a) {
    if (a==DR_WALK||a==DR_WALK_ALT||a==DR_RUN||a==DR_RUN_ALT||a==BE_WALK||a==BE_RUN||a==BE_MOVE_SIDE) return NF_PLAYER;
    if (a>=DR_NRUN_S && a<=DR_NRUN_SW) return NF_PLAYER;
    return a==DR_MOVE_SIDE?NF_DR_MOVE_SIDE : a==DR_MOVE_FRONT?NF_DR_MOVE_FRONT :
           a==DR_MOVE_BACK?NF_DR_MOVE_BACK : 0;
}
static int loop_n(float clock, float fps, int n) {
    int k=(int)(clock*fps);
    return (k<0?0:k)%n;
}
// Preserve the holds and cycle duration of each approved 12-frame preview.
// walk_cycle advances eight units per second at full humanoid movement speed.
static int video_run_frame(int hero, int direction, float walk_cycle) {
    float position=fmodf(fmaxf(walk_cycle,0.0f)*125.0f,(float)VIDEO_RUN_CYCLE_MS[hero][direction]);
    for (int f=0;f<NF_PLAYER;f++) {
        if (position<(float)VIDEO_RUN_FRAME_MS[hero][direction][f]) return f;
        position-=(float)VIDEO_RUN_FRAME_MS[hero][direction][f];
    }
    return NF_PLAYER-1;
}
static int video_idle_frame(int hero, int direction, uint32_t clock_ms) {
    unsigned position=clock_ms%VIDEO_IDLE_CYCLE_MS[hero][direction];
    for (int f=0;f<NF_PLAYER;f++) {
        if (position<VIDEO_IDLE_FRAME_MS[hero][direction][f]) return f;
        position-=VIDEO_IDLE_FRAME_MS[hero][direction][f];
    }
    return NF_PLAYER-1;
}
// Apply a sequence-wide size correction around its common boot-floor anchor.
static void video_art_size(int before, float factor, float foot_drop) {
    if (draw_count<=before) return;
    Draw *d=&draws[draw_count-1];
    float previous=d->scale;
    d->scale*=factor;
    d->y+=foot_drop*(d->scale-previous);
}
// Progresso t em [0,1] -> quadro 0..n-1 (animacoes de uma vez)
static int once_frame(float t, int n) {
    int f=(int)(clampf(t,0.0f,1.0f)*(float)n);
    return f>=n?n-1:f;
}
// Tabela de 4 valores (um por quadro original) interpolada para o quadro fr de n.
static float table4(const float *t, int fr, int n, bool loopy) {
    float u=loopy?(float)fr*4.0f/(float)n:(float)fr*3.0f/(float)(n-1);
    int i=(int)u; if (i>3) i=3;
    float f=u-(float)i;
    int j=loopy?(i+1)&3:(i<3?i+1:3);
    return t[i]*(1.0f-f)+t[j]*f;
}
static int walk_phase4(float clock) {
    int phase=(int)clock;
    return (phase<0?0:phase)%4;
}
static void emit(T3DViewport *vp, Anim anim, int dir, int frame, bool flip,
                 float x,float y,float z,float height) {
    float shadow_w=s_next_shadow; s_next_shadow=0.0f;
    uint8_t occl=s_next_occl; s_next_occl=0;
    if (draw_count>=DRAW_MAX-1) return;
    T3DVec3 feet;
    t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{x,y+FOOT_LIFT,z}});
    if (!in_view(x,z)) return;
    if (feet.v[0]<-80 || feet.v[0]>SCREEN_WIDTH+80 ||
        feet.v[1]<-80 || feet.v[1]>SCREEN_HEIGHT+80) return;
    float scale=clampf(ref_px_height(height)/60.0f,.16f,2.2f);
    if (shadow_w>0.0f) add_shadow(feet.v[0],feet.v[1]+1.0f,shadow_w);
    draws[draw_count++]=(Draw){anim,dir,frame,flip,feet.v[0],feet.v[1],scale,feet.v[1],NULL,false,0,x,z,occl};
}
static void add_scenery(T3DViewport *vp) {
    int count;
    const SceneProp *props=world_scene_props(&count);
    float px_per_unit=s_ref_ppu;
    for (int i=0;i<count && draw_count<DRAW_MAX-3;i++) {
        const SceneProp *p=&props[i];
        if (!in_view(p->x,p->z)) continue;
        sprite_t *prop_sprite;
        if (scene_is_animated(p->id)) {
            int frame=(get_ticks_ms()/180)%3;
            if (!scene_frames[p->id][frame]) {
                char path[96];
                snprintf(path,sizeof(path),"rom:/scene_%s_f%d.rgba16.sprite",scene_names[p->id],frame);
                scene_frames[p->id][frame]=sprite_load(path);
            }
            prop_sprite=scene_frames[p->id][frame];
        } else {
            if (!scene[p->id]) {
                char path[96];
                snprintf(path,sizeof(path),"rom:/scene_%s.rgba16.sprite",scene_names[p->id]);
                scene[p->id]=sprite_load(path);
            }
            prop_sprite=scene[p->id];
        }
        if (!prop_sprite) continue;
        bool decal = scene_is_decal(p->id);    // decalque deitado no chao (selo, sangue, ossos...)
        T3DVec3 feet;
        t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{p->x,decal?0.0f:FOOT_LIFT,p->z}});
        if (feet.v[0]<-140 || feet.v[0]>SCREEN_WIDTH+140 ||
            feet.v[1]<-140 || feet.v[1]>SCREEN_HEIGHT+140) continue;
        float scale = decal
            ? clampf((p->height*px_per_unit)/(float)prop_sprite->width,  .16f, 8.0f)  // height=diametro
            : clampf((p->height*px_per_unit)/(float)prop_sprite->height, .16f, 3.0f);
        if (p->id==SCENE_GRAVE_FENCE) {
            // A cerca precisa fechar exatamente de um tile ao outro: usa a largura REAL do tile
            // na tela, naquele ponto, em vez da escala de altura (que e maior que a horizontal).
            T3DVec3 l,r;
            t3d_viewport_calc_viewspace_pos(vp,&l,&(T3DVec3){{p->x-36.0f,FOOT_LIFT,p->z}});
            t3d_viewport_calc_viewspace_pos(vp,&r,&(T3DVec3){{p->x+37.0f,FOOT_LIFT,p->z}});
            float wpx=fabsf(r.v[0]-l.v[0]);
            if (wpx>8.0f) scale=clampf(wpx/(float)prop_sprite->width,.16f,3.0f);
        }
        if (!s_foot[p->id].done) analyze_foot(p->id,prop_sprite);
        const SceneFoot *ft=&s_foot[p->id];
        float sy=decal?feet.v[1]:feet.v[1]+ft->drop*scale;                  // a parte opaca de baixo encosta no chao
        if (!decal && scene_shadow_k[p->id]>0.0f) {
            float sw=clampf(ft->w*scale*1.2f,12.0f,170.0f);  // sombra do tamanho da pegada real
            add_shadow_px(feet.v[0]+ft->cx*scale,sy+1.0f,sw);
        }
        if (scene_is_fire(p->id) && s_glow_fire) {
            // Poca de luz que pulsa de leve, sob o fogo.
            float flick=1.0f+0.06f*sinf((float)get_ticks_ms()*0.011f+p->x);
            add_glow(s_glow_fire,feet.v[0],feet.v[1]-3.0f,scene_glow_width(p->id)*flick);
        }
        if ((p->id==SCENE_CRYSTAL || (p->id>=SCENE_CAVE_CRYSTAL_A && p->id<=SCENE_CAVE_CRYSTAL_C) ||
             p->id==SCENE_CAVE_PORTAL || p->id==SCENE_SWAMP_LANTERN || p->id==SCENE_FOREST_PORTAL ||
             p->id==SCENE_HEART_FUNGUS || p->id==SCENE_SWAMP_SHROOM || p->id==SCENE_ICE_PORTAL || p->id==SCENE_ICE_BRAZIER ||
             p->id==SCENE_FROST_BRAZIER || (p->id>=SCENE_ICE_CRYSTAL_A && p->id<=SCENE_ICE_CRYSTAL_C)) && s_glow_blue) {   // luz fria: fogo-fatuo, fungos
            float pl=1.0f+0.10f*sinf((float)get_ticks_ms()*0.006f+p->x);
            add_glow(s_glow_blue,feet.v[0],feet.v[1]-3.0f,62.0f*pl);
        }
        draws[draw_count++]=(Draw){0,0,0,false,feet.v[0],sy,scale,
                                   decal?-1.0e9f+feet.v[1]*0.001f:feet.v[1],prop_sprite,decal,
                                   (scene_is_fire(p->id)||p->id==SCENE_VENDOR)?0:world_scene_tint(),
                                   p->x,p->z,(uint8_t)1};   // decalques tambem somem atras dos muros
    }
}
#define HUNT_HEIGHT 44.0f   // pranchetas v2 no formato do druida: mesma escala aparente (38 x 1,18 x norma)
// Cacadora: linhas da prancheta = frente, diagonal de frente, lado, costas.
// O eixo X do mundo e invertido em relacao ao analogico; espelhar cada vista lateral
// pelo lado real do movimento, nao pela regra usada para as pranchetas do druida.
static int hunt_facing(float yaw, bool *flip) {
    float dx=cosf(yaw), dz=sinf(yaw);
    if (dz>0.62f) { *flip=false; return 3; }  // cima: costas
    if (dz<-.72f) { *flip=false; return 0; }  // baixo: frente
    if (dz<-.12f) { *flip=dx<0.0f; return 1; } // diagonal inferior: base olha esquerda
    *flip=dx>0.0f; return 2;                    // lado/superior: base olha direita
}
static void add_huntress(T3DViewport *vp) {
    float speed=sqrtf(g_druid.vx*g_druid.vx+g_druid.vz*g_druid.vz);
    float sprite_yaw=speed>10.0f && g_druid.state!=DRUID_STATE_ATTACK?g_druid.target_yaw:g_druid.yaw;
    bool flip; int dir=hunt_facing(sprite_yaw,&flip);
    // 12 quadros por animacao (4 pintados + intermediarios por fluxo optico, igual ao druida)
    const int N=NF_PLAYER;
    Anim anim=HU_IDLE;
    int fr=loop_n((float)get_ticks_ms()*.001f,5.0f*N/4.0f,N);
    if (g_druid.hp<=0 || g_druid.state==DRUID_STATE_DEAD) {
        anim=HU_DEATH; fr=once_frame(g_druid.state_timer/(.22f*4.0f),N);
    } else if (g_druid.hurt_timer>0) {
        anim=HU_HURT; fr=N/4;
    } else if (g_druid.state==DRUID_STATE_DODGE) {
        anim=HU_ROLL; fr=once_frame(1.0f-g_druid.dodge_timer/.30f,N);
    } else if (g_druid.state==DRUID_STATE_ATTACK) {
        anim=HU_ATTACK;
        float duration=g_druid.attack_duration>0?g_druid.attack_duration:.16f;
        fr=N/2+once_frame(1.0f-g_druid.attack_timer/duration,N/2); // disparo imediato, retorno curto
    } else if (g_druid.state==DRUID_STATE_CAST) {
        anim=HU_ATTACK; fr=N-1;
    } else if (speed>10.0f) {
        anim=HU_RUN;
        fr=((int)(g_druid.walk_cycle*(float)N*0.25f))%N; if(fr<0)fr=0;
    }
    int before=draw_count;
    s_next_shadow=18.0f;
    s_next_occl=2;
    emit(vp,anim,dir,fr,flip,g_druid.x,g_druid.y,g_druid.z,HUNT_HEIGHT);
    if (draw_count>before && anim==HU_RUN) {
        float hop=fabsf(sinf(g_druid.walk_cycle*1.5707963f));
        draws[draw_count-1].y-=hop*2.5f;
    }
    if (draw_count>before) {
        static int last_hp=-1; static uint32_t flash_until=0;
        uint32_t now=get_ticks_ms();
        if (last_hp>=0 && g_druid.hp<last_hp && g_druid.hp>0) flash_until=now+450;
        last_hp=g_druid.hp;
        if (now<flash_until && ((now/70)&1)==0) draws[draw_count-1].tint=0xFF4838FFu;
        else if (g_druid.camo_timer>0.0f) draws[draw_count-1].tint=((now/90)&1)?0x5A7850FFu:0x48644AFFu;   // camuflada: escura, cor de folha
        else if (g_druid.focus_timer>0.0f && ((now/160)&1)) draws[draw_count-1].tint=0xE6FFC8FFu;
    }
    // efeitos sobre ela: retaculo do foco acima da cabeca e o manto de folhas ao ativar
    if (g_druid.focus_timer>0.0f) {
        int b2=draw_count;
        emit(vp,HU_FOCUS,0,loop_n((float)get_ticks_ms()*.001f,6.0f,4),false,g_druid.x,g_druid.y+46.0f,g_druid.z,16);
        if (draw_count>b2) { draws[draw_count-1].order+=0.5f; }
    }
    if (g_druid.camo_timer>hunt_camo_time()-0.6f) {
        int b3=draw_count;
        float t=hunt_camo_time()-g_druid.camo_timer;
        emit(vp,HU_CAMO,0,once_frame(t/0.6f,4),false,g_druid.x,g_druid.y,g_druid.z-1.0f,44);
        if (draw_count>b3) draws[draw_count-1].order+=0.5f;
    }
}
// Arte do Barbaro ja importada? (senao ele aparece como o druida pintado de vermelho)
static bool barb_art(void) {
    static int has = -1;
    if (has < 0) { FILE *f = fopen("rom:/spr_barb_idle_s_d0_f0.rgba16.sprite", "r"); has = f != NULL; if (f) fclose(f); }
    return has == 1;
}
#define BARB_HEIGHT 46.0f   // um pouco maior que o druida (guerreiro enorme)
static void add_barb_fx(T3DViewport *vp) {
    if (g_druid.fx_shout > 0.0f) {
        int b=draw_count;
        emit(vp,BA_FX_SHOUT,0,once_frame(1.0f-g_druid.fx_shout/0.5f,4),false,g_druid.fx_x,0,g_druid.fx_z,
             barb_shout_radius()*1.9f);   // onda do tamanho real da area do grito
        if (draw_count>b) { draws[draw_count-1].centered=true; draws[draw_count-1].order=-1.0e9f+draws[draw_count-1].y*0.001f; }
    }
    if (g_druid.fx_impact > 0.0f) {
        int b=draw_count;
        emit(vp,BA_FX_IMPACT,0,once_frame(1.0f-g_druid.fx_impact/0.5f,4),false,g_druid.fx_x,0,g_druid.fx_z,
             52.0f*(1.0f+g_mods.leap_radius_pct)*1.9f);   // rachadura do tamanho da area do salto
        if (draw_count>b) { draws[draw_count-1].centered=true; draws[draw_count-1].order+=0.5f; }
    }
}
static void barb_finish(int before) {   // berserker: vermelho e maior; esmaecido no salto nao
    if (draw_count<=before) return;
    if (g_druid.berserk_timer>0.0f) {
        Draw *d=&draws[draw_count-1];
        float margin=0.0f;
        if (d->anim>=BA_COMBO1 && d->anim<=BA_COMBO3) margin=BARB_COMBO_TEXTURE_HEIGHT-BARB_COMBO_FOOT_ROW-2;
        if (d->anim==BA_WARCRY) margin=BARB_WARCRY_TEXTURE_HEIGHT-BARB_WARCRY_FOOT_ROW-2;
        if (d->anim==BA_LEAP || d->anim==BA_WHIRLWIND) margin=BARB_SKILL_TEXTURE_HEIGHT-BARB_SKILL_FOOT_ROW-2;
        d->y+=margin*d->scale*.10f;
        d->tint=0xFF6A58FFu; d->scale*=1.10f;
    }
}

// Historico em coordenadas do mundo: acompanha a camera e guarda a pose real.
// Seis copias leves, sem ponteiros de textura sujeitos a expulsao do cache.
#define BARB_TRAIL_COUNT 6
#define BARB_TRAIL_LIFE_MS 300
typedef struct { Anim anim; int dir,frame; bool flip; float x,y,z,mult,offset; uint32_t born; } BarbTrail;
static BarbTrail s_barb_trails[BARB_TRAIL_COUNT];
static int s_barb_trail_next;
static uint32_t s_barb_trail_tick, s_barb_trail_sample;
static float s_barb_trail_x,s_barb_trail_z;
static void clear_barb_trails(void) {
    memset(s_barb_trails,0,sizeof(s_barb_trails));
    s_barb_trail_tick=s_barb_trail_sample=0; s_barb_trail_next=0;
}
static void add_barb_trails(T3DViewport *vp, int before) {
    uint32_t now=get_ticks_ms();
    float dx=g_druid.x-s_barb_trail_x,dz=g_druid.z-s_barb_trail_z;
    if (!s_barb_trail_tick || now-s_barb_trail_tick>250 || dx*dx+dz*dz>3600.0f || g_druid.hp<=0) clear_barb_trails();
    s_barb_trail_tick=now; s_barb_trail_x=g_druid.x; s_barb_trail_z=g_druid.z;
    if (draw_count<=before || g_druid.hp<=0) return;
    Draw actor=draws[draw_count-1];
    float base=clampf(ref_px_height(BARB_HEIGHT)/60.0f,.16f,2.2f);
    bool active=g_druid.rush_timer>0.0f || g_druid.berserk_timer>0.0f;
    if (active && actor.anim==BA_RUN8 && now-s_barb_trail_sample>=45) {
        T3DVec3 feet;
        t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{g_druid.x,g_druid.y+FOOT_LIFT,g_druid.z}});
        s_barb_trails[s_barb_trail_next]=(BarbTrail){actor.anim,actor.dir,actor.frame,actor.flip,
            g_druid.x,g_druid.y,g_druid.z,actor.scale/base,(actor.y-feet.v[1])/actor.scale,now};
        s_barb_trail_next=(s_barb_trail_next+1)%BARB_TRAIL_COUNT;
        s_barb_trail_sample=now;
    }
    for (int i=0;i<BARB_TRAIL_COUNT;i++) {
        BarbTrail *t=&s_barb_trails[i]; uint32_t age=now-t->born;
        if (!t->born || age>=BARB_TRAIL_LIFE_MS) continue;
        // Nao duplica o desenho por cima dele mesmo ao soltar o analogico.
        float x=g_druid.x-t->x,z=g_druid.z-t->z;
        if (x*x+z*z<4.0f) continue;
        int n=draw_count; s_next_occl=1;
        emit(vp,t->anim,t->dir,t->frame,t->flip,t->x,t->y,t->z,BARB_HEIGHT);
        if (draw_count==n) continue;
        Draw *d=&draws[draw_count-1]; d->scale*=t->mult; d->y+=t->offset*d->scale;
        d->order=actor.order-.01f-(float)age*.001f;
        unsigned alpha=(unsigned)(110.0f*(1.0f-(float)age/BARB_TRAIL_LIFE_MS));
        d->tint=0xEF182800u | (alpha?alpha:1u);
    }
}
// Barbaro: setores com as diagonais de 45 graus bem no MEIO de um setor (no facing() do druida a
// diagonal cai exatamente na divisa frente/diagonal e costas/lado, e o sprite ficava piscando entre
// os dois). Diagonal de baixo -> prancheta diagonal; diagonal de cima -> costas. Com histerese.
static int barb_dir_base(float dz) {
    if (dz>.55f) return 3;
    if (dz<-.92f) return 0;
    if (dz<-.38f) return 1;
    return 2;
}
static int barb_dir_of(float dz, float m, int keep) {
    int c=barb_dir_base(dz);
    if (c!=keep && (barb_dir_base(dz+m)==keep || barb_dir_base(dz-m)==keep)) return keep;   // histerese
    return c;
}
static int barb_facing(float yaw, bool *flip) {
    static int last=0; static bool last_flip=false;
    float dx=cosf(yaw), dz=sinf(yaw);
    int d=barb_dir_of(dz,.08f,last);
    last=d;
    if (dx>.12f) last_flip=true; else if (dx<-.12f) last_flip=false;   // de frente/costas puro: mantem o lado
    *flip=last_flip;
    return d;
}
// Direcoes da tela: S, SE, E, NE, N, NW, W, SW. A camera espelha X do mundo.
// Corrida usa oito artes; idle e combo usam S/E/N/W completos, sem flip.
static int barb_facing8(float yaw, int *keep) {
    int last=*keep;
    float angle=atan2f(sinf(yaw),-cosf(yaw));
    float step=0.78539816f;   // 45 graus
    int nearest=((int)floorf((angle+1.5707963f+step*.5f)/step)+8)&7;
    float center=-1.5707963f+(float)last*step;
    float delta=atan2f(sinf(angle-center),cosf(angle-center));
    if (nearest!=last && fabsf(delta)>step*.5f+.06f) last=nearest;
    *keep=last;
    return last;
}
static int barb_idle_facing(float yaw) {
    static int last=0;
    float angle=atan2f(sinf(yaw),-cosf(yaw));
    float step=1.57079633f;
    int nearest=((int)floorf((angle+step*1.5f)/step)+4)&3;
    float center=-step+(float)last*step;
    float delta=atan2f(sinf(angle-center),cosf(angle-center));
    if (nearest!=last && fabsf(delta)>step*.5f+.06f) last=nearest;
    return last*2;   // cardinal indices of the saved eight-direction idle
}
static void add_barbarian(T3DViewport *vp) {
    float speed=sqrtf(g_druid.vx*g_druid.vx+g_druid.vz*g_druid.vz);
    float sprite_yaw=speed>10.0f && g_druid.state!=DRUID_STATE_ATTACK?g_druid.target_yaw:g_druid.yaw;
    bool flip; int dir=barb_facing(sprite_yaw,&flip);
    const int N=NF_PLAYER;
    Anim anim=BA_IDLE;
    if (g_druid.state!=DRUID_STATE_ATTACK && g_druid.state!=DRUID_STATE_CAST && g_druid.leap_timer<=0.0f && g_druid.spin_timer<=0.0f) warm_barb_combo(false);
    int fr=loop_n((float)get_ticks_ms()*.001f,10.0f,N);
    if (g_druid.hp<=0 || g_druid.state==DRUID_STATE_DEAD) {
        anim=BA_DEATH; fr=once_frame(g_druid.state_timer/(.22f*4.0f),N);
    } else if (g_druid.leap_timer>0.0f) {
        anim=BA_LEAP; dir=barb_combo_direction(g_druid.yaw); flip=false;
        fr=barb_leap_frame(BARB_LEAP_TIME-g_druid.leap_timer);
    } else if (g_druid.spin_timer>0.0f) {
        anim=BA_WHIRLWIND; dir=0; flip=false;
        fr=barb_whirlwind_frame(g_druid.spin_clock);
    } else if (g_druid.state==DRUID_STATE_ATTACK) {
        int step=g_druid.combo>=2?2:g_druid.combo==1?1:0;
        anim=(Anim)(BA_COMBO1+step);
        dir=barb_combo_direction(g_druid.yaw); flip=false;
        float duration=g_druid.attack_duration>0?g_druid.attack_duration:BARB_SWING_TIME;
        fr=barb_combo_frame(1.0f-g_druid.attack_timer/duration,step==2);
    } else if (g_druid.state==DRUID_STATE_CAST && g_druid.warcry_kind) {
        anim=BA_WARCRY; dir=barb_combo_direction(g_druid.yaw); flip=false;
        fr=once_frame(1.0f-g_druid.cast_timer/BARB_WARCRY_TIME,N);
    } else if (g_druid.hurt_timer>0) {
        anim=BA_HURT; fr=N/4;
    } else if (g_druid.state==DRUID_STATE_DODGE) {
        anim=BA_ROLL; fr=once_frame(1.0f-g_druid.dodge_timer/.40f,N);   // salto de esquiva
    } else if (g_druid.state==DRUID_STATE_CAST) {
        anim=BA_SHOUT; fr=once_frame(1.0f-g_druid.cast_timer/.45f,N);
    } else if (speed>10.0f) {
        anim=BA_RUN; fr=((int)(g_druid.walk_cycle*(float)N*0.25f))%N; if(fr<0)fr=0;
        // Oito direcoes completas; NE/NW usam as corridas diagonais aprovadas.
        static int run_last;
        int s8=barb_facing8(sprite_yaw,&run_last);
        anim=BA_RUN8; dir=s8; flip=false;
        fr=video_run_frame(0,dir,g_druid.walk_cycle);
    }
    if (anim==BA_IDLE) {
        dir=barb_idle_facing(g_druid.target_yaw); flip=false;
        fr=video_idle_frame(0,dir/2,get_ticks_ms());
    }
    int before=draw_count;
    s_next_shadow=20.0f;
    s_next_occl=2;
    emit(vp,anim,dir,fr,flip,g_druid.x,g_druid.y,g_druid.z,BARB_HEIGHT);
    if (anim==BA_IDLE) video_art_size(before,VIDEO_IDLE_SCALE[0][dir/2],VIDEO_IDLE_FOOT_DROP[0][dir/2]);
    if (anim==BA_RUN8) video_art_size(before,VIDEO_RUN_ART_SCALE[0][dir],VIDEO_RUN_FOOT_DROP[0][dir]);
    if (draw_count>before && anim>=BA_COMBO1 && anim<=BA_COMBO3) {
        Draw *d=&draws[draw_count-1];
        d->scale*=BARB_COMBO_ART_SCALE[anim-BA_COMBO1][dir];
        d->y+=(BARB_COMBO_TEXTURE_HEIGHT-BARB_COMBO_FOOT_ROW-2)*d->scale;
    }
    if (draw_count>before && anim==BA_WARCRY) {
        Draw *d=&draws[draw_count-1];
        d->scale*=BARB_WARCRY_ART_SCALE[dir];
        d->y+=(BARB_WARCRY_TEXTURE_HEIGHT-BARB_WARCRY_FOOT_ROW-2)*d->scale;
    }
    if (draw_count>before && (anim==BA_LEAP || anim==BA_WHIRLWIND)) {
        Draw *d=&draws[draw_count-1];
        d->scale*=anim==BA_LEAP?BARB_LEAP_ART_SCALE[dir]:BARB_WHIRLWIND_ART_SCALE;
        d->y+=(BARB_SKILL_TEXTURE_HEIGHT-BARB_SKILL_FOOT_ROW-2)*d->scale;
    }
    if (draw_count>before && anim==BA_RUN) draws[draw_count-1].y-=fabsf(sinf(g_druid.walk_cycle*1.5707963f))*2.5f;
    if (draw_count>before) {
        static int last_hp=-1; static uint32_t flash_until=0;
        uint32_t now=get_ticks_ms();
        if (last_hp>=0 && g_druid.hp<last_hp && g_druid.hp>0) flash_until=now+450;
        last_hp=g_druid.hp;
        if (now<flash_until && ((now/70)&1)==0) draws[draw_count-1].tint=0xFF4838FFu;
    }
    barb_finish(before);
    add_barb_trails(vp,before);
    add_barb_fx(vp);
}
static void add_druid(T3DViewport *vp) {
    if (g_hero==HERO_HUNTRESS) { add_huntress(vp); return; }
    if (g_hero==HERO_BARBARIAN && barb_art()) { add_barbarian(vp); return; }
    int barb_before=draw_count;   // barbaro sem arte: desenha o druida pintado de vermelho
    static bool running;
    static bool previous_beast;
    float speed=sqrtf(g_druid.vx*g_druid.vx+g_druid.vz*g_druid.vz);
    // Use the intended heading for sprite selection while moving. The
    // smoothly rotating yaw otherwise crosses several direction sectors and
    // swaps whole paintings during a single diagonal step.
    float sprite_yaw=speed>10.0f?g_druid.target_yaw:g_druid.yaw;
    bool flip; int dir=facing(sprite_yaw,&flip);
    bool beast=g_druid.is_beast;
    if (beast!=previous_beast) { running=false; previous_beast=beast; }
    Anim anim=beast?BE_IDLE:DR_IDLE;
    int fr=loop_n((float)get_ticks_ms()*.001f,beast?5.0f*NF_PLAYER/4.0f:DRUID_IDLE_FPS,NF_PLAYER);
    if (g_druid.hp<=0 || g_druid.state==DRUID_STATE_DEAD) {
        anim=beast?BE_DEATH:DR_DEATH;
        fr=once_frame(g_druid.state_timer/(.22f*4.0f),NF_PLAYER);
    } else if (g_druid.hurt_timer>0) {
        anim=beast?BE_HURT:DR_HURT; fr=NF_PLAYER/4;
    } else if (g_druid.state==DRUID_STATE_DODGE) {
        anim=beast?BE_ESCAPE:DR_ESCAPE; fr=once_frame(1.0f-g_druid.dodge_timer/(.075f*4.0f),NF_PLAYER);
    } else if (g_druid.state==DRUID_STATE_ATTACK) {
        anim=beast?BE_ATTACK:DR_ATTACK;
        float duration=g_druid.attack_duration>0?g_druid.attack_duration:.35f;
        fr=once_frame(1.0f-g_druid.attack_timer/duration,NF_PLAYER);
    } else if (g_druid.transform_timer>0) {
        anim=DR_TRANSFORM; fr=once_frame(1.0f-g_druid.transform_timer/(.08f*4.0f),NF_PLAYER);
    } else if (g_druid.state==DRUID_STATE_CAST) {
        anim=beast?BE_CAST:DR_CAST; fr=once_frame(1.0f-g_druid.cast_timer/(.08f*4.0f),NF_PLAYER);
    } else if (speed>10.0f) {
        int phase=walk_phase4(g_druid.walk_cycle);
        float max_speed=beast?110.0f:75.0f;
        // Hysteresis avoids flickering between different silhouettes while
        // the player nudges the analog stick near the run threshold.
        if (running ? speed<max_speed*.55f : speed>max_speed*.82f)
            running=!running;
        if (beast) {
            // Never interleave walk and run paintings: their posture and
            // canvas size differ, which makes the profile jump every frame.
            // De lado: folha de corrida propria (4 quadros) pintada virada para a ESQUERDA da tela;
            // para a direita o quadro e espelhado.
            static bool beast_side=true;
            float bs=fabsf(sinf(sprite_yaw));
            if (beast_side && bs>.62f) beast_side=false;
            else if (!beast_side && bs<.5f) beast_side=true;
            bool horizontal=beast_side;
            if (horizontal) {
                anim=BE_MOVE_SIDE; dir=0;
                fr=phase;
                flip=cosf(sprite_yaw)<0.0f;
            } else {
                anim=running?BE_RUN:BE_WALK;
                fr=phase;
            }
        } else {
            static int run_last;
            anim=DR_RUN8;
            dir=barb_facing8(sprite_yaw,&run_last);
            flip=false;
            fr=video_run_frame(1,dir,g_druid.walk_cycle);
        }
    }
    int nfm=move_frames(anim);
    if (nfm>0) {   // o ciclo (4 unidades de walk_cycle) e dividido em nfm quadros
        fr=(int)(g_druid.walk_cycle*(float)nfm*0.25f);
        fr=(fr<0?0:fr)%nfm;
    }
    if(fr<0)fr=0;
    if(nfm==0 && fr>anim_nf(anim)-1)fr=anim_nf(anim)-1;
    // Idle do Druida: arte nova de 4 direcoes (dir 0 = S/frente, 1 = W/esquerda da tela, 2 = N/costas,
    // 3 = E/direita da tela), NUNCA espelhada, sem diagonais. Segue a direcao pretendida (target_yaw),
    // nao o yaw suavizado (que gira de um lado a outro), e usa histerese para nao piscar.
    // Nesta camera a tela e espelhada em X: direita da tela = -x do mundo (ver druid.c).
    if (anim==DR_IDLE) {
        static int idle4;
        float th=atan2f(sinf(g_druid.target_yaw),-cosf(g_druid.target_yaw));   // 0 = direita da tela, -90 graus = para a camera
        float u=(-1.5707963f-th)/1.5707963f;                                    // S=0, W=1, N=2, E=3
        float diff=u-(float)idle4;
        while (diff>=2.0f) diff-=4.0f;
        while (diff<-2.0f) diff+=4.0f;
        if (diff>0.62f || diff<-0.62f) idle4=(int)floorf(u+0.5f)&3;
        dir=idle4; flip=false;
        int cardinal=dir==1?3:dir==3?1:dir; // saved idle table is S,E,N,W
        fr=video_idle_frame(1,cardinal,get_ticks_ms());
    }
    int before=draw_count;
    s_next_shadow=beast?21.0f:18.0f;
    s_next_occl=2;
    // Native run pixels share the barbarian's size calibration.
    float height=beast?BEAST_HEIGHT:(anim==DR_RUN8||anim==DR_IDLE)?BARB_HEIGHT:DRUID_HEIGHT;
    emit(vp,anim,dir,fr,flip,g_druid.x,g_druid.y,g_druid.z,height);
    if (anim==DR_IDLE) {
        int cardinal=dir==1?3:dir==3?1:dir;
        video_art_size(before,VIDEO_IDLE_SCALE[1][cardinal],VIDEO_IDLE_FOOT_DROP[1][cardinal]);
    }
    if (anim==DR_RUN8) video_art_size(before,VIDEO_RUN_ART_SCALE[1][dir],VIDEO_RUN_FOOT_DROP[1][dir]);
    if (draw_count>before && speed>10.0f &&
        (anim==DR_WALK||anim==DR_WALK_ALT||anim==DR_RUN||anim==DR_RUN_ALT||anim==BE_WALK||anim==BE_RUN)) {
        // Os quadros pintados de andar mudam pouco as pernas. Um pulinho sincronizado com os
        // passos (um por passo, 2 por ciclo) faz o ritmo da caminhada aparecer. A sombra fica
        // no chao e o corpo sobe: le como "pisada".
        bool fast=(anim==DR_RUN||anim==DR_RUN_ALT||anim==BE_RUN);
        float hop=fabsf(sinf(g_druid.walk_cycle*1.5707963f));
        draws[draw_count-1].y-=hop*(fast?3.0f:2.2f);
    }
    // Tamanho aparente igual em todas as animacoes/direcoes (tabela gerada de sprite_norm.h).
    if (draw_count>before) draws[draw_count-1].scale*=s_norm[anim][dir&3];
    // Levou dano: pisca em vermelho por ~0,45 s (druida e lobisomem). Detecta a queda de HP aqui,
    // assim a esquiva (que tambem da invulnerabilidade) nao pisca.
    {
        static int last_hp=-1; static uint32_t flash_until=0;
        uint32_t now=get_ticks_ms();
        if (last_hp>=0 && g_druid.hp<last_hp && g_druid.hp>0) flash_until=now+450;
        last_hp=g_druid.hp;
        if (draw_count>before && now<flash_until && ((now/70)&1)==0)
            draws[draw_count-1].tint=0xFF4838FFu;   // multiplica a textura: vermelho vivo
    }
    if (draw_count>before && anim==DR_ATTACK) {
        // The middle paintings crouch while the staff extends sideways.
        // Keep the character's apparent body size consistent with idle.
        // Antes {1.0,1.08,1.22,1.04}: compensava a postura agachada, mas o druida "crescia" no meio do golpe
        // (o capuz tem o mesmo tamanho do idle nos quadros pintados). Agora so iguala ao idle.
        static const float attack_scale[4]={1.12f,1.12f,1.12f,1.12f};   // fixo: sem variar quadro a quadro
        draws[draw_count-1].scale*=table4(attack_scale,fr,anim_nf(anim),false);
    } else if (draw_count>before && anim==DR_RUN && dir==2) {
        // Hold the profile head height steady while the feet stay planted.
        static const float profile_scale[4]={1.037f,1.0f,1.0f,.966f};
        draws[draw_count-1].scale*=table4(profile_scale,fr,anim_nf(anim),true);
    } else if (draw_count>before && anim==DR_RUN_ALT && dir==2) {
        static const float profile_alt_scale[4]={1.027f,.974f,1.009f,.991f};
        draws[draw_count-1].scale*=table4(profile_alt_scale,fr,anim_nf(anim),true);
    } else if (draw_count>before && anim==DR_RUN_ALT && flip && dir==1) {
        static const float diagonal_scale[4]={1.036f,.983f,1.055f,1.0f};
        draws[draw_count-1].scale*=table4(diagonal_scale,fr,anim_nf(anim),true);
    }
    if (g_hero==HERO_BARBARIAN && draw_count>barb_before) {   // provisorio do barbaro
        if (!draws[draw_count-1].tint) draws[draw_count-1].tint=0xFFB496FFu;
        barb_finish(barb_before);
        add_barb_fx(vp);
    }
}
static Anim enemy_anim(EnemyType type,int kind) {
    static const Anim map[4][4]={
        {SK_IDLE,SK_WALK,SK_ATTACK,SK_HURT},
        {DE_IDLE,DE_WALK,DE_ATTACK,DE_HURT},
        {CU_IDLE,CU_WALK,CU_ATTACK,CU_HURT},
        {MA_IDLE,MA_WALK,MA_ATTACK,MA_HURT}
    };
    return map[type][kind];
}
static void add_enemies(T3DViewport *vp) {
    Enemy *array=enemies_get_array();
    for(int i=0;i<MAX_ENEMIES;i++) {
        Enemy *e=&array[i]; if(!e->active)continue;
        bool flip; int dir=facing(e->yaw,&flip);
        // Base sheet columns are distinct combat poses, not an idle loop.
        Anim anim=enemy_anim(e->type,0); int fr=0;
        if(e->state==ENEMY_STATE_DEAD) {
            anim=e->type==ENEMY_TYPE_BOSS_MALAKOR?MA_DEATH:enemy_anim(e->type,3);
            fr=e->state_timer<.4f?3:2;
        } else if(e->hurt_timer>0 || e->state==ENEMY_STATE_HURT) {
            anim=enemy_anim(e->type,3); fr=1;
        } else if(e->state==ENEMY_STATE_TELEGRAPH) {
            anim=e->type==ENEMY_TYPE_BOSS_MALAKOR && e->boss_phase==2
                ?MA_ENRAGE:enemy_anim(e->type,2); fr=0;
        } else if(e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY) {
            anim=enemy_anim(e->type,2);
            fr=e->state==ENEMY_STATE_RECOVERY?3:2-(int)(e->state_timer/.125f);
            if(fr<0)fr=0;
            if(fr>3)fr=3;
        } else if(e->state==ENEMY_STATE_CHASE &&
                  e->vx*e->vx+e->vz*e->vz>64) {
            anim=enemy_anim(e->type,1); fr=loop(e->anim_timer,.5f);
        }
        if (e->type==ENEMY_TYPE_BOSS_MALAKOR && anim>=MA_IDLE && anim<=MA_ENRAGE) {
            // Fase 1: Malakor (armadura). Com metade da vida ele se transforma no Varkhul (fase 2):
            // por ~1,2 s toca a furia do Malakor e depois passa a usar os quadros do Varkhul.
            static int last_phase=1; static uint32_t morph_until=0;
            uint32_t now=get_ticks_ms();
            if (e->boss_phase==2 && last_phase==1) morph_until=now+1800;
            last_phase=e->boss_phase;
            int k=anim-MA_IDLE;
            bool forest=g_campaign==1 && forest_boss_art(), north=g_campaign==2 && north_boss_art();
            Anim p1=forest?SY_IDLE:north?JA_IDLE:MH_IDLE, p2=forest?RM_IDLE:north?DG_IDLE:VK_IDLE;
            if (e->boss_phase==2 && now<morph_until && e->state!=ENEMY_STATE_DEAD) {
                anim=(Anim)(p1+5); fr=(int)((1800-(morph_until-now))/450); if(fr>3)fr=3;   // furia da fase 1
            } else anim=(Anim)((e->boss_phase==2?p2:p1)+k);
        }
        float h=e->type==ENEMY_TYPE_BOSS_MALAKOR?(e->boss_phase==2?50.0f:
                (g_campaign==1 && forest_boss_art()?49.0f:44.0f)):   // Sylvara: figura menor no quadro   // quadros 144 px: chefe gigante
                e->type==ENEMY_TYPE_DEMON?40:34;
        if (e->elite) h*=1.3f;
        s_next_shadow=(e->state==ENEMY_STATE_DEAD)?0.0f:e->radius*2.7f;
        int before=draw_count;
        s_next_occl=1;
        emit(vp,anim,dir,fr,flip,e->x,e->y,e->z,h);
        if (e->elite && draw_count>before) draws[draw_count-1].tint=0xFF8C78FFu; // guardiao: tom rubro
        uint32_t mt=world_monster_tint();
        if (e->type==ENEMY_TYPE_BOSS_MALAKOR && campaign_boss_art()) mt=0;   // arte propria: sem tinta
        if (mt && draw_count>before) {   // campanha da floresta: monstros cor de musgo
            uint32_t base=draws[draw_count-1].tint?draws[draw_count-1].tint:0xFFFFFFFFu;
            uint32_t r=((base>>24)&255)*((mt>>24)&255)/255, g=((base>>16)&255)*((mt>>16)&255)/255, b=((base>>8)&255)*((mt>>8)&255)/255;
            draws[draw_count-1].tint=(r<<24)|(g<<16)|(b<<8)|255u;
        }
    }
}
static void add_spells(T3DViewport *vp) {
    const SpiritWolf *wolves=spells_get_wolves();
    for(int i=0;i<MAX_WOLVES;i++) {
        const SpiritWolf *w=&wolves[i]; if(!w->active)continue;
        bool flip; int dir=facing(w->yaw,&flip);
        Anim anim=w->life>w->max_life-0.5f?WO_SUMMON:
            w->attack_cooldown>.55f?WO_ATTACK:
            w->state==WOLF_RUN?WO_RUN:WO_IDLE;
        int fr=anim==WO_SUMMON?(int)((w->max_life-w->life)*8):loop(w->anim_timer,.6f);
        // Vertical run frame 2 is an airborne leap in the source art. Using
        // it every step made the wolf appear to hop in place up/down screen.
        if (anim==WO_RUN && (dir==0 || dir==3)) {
            static const int grounded[4]={0,1,3,1};
            fr=grounded[fr];
        }
        if(fr<0)fr=0;
        if(fr>3)fr=3;
        s_next_shadow=14.0f;
        s_next_occl=2;
        emit(vp,anim,dir,fr,flip,w->x,w->y,w->z,24);
    }
    const Projectile *projectiles=spells_get_projectiles();
    float projectile_scale=clampf(ref_px_height(15.0f)/60.0f,.16f,2.2f);
    // Painted effects pulse across their four frames. Compensate for the
    // different occupied widths so the projectile keeps a stable diameter.
    static const float bolt_size[4]={1.0f,1.43f,.92f,1.57f};
    static const float dark_size[4]={1.0f,1.94f,1.0f,.89f};
    static const float bolt_cx[4]={23.5f,24.5f,23.0f,23.5f};
    static const float bolt_cy[4]={31.5f,34.5f,37.0f,33.0f};
    static const float dark_cx[4]={23.5f,23.5f,23.5f,23.5f};
    static const float dark_cy[4]={28.5f,36.5f,37.0f,30.0f};
    for(int i=0;i<MAX_PROJECTILES;i++) {
        const Projectile *p=&projectiles[i]; if(!p->active)continue;
        if (p->type==PROJ_ARROW) {   // flecha: escolhe o quadro girado mais proximo da direcao na tela
            T3DVec3 a,b;
            t3d_viewport_calc_viewspace_pos(vp,&a,&(T3DVec3){{p->x,p->y,p->z}});
            t3d_viewport_calc_viewspace_pos(vp,&b,&(T3DVec3){{p->x+cosf(p->yaw)*20.0f,p->y,p->z+sinf(p->yaw)*20.0f}});
            float ang=atan2f(-(b.v[1]-a.v[1]),b.v[0]-a.v[0]);
            int k=((int)floorf(ang/0.7853982f+0.5f)%8+8)%8;
            int before=draw_count;
            emit(vp,ARROW,0,k,false,p->x,p->y,p->z,10);
            if (draw_count>before) { draws[draw_count-1].centered=true; draws[draw_count-1].scale=clampf(ref_px_height(15.0f)/60.0f,.16f,2.2f)*1.7f; }   // flecha visivel
            continue;
        }
        Anim anim=p->type==PROJ_SPIRIT_BOLT?FX_BOLT:FX_DARK;
        int fr=loop(p->life,10);
        int before=draw_count;
        emit(vp,anim,0,fr,false,p->x,p->y,p->z,15);
        if (draw_count>before) {
            Draw *d=&draws[draw_count-1];
            d->scale=projectile_scale*(anim==FX_BOLT?bolt_size[fr]:dark_size[fr]);
            d->centered=true;
            d->x+=(24.0f-(anim==FX_BOLT?bolt_cx[fr]:dark_cx[fr]))*d->scale;
            d->y+=(24.0f-(anim==FX_BOLT?bolt_cy[fr]:dark_cy[fr]))*d->scale;
        }
    }
    const RootTrap *roots=spells_get_roots();
    for(int i=0;i<MAX_ROOTS;i++) {
        const RootTrap *t=&roots[i]; if(!t->active)continue;
        // armada: so o circulo de runas (quadro 0); disparada: as raizes crescem (1..3) e seguram
        int fr=t->sprung?1+(int)(t->anim*8.0f):0; if(fr>3)fr=3;
        int before=draw_count;
        emit(vp,HU_ROOTS,0,fr,false,t->x,0,t->z,30);
        if (draw_count>before && !t->sprung) draws[draw_count-1].order=-1.0e9f+draws[draw_count-1].y*0.001f;   // no chao
    }
    const Hawk *hawks=spells_get_hawks();
    for(int i=0;i<MAX_HAWKS;i++) {
        const Hawk *h=&hawks[i]; if(!h->active)continue;
        float dx=cosf(h->yaw), dz=sinf(h->yaw);
        bool flip=false; int dir;
        if (dz>0.82f) dir=3;                  // costas, voando para cima
        else if (dz<-.82f) dir=0;             // frente, voando para baixo
        else { dir=dz>0.18f?1:2; flip=dx>0.0f; } // diagonal/lado: base aponta direita
        int before=draw_count;
        emit(vp,HAWK,dir,loop_n(h->anim,8.0f,4),flip,h->x,h->y,h->z,42);   // mais legivel no jogo
        if (draw_count>before) draws[draw_count-1].order+=40.0f;   // voa por cima de tudo
    }
    const ThornSpike *thorns=spells_get_thorns();
    for(int i=0;i<MAX_THORNS;i++) {
        const ThornSpike *t=&thorns[i]; if(!t->active)continue;
        int fr=(int)(t->life*8); if(fr>3)fr=3;
        emit(vp,FX_THORN,0,fr,false,t->x,0,t->z,46);   // espinhos/raizes do Druida: o dobro (antes 22, quase invisivel)
    }
}
// Portal de saida: luz azul no chao e faiscas subindo (o vortice em si e malha 3D no world.c).
static void add_portal_fx(T3DViewport *vp) {
    const AreaData *ad=&g_area_data[g_current_area];
    if (true || g_current_area!=AREA_FORSAKEN_PATH || !ad->exit_unlocked || !s_glow_blue) return;   // fase 1 agora usa o arco da caverna (brilho proprio)
    if (!in_view(ad->exit_x,ad->exit_z)) return;
    T3DVec3 f;
    t3d_viewport_calc_viewspace_pos(vp,&f,&(T3DVec3){{ad->exit_x,FOOT_LIFT,ad->exit_z}});
    float tms=(float)get_ticks_ms();
    float pulse=1.0f+0.10f*sinf(tms*0.006f);
    add_glow(s_glow_blue,f.v[0],f.v[1]-1.0f,58.0f*pulse);
    for (int i=0;i<6;i++) {
        float ph=fmodf(tms*0.00055f+(float)i/6.0f,1.0f);
        float sx=f.v[0]+sinf((float)i*2.3f+tms*0.002f)*(10.0f+6.0f*ph);
        float sy=f.v[1]-4.0f-ph*50.0f;
        add_glow(s_glow_blue,sx,sy,7.0f*(1.0f-ph)+2.0f);
    }
}
static void add_items(T3DViewport *vp) {
    add_portal_fx(vp);
    const ItemDrop *drops=items_get_drops();
    for(int i=0;i<MAX_DROPS;i++) {
        const ItemDrop *d=&drops[i]; if(!d->active)continue;
        s_next_shadow=11.0f;
        if (d->type==DROP_ESSENCE) emit(vp,ESSENCE,0,loop(d->bounce_timer,1.6f),false,d->x,d->y,d->z,18);
        else emit(vp,ITEM,d->type,loop(d->bounce_timer,1.3f),false,d->x,d->y,d->z,28);
        // brilho no chao para destacar: continue dourado, orbe de alma azul
        if (d->type==DROP_CONTINUE || d->type==DROP_SOUL_ORB || d->type==DROP_ESSENCE) {
            T3DVec3 f; t3d_viewport_calc_viewspace_pos(vp,&f,&(T3DVec3){{d->x,0.0f,d->z}});
            float pulse=1.0f+0.15f*sinf((float)get_ticks_ms()*0.006f+d->x);
            add_glow(d->type==DROP_CONTINUE?s_glow_gold:d->type==DROP_ESSENCE?s_glow_fire:s_glow_blue,f.v[0],f.v[1],(d->type==DROP_CONTINUE?46.0f:d->type==DROP_ESSENCE?22.0f:30.0f)*pulse);
        }
    }
    const Chest *chests=items_get_chests();
    for(int i=0;i<MAX_CHESTS;i++) {
        const Chest *c=&chests[i]; if(!c->active)continue;
        int fr=c->opened?(int)(c->open_timer*10)+1:0; if(fr>3)fr=3;
        if (!c->opened && s_glow_gold && in_view(c->x,c->z)) {
            // Bau fechado emite um brilho dourado pulsante: chama o jogador.
            T3DVec3 cf;
            t3d_viewport_calc_viewspace_pos(vp,&cf,&(T3DVec3){{c->x,0.0f,c->z}});
            // Selado: brilho vermelho (precisa vencer o evento). Bauzinho: brilho menor.
            float pulse=1.0f+0.14f*sinf((float)get_ticks_ms()*(c->locked?0.009f:0.005f));
            sprite_t *g=(c->locked&&s_glow_fire)?s_glow_fire:s_glow_gold;
            add_glow(g,cf.v[0],cf.v[1]-2.0f,(c->small?26.0f:46.0f)*pulse);
        }
        s_next_shadow=c->small?18.0f:30.0f;
        emit(vp,CHEST,1,fr,false,c->x,0,c->z,c->small?19:32);
    }
}
static float wrap_range(float v, float lo, float hi) {
    float span=hi-lo;
    float m=fmodf(v-lo,span); if (m<0.0f) m+=span;
    return lo+m;
}
// Visao limitada: manchas de nevoa derivando e, por cima, a escuridao que fecha em volta do druida.
static void draw_vision_overlay(T3DViewport *vp) {
    // User-requested removal of the black vignette in the original Cemetery.
    if (g_campaign == 0 && g_current_area == AREA_FORSAKEN_PATH) return;
    if (!VISION_ENABLED || !s_vision_spr) return;
    T3DVec3 p;
    t3d_viewport_calc_viewspace_pos(vp,&p,&(T3DVec3){{g_druid.x,g_druid.y+10.0f,g_druid.z}});
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_filter(FILTER_BILINEAR);
    float t=(float)get_ticks_ms()*0.001f;
    if (FOG_PUFFS_ENABLED && s_fog_spr) {
        for (int i=0;i<5;i++) {
            // paralaxe: a nevoa se mexe um pouco com o druida, alem de derivar sozinha
            float x=wrap_range(30.0f+(float)i*83.0f+t*(4.0f+(float)i*1.1f)-g_druid.x*1.3f,-120.0f,440.0f);
            float y=wrap_range(20.0f+(float)((i*53)%240)+t*(1.5f+(float)(i%3))+g_druid.z*0.9f,-80.0f,320.0f);
            float size=170.0f+(float)(i%3)*60.0f;
            rdpq_blitparms_t fp={.cx=s_fog_spr->width/2,.cy=s_fog_spr->height/2,
                .scale_x=size/(float)s_fog_spr->width,.scale_y=size*0.7f/(float)s_fog_spr->height};
            rdpq_sprite_blit(s_fog_spr,x,y,&fp);
        }
    }
    float diameter=((g_current_area==AREA_HOLLOW_CRYPT)?VISION_DIAMETER*0.86f:VISION_DIAMETER)*world_vision_scale();   // cripta: visao mais curta; nevasca fecha
    float sc=diameter/(float)s_vision_spr->width;
    rdpq_blitparms_t bp={.cx=s_vision_spr->width/2,.cy=s_vision_spr->height/2,.scale_x=sc,.scale_y=sc};
    rdpq_sprite_blit(s_vision_spr,p.v[0],p.v[1]-12.0f,&bp);
    rdpq_mode_filter(FILTER_POINT);
    // Fora do quadrado da mascara: escuridao solida. Sem isso, quando a visao encolhe (nevasca do
    // Norte, cripta) ou o heroi vai para perto da borda da tela, os cantos ficavam sem escuridao.
    {
        int x0=(int)(p.v[0]-diameter*0.5f)+1, x1=(int)(p.v[0]+diameter*0.5f)-1;
        int y0=(int)(p.v[1]-12.0f-diameter*0.5f)+1, y1=(int)(p.v[1]-12.0f+diameter*0.5f)-1;
        if (x0<0)x0=0;
        if (y0<0)y0=0;
        if (x1>SCREEN_WIDTH)x1=SCREEN_WIDTH;
        if (y1>SCREEN_HEIGHT)y1=SCREEN_HEIGHT;
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_set_prim_color(RGBA32(14,18,20,247));
        if (y0>0) rdpq_fill_rectangle(0,0,SCREEN_WIDTH,y0);
        if (y1<SCREEN_HEIGHT) rdpq_fill_rectangle(0,y1,SCREEN_WIDTH,SCREEN_HEIGHT);
        if (x0>0 && y1>y0) rdpq_fill_rectangle(0,y0,x0,y1);
        if (x1<SCREEN_WIDTH && y1>y0) rdpq_fill_rectangle(x1,y0,SCREEN_WIDTH,y1);
        if (x0>=SCREEN_WIDTH || x1<=0 || y0>=SCREEN_HEIGHT || y1<=0) rdpq_fill_rectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
    }
}

// ---------------------------------------------------------------------------------------
// Oclusao por muros (estilo Diablo). Os sprites sao 2D e nao tem teste de profundidade, entao a
// oclusao e geometrica: cada tile de muro ('#') SUL do sprite (a camera olha de sul para norte)
// cobre na tela o retangulo [topo da borda norte, base da face sul]. O que cai nesse retangulo
// fica atras do muro: o sprite e recortado (scissor) e, para o druida/lobos, a parte escondida
// e redesenhada como silhueta translucida.
typedef struct { int x0,y0,x1,y1; } IRect;
#define OCC_MAX 4
#define PIECE_MAX 24
static int wall_occluders(T3DViewport *vp, const Draw *d, IRect sp, IRect *out) {
    float hw,hh,tile,H; int cols,rows;
    if (!world_wall_info(&hw,&hh,&tile,&H,&cols,&rows)) return 0;
    int rs=(int)floorf((hh-d->wz)/tile);
    int c0=(int)floorf((d->wx-60.0f+hw)/tile), c1=(int)floorf((d->wx+60.0f+hw)/tile);
    int n=0;
    for (int r=rs+1; r<=rs+3 && r<rows; r++) for (int c=c0; c<=c1; c++) {
        if (c<0 || c>=cols || !world_wall_tile(c,r)) continue;
        // Projecao do muro: calculada uma vez por quadro e por tile (antes: 6 projecoes por sprite)
        static IRect memo[1600]; static uint32_t memo_frame[1600];
        int mi=r*cols+c;
        IRect w;
        if (mi>=0 && mi<1600 && memo_frame[mi]==frame_number) w=memo[mi];
        else {
            float x0=(float)c*tile-hw, x1=x0+tile, z1=hh-(float)r*tile, z0=z1-tile;
            float px[6][3]={{x0,0,z0},{x1,0,z0},{x0,H,z0},{x1,H,z0},{x0,H,z1},{x1,H,z1}};
            float minx=1e9f,miny=1e9f,maxx=-1e9f,maxy=-1e9f;
            for (int k=0;k<6;k++) {
                T3DVec3 q;
                t3d_viewport_calc_viewspace_pos(vp,&q,&(T3DVec3){{px[k][0],px[k][1],px[k][2]}});
                if (q.v[0]<minx) minx=q.v[0];
                if (q.v[0]>maxx) maxx=q.v[0];
                if (q.v[1]<miny) miny=q.v[1];
                if (q.v[1]>maxy) maxy=q.v[1];
            }
            w=(IRect){(int)floorf(minx),(int)floorf(miny),(int)ceilf(maxx),(int)ceilf(maxy)};
            if (mi>=0 && mi<1600) { memo[mi]=w; memo_frame[mi]=frame_number; }
        }
        if (w.x1<=sp.x0 || w.x0>=sp.x1 || w.y1<=sp.y0 || w.y0>=sp.y1) continue;
        bool merged=false;
        for (int k=0;k<n;k++) {
            if (abs(out[k].y0-w.y0)<=2 && abs(out[k].y1-w.y1)<=2 &&
                w.x0<=out[k].x1+1 && w.x1>=out[k].x0-1) {
                if (w.x0<out[k].x0) out[k].x0=w.x0;
                if (w.x1>out[k].x1) out[k].x1=w.x1;
                if (w.y0<out[k].y0) out[k].y0=w.y0;
                if (w.y1>out[k].y1) out[k].y1=w.y1;
                merged=true; break;
            }
        }
        if (!merged && n<OCC_MAX) out[n++]=w;
    }
    return n;
}
// p menos r: ate 4 retangulos restantes
static int rect_sub(IRect p, IRect r, IRect *out) {
    if (r.x1<=p.x0 || r.x0>=p.x1 || r.y1<=p.y0 || r.y0>=p.y1) { out[0]=p; return 1; }
    int n=0;
    if (r.y0>p.y0) out[n++]=(IRect){p.x0,p.y0,p.x1,r.y0};
    if (r.y1<p.y1) out[n++]=(IRect){p.x0,r.y1,p.x1,p.y1};
    int ya=r.y0>p.y0?r.y0:p.y0, yb=r.y1<p.y1?r.y1:p.y1;
    if (r.x0>p.x0) out[n++]=(IRect){p.x0,ya,r.x0,yb};
    if (r.x1<p.x1) out[n++]=(IRect){r.x1,ya,p.x1,yb};
    return n;
}
static void blit_draw(const Draw *d, sprite_t *spr) {
    float width_scale=d->scale*(d->anim==DR_MOVE_BACK?DRUID_BACK_WIDTH_BOOST:1.0f);
    rdpq_blitparms_t bp={.cx=spr->width/2,.cy=d->centered?spr->height/2:spr->height,
        .scale_x=width_scale,.scale_y=d->scale,.flip_x=d->flip};
    rdpq_sprite_blit(spr,d->x,d->y,&bp);
}
static void scissor_rect(IRect r) {
    if (r.x0<0) r.x0=0;
    if (r.y0<0) r.y0=0;
    if (r.x1>SCREEN_WIDTH) r.x1=SCREEN_WIDTH;
    if (r.y1>SCREEN_HEIGHT) r.y1=SCREEN_HEIGHT;
    if (r.x1<=r.x0 || r.y1<=r.y0) return;
    rdpq_set_scissor(r.x0,r.y0,r.x1,r.y1);
}

void sprites_draw_world(T3DViewport *vp) {
    if(!ready)return;
    if (g_hero!=HERO_BARBARIAN) clear_barb_trails();
    frame_number++; draw_count=0;
    // Dinamicos primeiro: personagem, inimigos e itens nunca perdem vaga para o cenario.
    add_druid(vp); add_enemies(vp); add_spells(vp); add_items(vp); add_scenery(vp);
    for(int i=1;i<draw_count;i++) {
        Draw value=draws[i]; int j=i-1;
        while(j>=0 && draws[j].order>value.order) { draws[j+1]=draws[j];j--; }
        draws[j+1]=value;
    }
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    bool tinted=false, translucent=false;
    for(int i=0;i<draw_count;i++) {
        Draw *d=&draws[i];
        sprite_t *spr=d->direct?d->direct:frame_sprite(d->anim,d->dir,d->frame);
        if(!spr)continue;
        if (d->tint) {
            bool ghost=(d->tint&255u)<255u;
            if (!tinted || translucent!=ghost) {
                if (ghost) rdpq_mode_combiner(RDPQ_COMBINER1((ZERO,ZERO,ZERO,PRIM),(TEX0,ZERO,PRIM,ZERO)));
                else rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
                tinted=true; translucent=ghost;
            }
            rdpq_set_prim_color(RGBA32((d->tint>>24)&255,(d->tint>>16)&255,(d->tint>>8)&255,d->tint&255));
        } else if (tinted) {
            rdpq_mode_combiner(RDPQ_COMBINER_TEX); tinted=false; translucent=false;
        }
        IRect occ[OCC_MAX]; int nocc=0; IRect sp={0,0,0,0};
        if (d->occl) {
            float width_scale=d->scale*(d->anim==DR_MOVE_BACK?DRUID_BACK_WIDTH_BOOST:1.0f);
            float cx=(float)(spr->width/2)*width_scale;
            sp.x0=(int)floorf(d->x-cx); sp.x1=(int)ceilf(d->x-cx+(float)spr->width*width_scale);
            if (d->centered) {   // decalques: desenhados centrados no ponto
                float hh=(float)spr->height*d->scale*0.5f;
                sp.y0=(int)floorf(d->y-hh); sp.y1=(int)ceilf(d->y+hh);
            } else {
                sp.y0=(int)floorf(d->y-(float)spr->height*d->scale); sp.y1=(int)ceilf(d->y);
            }
            nocc=wall_occluders(vp,d,sp,occ);
        }
        if (nocc==0) { blit_draw(d,spr); continue; }

        // Parte visivel = retangulo do sprite menos os retangulos dos muros.
        IRect pieces[PIECE_MAX]; int np=1; pieces[0]=sp;
        for (int k=0;k<nocc;k++) {
            IRect next[PIECE_MAX]; int nn=0;
            for (int q=0;q<np;q++) {
                IRect sub[4]; int ns=rect_sub(pieces[q],occ[k],sub);
                for (int m=0;m<ns && nn<PIECE_MAX;m++) next[nn++]=sub[m];
            }
            for (int q=0;q<nn;q++) pieces[q]=next[q];
            np=nn;
        }
        for (int q=0;q<np;q++) { scissor_rect(pieces[q]); blit_draw(d,spr); }
        if (d->occl==2) {
            // Silhueta translucida do que esta atras do muro.
            rdpq_mode_combiner(RDPQ_COMBINER1((ZERO,ZERO,ZERO,PRIM),(TEX0,ZERO,PRIM,ZERO)));
            rdpq_set_prim_color(RGBA32(150,255,185,150));
            for (int k=0;k<nocc;k++) {
                IRect r=occ[k];
                if (r.x0<sp.x0) r.x0=sp.x0;
                if (r.y0<sp.y0) r.y0=sp.y0;
                if (r.x1>sp.x1) r.x1=sp.x1;
                if (r.y1>sp.y1) r.y1=sp.y1;
                if (r.x1<=r.x0 || r.y1<=r.y0) continue;
                scissor_rect(r); blit_draw(d,spr);
            }
            rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            tinted=false;
        }
        rdpq_set_scissor(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
    }
    draw_vision_overlay(vp);
}
void sprites_draw_ui(SpriteUi id,float x,float y,float scale) {
    if(!ready || id<0 || id>=SPR_UI_COUNT)return;
    if(!ui[id]) {
        char path[72];
        snprintf(path,sizeof(path),"rom:/spr_%s.rgba16.sprite",ui_names[id]);
        ui[id]=sprite_load(path);
        if(!ui[id]) { debugf("[SPRITES] missing %s\n",path);return; }
    }
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_blitparms_t bp={.scale_x=scale,.scale_y=scale};
    rdpq_sprite_blit(ui[id],x,y,&bp);
}

// Retrato animado do druida (tela de escolha de personagem): idle de frente, pes em (x, y).
void sprites_draw_druid_portrait(float x, float y, float scale) { sprites_draw_hero_portrait(HERO_DRUID, x, y, scale); }
void sprites_draw_hero_portrait(int hero, float x, float y, float scale) { sprites_draw_hero_portrait_ex(hero, x, y, scale, false); }
void sprites_draw_hero_portrait_ex(int hero, float x, float y, float scale, bool dark) {
    if (hero==HERO_ROGUE) return; // no borrowed portrait while its own art is pending
    if (!ready) return;
    frame_number++;
    Anim pa = hero == HERO_HUNTRESS ? HU_IDLE : (hero == HERO_BARBARIAN && barb_art()) ? BA_IDLE : DR_IDLE;
    int fr = (int)((float)get_ticks_ms() * 0.001f * (pa == DR_IDLE ? DRUID_IDLE_FPS : 5.0f * NF_PLAYER / 4.0f)) % NF_PLAYER;
    if (pa==BA_IDLE || pa==DR_IDLE) fr=video_idle_frame(pa==BA_IDLE?0:1,0,get_ticks_ms());
    sprite_t *spr = frame_sprite(pa, 0, fr);
    if (!spr) return;
    rdpq_set_mode_standard();
    if (dark) { rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT); rdpq_set_prim_color(RGBA32(14, 10, 10, 255)); }   // silhueta
    else rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_alphacompare(1);
    rdpq_blitparms_t bp = { .cx = spr->width / 2, .cy = spr->height, .scale_x = scale, .scale_y = scale };
    rdpq_sprite_blit(spr, x, y, &bp);
}
