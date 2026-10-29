#include "sprites.h"
#include "sprite_norm.h"
#include "anim_frames.h"
#include "run_video_art.h"
#include "idle_video_art.h"
#include "skeleton_art.h"
#include "demon_art.h"
#include "demon_attack_art.h"
#include "demon_combat.h"
#include "cultist_art.h"
#include "cultist_spell_art.h"
#include "cultist_combat.h"
#include "chest_art.h"
#include "boss_movement_art.h"
#include "malakor_combat.h"
#include "malakor_combat_art.h"
#include "varkhul_combat.h"
#include "campaign_combat.h"
#include "druid.h"
#include "enemies.h"
#include "spells.h"
#include "items.h"
#include "world.h"
#include "cemetery_stage.h"
#include "huntress.h"
#include "barbarian.h"
#include "barbarian_attack.h"
#include "barbarian_combo_art.h"
#include "barbarian_warcry_art.h"
#include "barbarian_skill_art.h"
#include "barbarian_skills.h"
#include "cards.h"
#include <stdint.h>
#include <rdpq_tri.h>

#define CACHE_BASE 192       // quadros na memoria (console de 4 MB)
#define CACHE_EXPANDED 384   // com Expansion Pak (8 MB): o dobro, sem recarregar da ROM no meio da horda
#define CACHE_MAX CACHE_EXPANDED
#define DRAW_MAX 320
#define VISION_ENABLED  0   // User requested full-screen visibility in every stage/campaign.
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
    DE_IDLE, DE_WALK, DE_ATTACK, DE_DEATH,
    CU_IDLE, CU_WALK, CU_ATTACK, CU_DEATH,
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
    DE_SLAM,
    FX_CULTIST_SKULL, FX_CULTIST_HEAL,
    CHEST_STEEL, CHEST_QUEST,
    MH_PUNCH, MH_GROUND, MH_PULL, FX_MH_WAVE,
    VK_SCYTHE, VK_DROP, VK_MORPH, VK_DIE, FX_VK_IMPACT,
    DW_CONTACT,DW_WALK,DW_CAST,DW_DIE,
    // Approved hero revision animations.
    HA_000,
    HA_001,
    HA_002,
    HA_003,
    HA_004,
    HA_005,
    HA_006,
    HA_007,
    HA_008,
    HA_009,
    HA_010,
    HA_011,
    HA_012,
    HA_013,
    HA_014,
    HA_015,
    HA_016,
    HA_017,
    HA_018,
    HA_019,
    HA_020,
    HA_021,
    HA_022,
    HA_023,
    HA_024,
    HA_025,
    HA_026,
    HA_027,
    HA_028,
    HA_029,
    HA_030,
    HA_031,
    HA_032,
    HA_033,
    HA_034,
    HA_035,
    HA_036,
    HA_037,
    HA_038,
    HA_039,
    HA_040,
    HA_041,
    HA_042,
    // End approved hero revision animations.
    // Approved campaign art: generated, sparse directions.
    APV_000,
    APV_001,
    APV_002,
    APV_003,
    APV_004,
    APV_005,
    APV_006,
    APV_007,
    APV_008,
    APV_009,
    APV_010,
    APV_011,
    APV_012,
    APV_013,
    APV_014,
    APV_015,
    APV_016,
    APV_017,
    APV_018,
    APV_019,
    APV_020,
    APV_021,
    APV_022,
    APV_023,
    APV_024,
    APV_025,
    APV_026,
    APV_027,
    APV_028,
    APV_029,
    APV_030,
    APV_031,
    APV_032,
    APV_033,
    APV_034,
    APV_035,
    APV_036,
    APV_037,
    APV_038,
    APV_039,
    APV_040,
    APV_041,
    APV_042,
    APV_043,
    APV_044,
    APV_045,
    APV_046,
    APV_047,
    APV_048,
    APV_049,
    APV_050,
    APV_051,
    APV_052,
    APV_053,
    APV_054,
    APV_055,
    APV_056,
    APV_057,
    APV_058,
    APV_059,
    APV_060,
    APV_061,
    APV_062,
    APV_063,
    APV_064,
    APV_065,
    APV_066,
    APV_067,
    APV_068,
    APV_069,
    APV_070,
    APV_071,
    APV_072,
    APV_073,
    APV_074,
    APV_075,
    APV_076,
    APV_077,
    APV_078,
    APV_079,
    APV_080,
    APV_081,
    APV_082,
    APV_083,
    APV_084,
    APV_085,
    APV_086,
    APV_087,
    APV_088,
    APV_089,
    APV_090,
    APV_091,
    APV_092,
    APV_093,
    APV_094,
    APV_095,
    APV_096,
    APV_097,
    APV_098,
    APV_099,
    APV_100,
    APV_101,
    APV_102,
    APV_103,
    APV_104,
    APV_105,
    APV_106,
    APV_107,
    APV_108,
    APV_109,
    APV_110,
    APV_111,
    APV_112,
    APV_113,
    APV_114,
    APV_115,
    APV_116,
    APV_117,
    APV_118,
    APV_119,
    APV_120,
    APV_121,
    ANIM_COUNT
} Anim;

static const char *names[ANIM_COUNT] = {
    "mage_idle","mage_walk","mage_walk_alt","mage_run","mage_run_alt","mage_attack","mage_cast","mage_escape",
    "mage_hurt","mage_death","mage_transform",
    "werewolf_idle","werewolf_walk","werewolf_run","werewolf_attack","werewolf_cast",
    "werewolf_escape","werewolf_hurt","werewolf_death",
    "skeleton_v2_idle","skeleton_v2_walk","skeleton_v2_attack","skeleton_hurt",
    "demon_idle","demon_v2_walk","demon_v2_attack","demon_death",
    "cultist_idle","cultist_v2_walk","cultist_v3_cast","cultist_death",
    "malakor_idle","malakor_walk","malakor_attack","malakor_hurt","malakor_death","malakor_enrage",
    "wolf_idle","wolf_run","wolf_attack","wolf_summon",
    "spirit_bolt","thorn_burst","dark_orb","item","chest_v2_wood",
    "mage_run_left","mage_run_front","werewolf_run_left","mage_run_back",
    "mage_nrun_s","mage_nrun_n","mage_nrun_w","mage_nrun_sw",
    "malakor_v3_idle","malakor_v3_walk","malakor_hd_attack","malakor_hd_hurt","malakor_hd_death","malakor_hd_enrage",
    "varkhul_v3_idle","varkhul_v3_run","varkhul_attack","varkhul_hurt","varkhul_death","varkhul_enrage",
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
    "mage_run8","demon_v2_slam","cultist_skull","cultist_heal","chest_v2_steel","chest_v2_quest",
    "malakor_v4_punch","malakor_v4_ground","malakor_v4_pull","malakor_wave",
    "varkhul_v4_scythe","varkhul_v4_drop","varkhul_v4_morph","varkhul_v4_death","varkhul_v4_impact",
    "desert_warden_contact","desert_warden_walk","desert_warden_cast","desert_warden_die",
    "ha_druida_humano_transformar_lobo",
    "ha_druida_humano_transformar_cervo",
    "ha_druida_cervo_investida_bestial",
    "ha_cacadora_base_flecha_perfurante",
    "ha_cacadora_base_sombra_violeta",
    "ha_cacadora_base_rajada_perfurante",
    "ha_ladina_base_combo_02_esquerda",
    "ha_ladina_base_combo_03_duplo",
    "ha_cacadora_efeitos_virote_perfurante",
    "ha_cacadora_efeitos_armadilha_prensora",
    "ha_fx_cacadora_fx_flechas_triplas",
    "ha_fx_mago_fx_orbe_negra",
    "ha_fx_mago_fx_impacto_orbe_area",
    "ha_fx_druida_simbolo_garras",
    "ha_fx_druida_simbolo_lobo",
    "ha_fx_druida_simbolo_cervo",
    "ha_druida_humano_ataque_garras",
    "ha_druida_lobo_ataque_garras",
    "ha_druida_cervo_ataque_garras",
    "ha_druida_humano_investida_bestial",
    "ha_druida_lobo_investida_bestial",
    "ha_cacadora_base_ataque_triplo",
    "ha_cacadora_base_armadilha_prensora",
    "ha_cacadora_base_rolamento",
    "ha_ladina_base_combo_01_direita",
    "ha_ladina_base_bomba_fumaca",
    "ha_ladina_base_avanco_giratorio",
    "ha_ladina_base_arremessar_adaga",
    "ha_ladina_base_teleporte",
    "ha_mago_base_avanco_energia",
    "ha_ladina_efeitos_nuvem_fumaca",
    "ha_ladina_efeitos_adaga_arremessada",
    "ha_mago_efeitos_dreno_almas",
    "ha_mago_efeitos_barreira_mortos",
    "ha_mago_efeitos_aura_escuridao",
    "ha_ladina_base_contact",
    "ha_mago_base_contact",
    "ha_mago_base_pentagrama_orbe",
    "ha_mago_base_poco_almas",
    "ha_mago_base_drenar_vida",
    "ha_mago_base_barreira_mortos",
    "ha_mago_base_poder_mortos",
    "ha_mago_efeitos_poco_almas",
    "apv_1_enforcado_brejo_contact",
    "apv_1_enforcado_brejo_andar",
    "apv_1_enforcado_brejo_ataque_normal",
    "apv_1_enforcado_brejo_morrer",
    "apv_1_flagelo_raizes_contact",
    "apv_1_flagelo_raizes_ataque_normal",
    "apv_1_flagelo_raizes_casca_ancorada",
    "apv_1_flagelo_raizes_morrer",
    "apv_1_oraculo_fungico_contact",
    "apv_1_oraculo_fungico_conjurar",
    "apv_1_oraculo_fungico_morrer",
    "apv_1_sylvara_contact",
    "apv_1_sylvara_andar",
    "apv_1_sylvara_ataque_normal",
    "apv_1_sylvara_sementes_funebres",
    "apv_1_sylvara_circulo_espinhos",
    "apv_1_sylvara_transformar_mae_raiz",
    "apv_1_mae_raiz_contact",
    "apv_1_mae_raiz_ataque_normal",
    "apv_1_mae_raiz_raizes_cacadoras",
    "apv_1_mae_raiz_floracao_parasita",
    "apv_1_mae_raiz_morrer",
    "apv_1_efeitos_dardo_micelio",
    "apv_1_efeitos_floracao_negra",
    "apv_1_efeitos_germinacao_espinhos",
    "apv_1_efeitos_mandibula_raiz",
    "apv_1_efeitos_veu_esporos",
    "apv_2_guerreiro_sepulcro_contact",
    "apv_2_guerreiro_sepulcro_andar",
    "apv_2_guerreiro_sepulcro_ataque_normal",
    "apv_2_guerreiro_sepulcro_morrer",
    "apv_2_cao_tumulo_contact",
    "apv_2_cao_tumulo_andar",
    "apv_2_cao_tumulo_morder",
    "apv_2_cao_tumulo_uivo_alcateia",
    "apv_2_cao_tumulo_morrer",
    "apv_2_vidente_geada_contact",
    "apv_2_vidente_geada_conjurar",
    "apv_2_vidente_geada_morrer",
    "apv_2_jarl_hrimgar_contact",
    "apv_2_jarl_hrimgar_ataque_normal",
    "apv_2_jarl_hrimgar_espada_retorno",
    "apv_2_jarl_hrimgar_tribunal_runico",
    "apv_2_jarl_hrimgar_transformar_draugr_gelo",
    "apv_2_draugr_gelo_contact",
    "apv_2_draugr_gelo_ataque_normal",
    "apv_2_draugr_gelo_lancas_carapaca",
    "apv_2_draugr_gelo_geada_inexoravel",
    "apv_2_draugr_gelo_morrer",
    "apv_2_efeitos_espada_voadora",
    "apv_2_efeitos_estilhaco_geada",
    "apv_2_efeitos_geada_solo",
    "apv_2_efeitos_reflexo_gelo",
    "apv_2_efeitos_runa_julgamento",
    "apv_3_mumia_lanceira_contact",
    "apv_3_mumia_lanceira_andar",
    "apv_3_mumia_lanceira_ataque_normal",
    "apv_3_mumia_lanceira_morrer",
    "apv_3_ushabti_rachado_contact",
    "apv_3_ushabti_rachado_andar",
    "apv_3_ushabti_rachado_ataque_normal",
    "apv_3_ushabti_rachado_escaravelhos_carcere",
    "apv_3_ushabti_rachado_morrer",
    "apv_3_nemezar_contact",
    "apv_3_nemezar_andar",
    "apv_3_nemezar_ataque_normal",
    "apv_3_nemezar_correntes_dinastia",
    "apv_3_nemezar_balanca_mortuaria",
    "apv_3_nemezar_transformar_rei_sem_sol",
    "apv_3_rei_sem_sol_contact",
    "apv_3_rei_sem_sol_andar",
    "apv_3_rei_sem_sol_ataque_normal",
    "apv_3_rei_sem_sol_liturgia_quatro",
    "apv_3_rei_sem_sol_morrer",
    "apv_3_efeitos_corrente_dinastia",
    "apv_3_efeitos_escaravelho",
    "apv_3_efeitos_mandala_vazio",
    "apv_3_efeitos_raio_eclipse",
    "apv_4_saqueador_cadaverico_contact",
    "apv_4_saqueador_cadaverico_ataque_normal",
    "apv_4_saqueador_cadaverico_morrer",
    "apv_4_carrasco_ferro_contact",
    "apv_4_carrasco_ferro_ataque_normal",
    "apv_4_carrasco_ferro_sentenca_ferro",
    "apv_4_carrasco_ferro_morrer",
    "apv_4_bruxa_saco_contact",
    "apv_4_bruxa_saco_andar",
    "apv_4_bruxa_saco_agulha_luto",
    "apv_4_bruxa_saco_costura_boca",
    "apv_4_bruxa_saco_morrer",
    "apv_4_condessa_vespera_contact",
    "apv_4_condessa_vespera_ataque_normal",
    "apv_4_condessa_vespera_tacas_rubras",
    "apv_4_condessa_vespera_transformar_noiva_meia_noite",
    "apv_4_noiva_meia_noite_contact",
    "apv_4_noiva_meia_noite_ataque_normal",
    "apv_4_noiva_meia_noite_grito_hora_morta",
    "apv_4_noiva_meia_noite_procissao_trapos",
    "apv_4_noiva_meia_noite_morrer",
    "apv_4_efeitos_agulha_luto",
    "apv_4_efeitos_costura_maldita",
    "apv_4_efeitos_gota_rubra",
    "apv_4_efeitos_lamento_morto",
    "apv_4_efeitos_trapo_errante",
    "apv_4_efeitos_veu_lamina",
    "apv_1_oraculo_fungico_andar",
    "apv_3_rei_sem_sol_mandala_vazio",
    "apv_4_saqueador_cadaverico_andar",
    "apv_4_condessa_vespera_veu_cortante",
    "apv_3_escorpiao_ferrao",
    "apv_3_escorpiao_morrer",
    "apv_3_escorpiao_contact",
    "apv_2_vidente_geada_andar",
    "apv_1_flagelo_raizes_andar",
    "apv_1_mae_raiz_andar",
    "apv_2_jarl_hrimgar_andar",
    "apv_2_draugr_gelo_andar",
    "apv_4_carrasco_ferro_andar",
    "apv_4_condessa_vespera_andar",
    "apv_4_noiva_meia_noite_andar",
    "apv_3_escorpiao_andar",
    "apv_3_escorpiao_pincas",
};
#include "approved_campaign_art_data.h"
#include "approved_hero_art_data.h"
#include "sprite_resolution_data.h"
#include "review_hero_actions.h"

// Legacy campaign probes are retired; approved_spec is the only art gate.
// A arte do chefe da floresta ja foi importada? (senao usa o Malakor/Varkhul pintados de musgo)
static bool forest_boss_art(void) {return false;}
static bool north_boss_art(void) {return false;}
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
                 float wx, wz; uint8_t occl; int16_t rotation; float wy; } Draw; // rotation uses existing alignment padding
static Cache cache[CACHE_MAX];
static uint32_t s_frame_loads;
static int s_cache_size = CACHE_BASE;
static uint32_t s_cache_bytes, s_cache_budget;
#define CACHE_BUDGET_BASE (512u * 1024u)
#define CACHE_BUDGET_EXPANDED (2048u * 1024u)
// Maior textura dinamica atual:112x112 RGBA32. Reserva inclui metadados e margem.
#define CACHE_LOAD_RESERVE (64u * 1024u)
// Indice direto chave -> vaga (+1). Antes cada sprite procurava a chave nas 192 vagas, todo quadro.
#define KEY_MAX (ANIM_COUNT * 8 * 16) // exact animation table size; no unused fixed reserve
static uint16_t s_key_slot[KEY_MAX];
static uint8_t s_key_missing[KEY_MAX / 8];
_Static_assert(ANIM_COUNT * 8 * 16 <= KEY_MAX, "KEY_MAX pequeno demais");   // arquivo que nao existe: nao tenta abrir de novo a cada quadro
static Draw draws[DRAW_MAX];
static sprite_t *ui[SPR_UI_COUNT];
static sprite_t *scene[SCENE_PROP_COUNT];
static sprite_t *scene_frames[SCENE_PROP_COUNT][4];
static const char *scene_names[SCENE_PROP_COUNT] = {
    [SCENE_CHAPEL_CANDLES]="chapel_candles",
    [SCENE_CHAPEL_FONT]="chapel_font",
    [SCENE_CHAPEL_KNEELER]="chapel_kneeler",
    [SCENE_CHAPEL_LECTERN]="chapel_lectern",
    [SCENE_CHAPEL_ALTAR]="chapel_altar",
    [SCENE_CHAPEL_CONFESSIONAL]="chapel_confessional",
    [SCENE_EX_DESERT_STELA]="ex_desert_stela",
    [SCENE_EX_DESERT_URNS]="ex_desert_urns",
    [SCENE_EX_DESERT_SARCOPHAGUS]="ex_desert_sarcophagus",
    [SCENE_EX_DESERT_MUMMY]="ex_desert_mummy",
    [SCENE_EX_DESERT_THORN]="ex_desert_thorn",
    [SCENE_EX_DESERT_BRAZIER]="ex_desert_brazier",
    [SCENE_EX_VICT_LAMP]="ex_vict_lamp",
    [SCENE_EX_VICT_HEARSE]="ex_vict_hearse",
    [SCENE_EX_VICT_GRAVES]="ex_vict_graves",
    [SCENE_EX_VICT_QUARANTINE]="ex_vict_quarantine",
    [SCENE_EX_VICT_RELIQUARY]="ex_vict_reliquary",
    [SCENE_EX_VICT_FENCE]="ex_vict_fence",
    [SCENE_EX_DESERT_EXIT]="ex_desert_exit",
    [SCENE_EX_VICT_EXIT]="ex_vict_exit",
    [SCENE_FOREST_FERN]="forest_fern",
    [SCENE_FOREST_WOOD_CAGE]="forest_wood_cage",
    [SCENE_FOREST_PYRE_STAKE]="forest_pyre_stake",
    [SCENE_FOREST_BONE_EFFIGY]="forest_bone_effigy",
    [SCENE_FOREST_DRYGRASS]="forest_drygrass",
    [SCENE_SWAMP_ROWBOAT]="swamp_rowboat",
    [SCENE_HEART_V2_TOTEM_ON]="heart_v2_totem",
    [SCENE_HEART_V2_TOTEM_OFF]="heart_v2_totem",
    [SCENE_SNOW_V1_SHRUB]="snow_v1_shrub",
    [SCENE_SNOW_V1_CART]="snow_v1_cart",
    [SCENE_TREE_DEAD]="tree_dead", "gravestone", "chapel_arch", "campfire", "traveler_body",
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
static bool scene_is_layered(ScenePropId id) {
    return id==SCENE_EX_DESERT_BRAZIER || id==SCENE_EX_VICT_QUARANTINE;
}
static sprite_t *scene_layer(ScenePropId id,int frame) {
    char path[96];
    if(!scene[id]) {
        snprintf(path,sizeof(path),"rom:/scene_%s_base.rgba16.sprite",scene_names[id]);
        scene[id]=sprite_load(path);
    }
    if(!scene_frames[id][frame]) {
        snprintf(path,sizeof(path),"rom:/scene_%s_motion_f%d.rgba16.sprite",scene_names[id],frame);
        scene_frames[id][frame]=sprite_load(path);
    }
    return scene[id];
}
static bool scene_is_animated(ScenePropId id) {
    return id==SCENE_EX_DESERT_BRAZIER || id==SCENE_EX_VICT_QUARANTINE || id==SCENE_CAMPFIRE || id==SCENE_CRYPT_BRAZIER ||
           id==SCENE_INFERNAL_BRAZIER || id==SCENE_CAVE_BRAZIER || id==SCENE_CAVE_PORTAL ||
           id==SCENE_HELL_FIREVENT || id==SCENE_HELL_BRAZIER || id==SCENE_HELL_PORTAL || id==SCENE_VENDOR ||
           id==SCENE_SWAMP_LANTERN || id==SCENE_FOREST_PORTAL || id==SCENE_HEART_FUNGUS || id==SCENE_HEART_SPORE ||
           id==SCENE_SNOW_FIRE || id==SCENE_ICE_PORTAL || id==SCENE_ICE_BRAZIER || id==SCENE_FROST_VENT || id==SCENE_FROST_BRAZIER ||
           (id>=SCENE_PORTAL_PATH && id<=SCENE_PORTAL_ALTAR);
}
static int draw_count;
static int scenery_frame(ScenePropId id) {
    if(id==SCENE_EX_DESERT_BRAZIER)return (get_ticks_ms()/145)%4;
    if(id==SCENE_EX_VICT_QUARANTINE) {
        static const uint8_t sequence[8]={0,0,1,0,0,2,0,3};
        return sequence[(get_ticks_ms()/240)%8];
    }
    return (get_ticks_ms()/180)%3; // Existing three-frame loops keep their timing.
}
static uint32_t frame_number;
static bool ready;

// Escala FIXA para todos os sprites: pixels por unidade de mundo medidos numa camera de
// referencia ja assentada (mesmos valores de main.c). Antes cada tipo de sprite usava uma
// regra diferente (perspectiva ao vivo ou a camera que atrasa ao andar), e por isso eles
// cresciam e encolhiam enquanto o jogador se movia.
static float s_ref_ppu = 1.73f;               // cenario: px por unidade (segmento de 100u, igual ao original)
static T3DViewport s_ref_vp;                   // camera de referencia (jogador parado, camera assentada)
static float s_norm[ANIM_COUNT][4];             // fator de tamanho por (animacao, direcao), de sprite_norm.h
static uint8_t s_next_occl = 0;               // 0: padrao fisico; 2: personagem com silhueta
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
    draws[draw_count++]=(Draw){0,0,0,false,x,y,scale,-1.0e8f+y,s_shadow_spr,true,0,0.0f,0.0f,0,0,0};
}
static void add_shadow(float x, float y, float width_units) {
    if (!SHADOWS_ENABLED || !s_shadow_spr || draw_count>=DRAW_MAX) return;
    float scale = clampf(width_units*s_ref_ppu/(float)s_shadow_spr->width, .1f, 6.0f);
    y -= 0.5f*(float)(s_shadow_spr->height/2)*scale;   // sobe a sombra: so uma fatia aparece abaixo dos pes
    draws[draw_count++]=(Draw){0,0,0,false,x,y,scale,-1.0e8f+y,s_shadow_spr,true,0,0.0f,0.0f,0,0,0};
}
static void add_glow(sprite_t *spr, float x, float y, float width_units) {
    if (!spr || draw_count>=DRAW_MAX) return;
    if (x<-140 || x>SCREEN_WIDTH+140 || y<-100 || y>SCREEN_HEIGHT+100) return;
    float scale = clampf(width_units*s_ref_ppu/(float)spr->width, .1f, 8.0f);
    draws[draw_count++]=(Draw){0,0,0,false,x,y,scale,-9.0e8f+y,spr,true,0,0.0f,0.0f,0,0,0};
}
static void anchor_ground_draw(int before,float x,float z) {
    if(draw_count<=before)return;
    Draw *d=&draws[draw_count-1];
    d->wx=x;d->wz=z;d->wy=cemetery_stage_ground_y(x,z,d->scale*d->direct->width/s_ref_ppu)+1;
    d->occl=1;
}
static void add_world_glow(sprite_t *spr,float sx,float sy,float width,float x,float z) {
    int before=draw_count;add_glow(spr,sx,sy,width);anchor_ground_draw(before,x,z);
}

// Animacoes do jogador com versao suave (arquivos spr_<nome>_s_d*_f0..NF_PLAYER-1).
static bool smooth_anim(Anim a) { return (a>=DR_IDLE && a<=BE_DEATH) || a==BE_MOVE_SIDE || (a>=HU_IDLE && a<=HU_ROLL) || (a>=BA_IDLE && a<=BA_ROLL) || a==BA_RUN8 || a==DR_RUN8 || (a>=BA_COMBO1 && a<=BA_WHIRLWIND); }
static int anim_nf(Anim a) {
    if(a>=HA_000 && a<APV_000)return hero_approved_art[a-HA_000].frames;
    if(a==DW_CONTACT)return 1;
    if(a>=DW_WALK && a<=DW_DIE)return 4;
    if(a>=APV_000 && a<ANIM_COUNT)return approved_art[a-APV_000].frames;
    if (a==MH_PUNCH || a==MH_GROUND || a==MH_PULL || a==FX_MH_WAVE) return MALAKOR_COMBAT_FRAMES;
    if (a==MH_IDLE || a==VK_IDLE) return 1;
    if (a==MH_WALK || a==VK_WALK) return BOSS_MOVEMENT_FRAMES;
    if (a==SK_IDLE) return 1;
    if (a==SK_WALK) return SKELETON_WALK_FRAMES;
    if (a==SK_ATTACK) return SKELETON_ATTACK_FRAMES;
    if (a==DE_WALK) return DEMON_WALK_FRAMES;
    if (a==CU_WALK) return CULTIST_WALK_FRAMES;
    if (a==CU_ATTACK) return CULTIST_CAST_FRAMES;
    if (a==CU_DEATH) return 2;
    if (a==CHEST || a==CHEST_STEEL || a==CHEST_QUEST) return CHEST_ART_FRAMES;
    if (a==DE_ATTACK || a==DE_SLAM) return DEMON_ATTACK_FRAMES;
    if (a==DE_DEATH) return 2;
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
    const SpriteResolution *resolution=&sprite_resolution[anim][dir];
    uint32_t load_reserve=resolution->density?(uint32_t)resolution->width*resolution->height+1024u:CACHE_LOAD_RESERVE;
    while (slot<0 || s_cache_bytes+load_reserve>s_cache_budget) {
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
    s_frame_loads++;
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
    // New four-frame scenery is small and can be read during the transition,
    // rather than opening a file on its first visible flame/head movement.
    int prop_count=0;const SceneProp *props=world_scene_props(&prop_count);
    for(int i=0;i<prop_count;i++) {
        ScenePropId id=props[i].id;
        if(id!=SCENE_EX_DESERT_BRAZIER && id!=SCENE_EX_VICT_QUARANTINE)continue;
        for(int f=0;f<4;f++)scene_layer(id,f);
    }
    warm_barb_combo(true);
    if (s_cache_size <= CACHE_BASE) return;
    static const Anim pre[] = { SK_WALK, SK_ATTACK, SK_HURT, DE_WALK, DE_ATTACK, DE_SLAM, DE_DEATH,
                                CU_WALK, CU_ATTACK, CU_DEATH };
    for (unsigned a = 0; a < sizeof(pre) / sizeof(pre[0]); a++) {
        if(g_campaign>0 && !(g_campaign==3 && (pre[a]==CU_WALK || pre[a]==CU_ATTACK || pre[a]==CU_DEATH)))continue;
        int directions=(pre[a]==SK_WALK || pre[a]==SK_ATTACK || pre[a]==DE_WALK || pre[a]==DE_ATTACK || pre[a]==DE_SLAM || pre[a]==CU_WALK || pre[a]==CU_ATTACK || pre[a]==CU_DEATH)?3:4;
        for (int d = 0; d < directions; d++)
            for (int f = pre[a]==SK_HURT?1:0; f < anim_nf(pre[a]); f++) frame_sprite(pre[a], d, f);
    }
    for (int f = 0; f < 4; f++) { frame_sprite(FX_BOLT, 0, f); frame_sprite(FX_DARK, 0, f); frame_sprite(ITEM, 0, f); frame_sprite(ESSENCE, 0, f); }
    if(g_campaign==0 || g_campaign==3)
        for (int f=0;f<4;f++) { frame_sprite(FX_CULTIST_SKULL,0,f);frame_sprite(FX_CULTIST_HEAL,0,f); }
    for (int f=0;f<CHEST_ART_FRAMES;f++) {frame_sprite(CHEST,0,f);frame_sprite(CHEST_STEEL,0,f);frame_sprite(CHEST_QUEST,0,f);}
    if (g_campaign==0) {
        static const Anim bosses[]={MH_IDLE,MH_WALK,VK_IDLE,VK_WALK};
        for (unsigned a=0;a<sizeof(bosses)/sizeof(bosses[0]);a++)
            for (int d=0;d<3;d++)
                for (int f=0;f<anim_nf(bosses[a]);f++)frame_sprite(bosses[a],d,f);
    }
    if(g_campaign>0 && g_campaign<=4)for(int role=0;role<5;role++) {
        int index=approved_lookup[g_campaign][role][CA_CONTACT];if(index<0)continue;
        for(int d=0;d<3;d++) {
            if(s_cache_bytes+CACHE_LOAD_RESERVE>s_cache_budget*3/4)break;
            frame_sprite((Anim)approved_art[index].anim,d,0);
        }
    }
    if(g_campaign==0 && g_current_area==AREA_BLACK_ALTAR) {
        for(int d=0;d<3;d++)for(int f=0;f<4;f++) {frame_sprite(MH_PUNCH,d,f);frame_sprite(MH_GROUND,d,f);}
        for(int f=0;f<4;f++) {frame_sprite(MH_PULL,0,f);frame_sprite(FX_MH_WAVE,0,f);}
    }
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
    if(VISION_ENABLED)s_vision_spr=sprite_load("rom:/scene_vision.rgba32.sprite");
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
// Cardinal enemy artwork: 0 front, 1 right, 2 back. The camera projects
// world -X to screen right; left deliberately mirrors right (user approved).
// Hurt/death retain their original four-row sheet mapping until redrawn.
static int skeleton_facing(float yaw, bool *flip) {
    float dx=cosf(yaw), dz=sinf(yaw);
    if (fabsf(dz)>=fabsf(dx)) {
        *flip=false;
        return dz>=0.0f?2:0;
    }
    *flip=dx>0.0f;
    return 1;
}
static bool boss_cardinal_art(Anim a) {
    return a==MH_IDLE || a==MH_WALK || a==VK_IDLE || a==VK_WALK;
}
static int boss_movement_frame(float clock, bool moving, bool transformed) {
    if (!moving) return 0;
    float interval=transformed?VARKHUL_RUN_TIMER_PER_FRAME:MALAKOR_WALK_TIMER_PER_FRAME;
    return (int)(fmaxf(clock,0.0f)/interval)%BOSS_MOVEMENT_FRAMES;
}
static bool malakor_combat_art(Anim a) {return a==MH_PUNCH || a==MH_GROUND || a==MH_PULL;}
static int malakor_combat_frame(const Enemy *e) {
    if(e->state==ENEMY_STATE_TELEGRAPH)
        return e->telegraph_duration-e->state_timer<.25f?0:1;
    if(e->state==ENEMY_STATE_ATTACK && e->boss_move==MALAKOR_MOVE_PULL)
        return 1+((int)(fmaxf(0.0f,MALAKOR_PULL_ACTIVE-e->state_timer)/.16f)&1);
    return e->state==ENEMY_STATE_ATTACK?2:3;
}
// Match the approved keys to existing combat events. Damage/sound already
// happen when TELEGRAPH becomes ATTACK; never delay the impact art past that.
static int skeleton_attack_frame(EnemyState state, float remaining, float windup) {
    if (state==ENEMY_STATE_TELEGRAPH)
        return remaining<=fminf(SKELETON_ATTACK_DESCENT_SECONDS,windup*.5f)?1:0;
    return state==ENEMY_STATE_ATTACK?2:3;
}
static int demon_walk_frame(float timer, bool moving) {
    if (!moving) return 0;
    return (int)(fmaxf(timer,0.0f)/DEMON_WALK_TIMER_PER_FRAME)%DEMON_WALK_FRAMES;
}
static int cultist_walk_frame(float timer, bool moving) {
    if (!moving) return 0;
    return (int)(fmaxf(timer,0.0f)/CULTIST_WALK_TIMER_PER_FRAME)%CULTIST_WALK_FRAMES;
}
static int cultist_cast_frame(EnemyState state, float remaining) {
    if (state==ENEMY_STATE_TELEGRAPH)
        return CULTIST_CAST_WINDUP-remaining<CULTIST_CAST_GATHER?0:1;
    return state==ENEMY_STATE_ATTACK?2:3;
}
static int demon_attack_frame(EnemyState state, float remaining, bool slam) {
    if (state==ENEMY_STATE_TELEGRAPH) {
        if (slam) return DEMON_SLAM_WINDUP-remaining<DEMON_SLAM_PREP_POSE?0:1;
        return remaining>0.10f?0:1;
    }
    return state==ENEMY_STATE_ATTACK?2:3;
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
    // Everything emitted here belongs to the world, including loot, chests,
    // projectiles and spell sprites. UI and ground effects use separate paths.
    uint8_t occl=s_next_occl?s_next_occl:1; s_next_occl=0;
    if (draw_count>=DRAW_MAX-1) return;
    T3DVec3 feet;
    t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{x,y+FOOT_LIFT,z}});
    if (!in_view(x,z)) return;
    if (feet.v[0]<-80 || feet.v[0]>SCREEN_WIDTH+80 ||
        feet.v[1]<-80 || feet.v[1]>SCREEN_HEIGHT+80) return;
    float scale=clampf(ref_px_height(height)/60.0f,.16f,2.2f);
    if (shadow_w>0.0f) {
        int before=draw_count;add_shadow(feet.v[0],feet.v[1]+1.0f,shadow_w);anchor_ground_draw(before,x,z);
    }
    draws[draw_count++]=(Draw){anim,dir,frame,flip,feet.v[0],feet.v[1],scale,feet.v[1],NULL,false,0,x,z,occl,0,y+FOOT_LIFT};
}
// Five painted views. Only approved directions are substituted; a missing
// direction keeps its previous live animation instead of borrowing rejected art.
static int hero_art_direction(float yaw,bool *flip) {
    float angle=atan2f(sinf(yaw),-cosf(yaw));
    int sector=((int)floorf((angle+1.5707963f+.39269908f)/.78539816f)+8)&7;
    static const unsigned char view[8]={0,4,1,3,2,3,1,4};
    *flip=sector>4;return view[sector];
}
static bool emit_hero_art(T3DViewport *vp,int index,int dir,int frame,bool flip,
                          float x,float y,float z,float height,bool centered) {
    const HeroApprovedArt *a=&hero_approved_art[index];
    if(!a->active || !(a->mask&(1<<dir)))return false;
    if(frame<0)frame=0;
    if(frame>=a->frames)frame=a->frames-1;
    int before=draw_count;s_next_occl=2;
    emit(vp,(Anim)a->anim,dir,frame,flip,x,y,z,height);
    if(draw_count>before) {
        Draw *d=&draws[draw_count-1];d->centered=centered;
        if(!centered)d->y+=(a->height-a->floor)*d->scale;
    }
    return true;
}
static bool hero_art_pose(T3DViewport *vp,int index,float progress,float yaw,float height) {
    bool flip;int dir=hero_art_direction(yaw,&flip);
    return emit_hero_art(vp,index,dir,once_frame(progress,12),flip,g_druid.x,g_druid.y,g_druid.z,height,false);
}
static bool add_other_approved_hero(T3DViewport *vp) {
    if(g_hero!=HERO_ROGUE && g_hero!=HERO_MAGE)return false;
    bool rogue=g_hero==HERO_ROGUE;int before=draw_count;
    int art=rogue?HART_LADINA_BASE_CONTACT:HART_MAGO_BASE_CONTACT;
    float progress=0;
    if(g_druid.state==DRUID_STATE_DODGE){
        art=rogue?HART_LADINA_BASE_TELEPORTE:HART_MAGO_BASE_AVANCO_ENERGIA;
        progress=1-g_druid.dodge_timer/.30f;
    }else if(rogue && g_druid.state==DRUID_STATE_ATTACK){
        art=g_druid.combo==2?HART_LADINA_BASE_COMBO_03_DUPLO:g_druid.combo==1?HART_LADINA_BASE_COMBO_02_ESQUERDA:HART_LADINA_BASE_COMBO_01_DIREITA;
        progress=1-g_druid.attack_timer/fmaxf(.01f,g_druid.attack_duration);
    }else if(rogue && g_druid.visual_timer>0){
        int a=g_druid.visual_action;
        if(a==HERO_VIS_SMOKE)art=HART_LADINA_BASE_BOMBA_FUMACA;
        if(a==HERO_VIS_SPIN)art=HART_LADINA_BASE_AVANCO_GIRATORIO;
        if(a==HERO_VIS_KNIFE)art=HART_LADINA_BASE_ARREMESSAR_ADAGA;
        progress=1-g_druid.visual_timer/fmaxf(.01f,g_druid.visual_duration);
    }else if(!rogue && g_druid.state==DRUID_STATE_ATTACK){
        art=HART_MAGO_BASE_PENTAGRAMA_ORBE;
        progress=1-g_druid.attack_timer/fmaxf(.01f,g_druid.attack_duration);
    }else if(!rogue && g_druid.visual_timer>0){
        int a=g_druid.visual_action;
        if(a==HERO_VIS_WELL)art=HART_MAGO_BASE_POCO_ALMAS;
        if(a==HERO_VIS_DRAIN)art=HART_MAGO_BASE_DRENAR_VIDA;
        if(a==HERO_VIS_BARRIER)art=HART_MAGO_BASE_BARREIRA_MORTOS;
        if(a==HERO_VIS_DARKNESS)art=HART_MAGO_BASE_PODER_MORTOS;
        progress=1-g_druid.visual_timer/fmaxf(.01f,g_druid.visual_duration);
    }
    float yaw=g_druid.state==DRUID_STATE_WALK?g_druid.target_yaw:g_druid.yaw;
    if(!hero_art_pose(vp,art,progress,yaw,46))
        hero_art_pose(vp,rogue?HART_LADINA_BASE_CONTACT:HART_MAGO_BASE_CONTACT,0,yaw,46);
    if(draw_count>before && rogue && g_druid.camo_timer>0)draws[draw_count-1].tint=0xFFFFFF50u;
    return true;
}
static void add_review_hero_fx(T3DViewport *vp) {
    static const int art[]={HART_LADINA_EFEITOS_NUVEM_FUMACA,HART_MAGO_EFEITOS_DRENO_ALMAS,
        HART_MAGO_EFEITOS_BARREIRA_MORTOS,HART_MAGO_EFEITOS_AURA_ESCURIDAO,
        HART_FX_DRUIDA_SIMBOLO_GARRAS,HART_FX_DRUIDA_SIMBOLO_LOBO,HART_FX_DRUIDA_SIMBOLO_CERVO,
        HART_MAGO_EFEITOS_POCO_ALMAS};
    const HeroEffect *fx=review_hero_effects();
    for(int i=0;i<MAX_HERO_EFFECTS;i++)if(fx[i].life>0){
        const HeroEffect *f=&fx[i];int before=draw_count;
        bool icon=f->kind>=HERO_FX_CLAWS && f->kind<=HERO_FX_STAG;
        bool well=f->kind==HERO_FX_WELL;
        if(well && f->tick>0)continue;
        float progress=1-f->life/fmaxf(.01f,f->duration);
        int frame=icon?0:well?once_frame((f->duration-f->life-.5f)/1.02f,12):once_frame(fmodf(progress*3,1.0f),12);
        emit_hero_art(vp,art[f->kind],0,frame,false,f->x,icon?56:well?2:12,f->z,icon?24:64,true);
        if(draw_count>before && !icon)draws[draw_count-1].order-=.1f;
    }
}
static void add_huntress_trails(T3DViewport *vp,int before);
static void add_scenery(T3DViewport *vp) {
    int count;
    const SceneProp *props=world_scene_props(&count);
    float px_per_unit=s_ref_ppu;
    for (int i=0;i<count && draw_count<DRAW_MAX-3;i++) {
        const SceneProp *p=&props[i];
        if(p->ground_handle>0)continue; // rendered on a true ground plane, with mutable ritual state
        if (!in_view(p->x,p->z)) continue;
        sprite_t *prop_sprite;
        if (scene_is_layered(p->id)) {
            prop_sprite=scene_layer(p->id,scenery_frame(p->id));
        } else if (scene_is_animated(p->id)) {
            int frame=scenery_frame(p->id);
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
        t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{p->x,decal?p->y:FOOT_LIFT,p->z}});
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
            int before=draw_count;add_shadow_px(feet.v[0]+ft->cx*scale,sy+1.0f,sw);anchor_ground_draw(before,p->x,p->z);
        }
        if (scene_is_fire(p->id) && s_glow_fire) {
            // Poca de luz que pulsa de leve, sob o fogo.
            float flick=1.0f+0.06f*sinf((float)get_ticks_ms()*0.011f+p->x);
            add_world_glow(s_glow_fire,feet.v[0],feet.v[1]-3.0f,scene_glow_width(p->id)*flick,p->x,p->z);
        }
        if ((p->id==SCENE_CRYSTAL || (p->id>=SCENE_CAVE_CRYSTAL_A && p->id<=SCENE_CAVE_CRYSTAL_C) ||
             p->id==SCENE_CAVE_PORTAL || p->id==SCENE_SWAMP_LANTERN || p->id==SCENE_FOREST_PORTAL ||
             p->id==SCENE_HEART_FUNGUS || p->id==SCENE_SWAMP_SHROOM || p->id==SCENE_ICE_PORTAL || p->id==SCENE_ICE_BRAZIER ||
             p->id==SCENE_FROST_BRAZIER || (p->id>=SCENE_ICE_CRYSTAL_A && p->id<=SCENE_ICE_CRYSTAL_C)) && s_glow_blue) {   // luz fria: fogo-fatuo, fungos
            float pl=1.0f+0.10f*sinf((float)get_ticks_ms()*0.006f+p->x);
            add_world_glow(s_glow_blue,feet.v[0],feet.v[1]-3.0f,62.0f*pl,p->x,p->z);
        }
        draws[draw_count++]=(Draw){0,0,0,false,feet.v[0],sy,scale,
                                   decal?-1.0e9f+feet.v[1]*0.001f:feet.v[1],prop_sprite,decal,
                                   (scene_is_fire(p->id)||p->id==SCENE_VENDOR)?0:world_scene_tint(),
                                   p->x,p->z,(uint8_t)1,0,decal?p->y:FOOT_LIFT};
        if(p->id==SCENE_HEART_V2_TOTEM_OFF)draws[draw_count-1].tint=0x788478FFu;
        if(scene_is_layered(p->id) && draw_count<DRAW_MAX) {
            // One immutable body, one small motion layer. Both share the body's
            // scale, world foot and depth slope; frame bounds never move the prop.
            sprite_t *motion=scene_frames[p->id][scenery_frame(p->id)];
            if(motion) {
                Draw part=draws[draw_count-1];
                float socket_y=p->id==SCENE_EX_DESERT_BRAZIER?34.0f:38.0f;
                part.y+=(socket_y-(float)prop_sprite->height)*scale;
                part.direct=motion;
                if(p->id==SCENE_EX_DESERT_BRAZIER)part.tint=0;
                draws[draw_count++]=part;
            }
        }
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
    if(g_druid.hp>0 && g_druid.hurt_timer<=0 && g_druid.state==DRUID_STATE_DODGE &&
       hero_art_pose(vp,HART_CACADORA_BASE_ROLAMENTO,1-g_druid.dodge_timer/.30f,g_druid.yaw,HUNT_HEIGHT))return;
    if(g_druid.hp>0 && g_druid.hurt_timer<=0 && g_druid.state==DRUID_STATE_ATTACK) {
        int before=draw_count;
        if(hero_art_pose(vp,HART_CACADORA_BASE_ATAQUE_TRIPLO,1-g_druid.attack_timer/fmaxf(.01f,g_druid.attack_duration),g_druid.yaw,HUNT_HEIGHT)) {
            add_huntress_trails(vp,before);return;
        }
    }
    if(g_druid.hp>0 && g_druid.hurt_timer<=0 && g_druid.visual_timer>0 &&
       g_druid.state!=DRUID_STATE_ATTACK && g_druid.state!=DRUID_STATE_DODGE) {
        int art=g_druid.visual_action==HUNT_VISUAL_FOCUS?HART_CACADORA_BASE_SOMBRA_VIOLETA:
                g_druid.visual_action==HUNT_VISUAL_VOLLEY?HART_CACADORA_BASE_RAJADA_PERFURANTE:
                g_druid.visual_action==HUNT_VISUAL_TRAP?HART_CACADORA_BASE_ARMADILHA_PRENSORA:
                g_druid.visual_action==HUNT_VISUAL_PIERCE?HART_CACADORA_BASE_FLECHA_PERFURANTE:-1;
        int before=draw_count;
        if(art>=0 && hero_art_pose(vp,art,1-g_druid.visual_timer/fmaxf(.01f,g_druid.visual_duration),g_druid.yaw,HUNT_HEIGHT)) {
            add_huntress_trails(vp,before);return;
        }
    }
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
    add_huntress_trails(vp,before);
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
// Same six historical silhouettes as Barbarian, independently stored and purple.
static BarbTrail s_hunt_trails[BARB_TRAIL_COUNT];
static unsigned s_hunt_next;
static uint32_t s_hunt_tick,s_hunt_sample;
static float s_hunt_x,s_hunt_z;
static void add_huntress_trails(T3DViewport *vp,int before) {
    uint32_t now=get_ticks_ms();
    float dx=g_druid.x-s_hunt_x,dz=g_druid.z-s_hunt_z;
    if(!s_hunt_tick || now-s_hunt_tick>250 || dx*dx+dz*dz>3600 || g_druid.hp<=0)
        memset(s_hunt_trails,0,sizeof(s_hunt_trails));
    s_hunt_tick=now;s_hunt_x=g_druid.x;s_hunt_z=g_druid.z;
    if(draw_count<=before)return;
    Draw actor=draws[draw_count-1];
    if(g_druid.focus_timer>0 && dx*dx+dz*dz>.05f && now-s_hunt_sample>=45) {
        T3DVec3 feet;t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{g_druid.x,g_druid.y+FOOT_LIFT,g_druid.z}});
        s_hunt_trails[s_hunt_next]=(BarbTrail){actor.anim,actor.dir,actor.frame,actor.flip,g_druid.x,g_druid.y,g_druid.z,actor.scale,(actor.y-feet.v[1])/actor.scale,now};
        s_hunt_next=(s_hunt_next+1)%BARB_TRAIL_COUNT;s_hunt_sample=now;
    }
    for(int i=0;i<BARB_TRAIL_COUNT;i++) {
        BarbTrail *t=&s_hunt_trails[i];uint32_t age=now-t->born;
        if(!t->born || age>=BARB_TRAIL_LIFE_MS)continue;
        float x=g_druid.x-t->x,z=g_druid.z-t->z;if(x*x+z*z<4)continue;
        int n=draw_count;s_next_occl=1;
        emit(vp,t->anim,t->dir,t->frame,t->flip,t->x,t->y,t->z,HUNT_HEIGHT);
        if(draw_count==n)continue;
        Draw *d=&draws[draw_count-1];d->scale=t->mult;d->y+=t->offset*d->scale;
        d->order=actor.order-.01f-age*.001f;
        unsigned alpha=(unsigned)(110*(1-(float)age/BARB_TRAIL_LIFE_MS));d->tint=0xA044EF00u|(alpha?alpha:1u);
    }
}
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
// Separate world-space history for Varkhul, same six red silhouettes as Barbarian.
static BarbTrail s_vk_trails[BARB_TRAIL_COUNT];
static unsigned s_vk_next;
static uint32_t s_vk_tick,s_vk_sample;
static float s_vk_x,s_vk_z;
static void clear_vk_trails(void) {
    memset(s_vk_trails,0,sizeof(s_vk_trails));s_vk_tick=s_vk_sample=0;s_vk_next=0;
}
static void add_vk_trails(T3DViewport *vp,const Enemy *e,int before) {
    if(draw_count<=before)return;
    Draw actor=draws[draw_count-1];uint32_t now=get_ticks_ms();
    float dx=e->x-s_vk_x,dz=e->z-s_vk_z;
    if(!s_vk_tick || now-s_vk_tick>250 || dx*dx+dz*dz>3600 ||
       e->morph_timer>0 || e->state==ENEMY_STATE_DEAD ||
       (e->boss_move==VARKHUL_DROP && e->state!=ENEMY_STATE_CHASE))clear_vk_trails();
    s_vk_tick=now;s_vk_x=e->x;s_vk_z=e->z;
    if(enemies_get_varkhul_fx()->rush>0 && actor.anim==VK_WALK &&
       e->vx*e->vx+e->vz*e->vz>64 && now-s_vk_sample>=45) {
        T3DVec3 feet;t3d_viewport_calc_viewspace_pos(vp,&feet,&(T3DVec3){{e->x,e->y+FOOT_LIFT,e->z}});
        s_vk_trails[s_vk_next]=(BarbTrail){actor.anim,actor.dir,actor.frame,actor.flip,
            e->x,e->y,e->z,actor.scale,(actor.y-feet.v[1])/actor.scale,now};
        s_vk_next=(s_vk_next+1)%BARB_TRAIL_COUNT;s_vk_sample=now;
    }
    for(int i=0;i<BARB_TRAIL_COUNT;i++) {
        BarbTrail *t=&s_vk_trails[i];uint32_t age=now-t->born;
        if(!t->born || age>=BARB_TRAIL_LIFE_MS)continue;
        float x=e->x-t->x,z=e->z-t->z;if(x*x+z*z<4)continue;
        int n=draw_count;s_next_occl=1;
        emit(vp,t->anim,t->dir,t->frame,t->flip,t->x,t->y,t->z,VARKHUL_ART_HEIGHT);
        if(draw_count==n)continue;
        Draw *d=&draws[draw_count-1];d->scale=t->mult;d->y+=t->offset*d->scale;
        d->order=actor.order-.01f-age*.001f;
        unsigned alpha=(unsigned)(110*(1-(float)age/BARB_TRAIL_LIFE_MS));
        d->tint=0xEF182800u|(alpha?alpha:1u);
    }
}

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
    add_review_hero_fx(vp);
    if (g_hero==HERO_DRUID && !g_druid.is_beast && !g_druid.is_stag) return;
    if(add_other_approved_hero(vp))return;
    if (g_hero==HERO_HUNTRESS) { add_huntress(vp); return; }
    if (g_hero==HERO_BARBARIAN && barb_art()) { add_barbarian(vp); return; }
    if(g_hero==HERO_DRUID && g_druid.hp>0 && g_druid.hurt_timer<=0) {
        if(g_druid.state==DRUID_STATE_ATTACK) {
            int art=g_druid.is_stag?HART_DRUIDA_CERVO_ATAQUE_GARRAS:g_druid.is_beast?HART_DRUIDA_LOBO_ATAQUE_GARRAS:HART_DRUIDA_HUMANO_ATAQUE_GARRAS;
            if(hero_art_pose(vp,art,1-g_druid.attack_timer/fmaxf(.01f,g_druid.attack_duration),g_druid.yaw,g_druid.is_beast?50:46))return;
        } else if(g_druid.state==DRUID_STATE_DODGE) {
            int art=g_druid.is_stag?HART_DRUIDA_CERVO_INVESTIDA_BESTIAL:g_druid.is_beast?HART_DRUIDA_LOBO_INVESTIDA_BESTIAL:HART_DRUIDA_HUMANO_INVESTIDA_BESTIAL;
            if(hero_art_pose(vp,art,1-g_druid.dodge_timer/.30f,g_druid.yaw,g_druid.is_beast?50:46))return;
        } else if(g_druid.transform_timer>0 && g_druid.is_beast) {
            if(hero_art_pose(vp,g_druid.is_stag?HART_DRUIDA_HUMANO_TRANSFORMAR_CERVO:HART_DRUIDA_HUMANO_TRANSFORMAR_LOBO,1-g_druid.transform_timer/(g_druid.is_stag?.72f:.32f),g_druid.yaw,46))return;
        } else if(g_druid.transform_timer>0) {
            if(hero_art_pose(vp,HART_DRUIDA_HUMANO_ATAQUE_GARRAS,0,g_druid.yaw,46))return;
        }
        if(g_druid.is_stag && hero_art_pose(vp,HART_DRUIDA_CERVO_ATAQUE_GARRAS,0,g_druid.target_yaw,50))return;
    }
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
// Approved partial views are explicit: a missing direction stays on the approved
// contact design, never silently borrows a rejected view or mirrors front/back.
static const ApprovedArt *approved_spec_for(int camp,int role,int action,int *direction) {
 if(camp<1 || camp>4 || role<0 || role>5 || action<0 || action>=CA_ACTION_COUNT)return NULL;
 int idx=approved_lookup[camp][role][action];if(idx<0)return NULL;
 const ApprovedArt *s=&approved_art[idx];if(s->frontal)*direction=0;
 if(!(s->mask&(1u<<*direction)))return NULL;
 return s;
}
static bool add_approved_enemy(T3DViewport *vp,const Enemy *e,int index) {
 int camp=enemy_campaign(e);if(camp==0 || !campaign_enemy_enabled(e))return false;
 int role=campaign_enemy_role(e),action=role<3?CA_WALK:CA_CONTACT,fr=0;
 bool flip=false;int dir=skeleton_facing(e->yaw,&flip),original_dir=dir;
 bool moving=e->state==ENEMY_STATE_CHASE && e->vx*e->vx+e->vz*e->vz>64;
 if(e->state==ENEMY_STATE_DEAD) {
  action=CA_DEATH;fr=(int)((1.5f-e->state_timer)/.25f);if(fr<0)fr=0;if(fr>3)fr=3;
 } else if(e->morph_timer>0) {
  role=3;action=CA_MORPH;fr=(int)((1.8f-e->morph_timer)/.45f);if(fr<0)fr=0;if(fr>3)fr=3;
 } else if(e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY) {
  action=e->boss_move==2?CA_SKILL2:e->boss_move==1?CA_SKILL1:CA_NORMAL;
  fr=e->state==ENEMY_STATE_TELEGRAPH?(e->state_timer>e->telegraph_duration*.45f?0:1):e->state==ENEMY_STATE_ATTACK?2:3;
 } else if(moving) {action=CA_WALK;fr=(int)(fmaxf(0,e->anim_timer)/1.8f)%4;}
 if(camp==2 && role==3 && action==CA_SKILL1 && e->state==ENEMY_STATE_RECOVERY &&
    campaign_effect_active(index,CFE_ESPADA_VOADORA))fr=2;
 const ApprovedArt *s=approved_spec_for(camp,role,action,&dir);
 if(!s) {
  dir=original_dir;
  s=approved_spec_for(camp,role,role<3?CA_WALK:CA_CONTACT,&dir);fr=0;
  if(!s && role<3)s=approved_spec_for(camp,role,CA_CONTACT,&dir);
 }
 if(!s)return false;
 if(s->frontal) {flip=false;}
 if(s->frames==1) {fr=0;}
 int before=draw_count;s_next_occl=1;s_next_shadow=0;
 float h=s->world_height*(e->elite?1.3f:world_boss_guardian(index)?1.2f:1);
 if(e->variant==EV_MINIBOSS)h*=.70f;
 emit(vp,(Anim)s->anim,dir,fr,flip,e->x,e->y,e->z,h);
 if(draw_count>before) {
  Draw *d=&draws[draw_count-1];d->y+=(s->height-s->floor)*d->scale;
  if(e->elite)d->tint=0xFF8C78FFu;
 }
 return true;
}
static void approved_fx_one(T3DViewport *vp,const CampaignFX *f,float x,float z,int fr,float diameter) {
 int dir=0;const ApprovedArt *s=approved_spec_for(f->campaign,5,CA_FX_BASE+f->effect,&dir);if(!s)return;
 if(draw_count>=DRAW_MAX-1 || !in_view(x,z))return;
 sprite_t *spr=frame_sprite((Anim)s->anim,0,fr);if(!spr)return;
 bool ground=f->kind!=CFX_PROJECTILE && f->kind!=CFX_DECOY;
 T3DVec3 p;t3d_viewport_calc_viewspace_pos(vp,&p,&(T3DVec3){{x,ground?2:16,z}});
 float scale=diameter*s_ref_ppu/104.0f;
 Draw *d=&draws[draw_count++];
 *d=(Draw){.anim=(Anim)s->anim,.frame=fr,.x=p.v[0],.y=p.v[1],.scale=scale,
  .order=p.v[1]-(ground?.3f:0),.centered=true,.direct=ground?spr:NULL,
  .tint=f->delay>0?0xFFFFFF80u:0,.wx=x,.wz=z,.wy=ground?2:16,.occl=1};
 if(f->kind==CFX_PROJECTILE || f->kind==CFX_LINE || f->kind==CFX_CONE) {
  T3DVec3 q;t3d_viewport_calc_viewspace_pos(vp,&q,&(T3DVec3){{x+f->dx*20,ground?2:16,z+f->dz*20}});
  d->rotation=(int16_t)lrintf((ground?atan2f(f->dz,f->dx):atan2f(-(q.v[1]-p.v[1]),q.v[0]-p.v[0]))*4096);
 }
}
static void add_campaign_fx(T3DViewport *vp) {
 const CampaignFX *fx=campaign_get_fx();
 for(int i=0;i<CAMPAIGN_FX_MAX;i++) {
  const CampaignFX *f=&fx[i];if(f->life<=0)continue;
  int fr=f->delay>0?0:f->kind==CFX_PROJECTILE?(int)((f->total-f->life)*8)%4:
   1+(int)((f->total-f->windup-f->life)/.25f);
  if(fr<0) {fr=0;}
  if(fr>3) {fr=3;}
  if(f->kind==CFX_RING) {
   float a=atan2f(f->dz,f->dx);
   for(int k=0;k<12;k++) {
    float q=k*6.2831853f/12;
    if(fabsf(atan2f(sinf(q-a),cosf(q-a)))<.65f)continue;
    approved_fx_one(vp,f,f->x+cosf(q)*f->length,f->z+sinf(q)*f->length,fr,25);
   }
  } else if(f->kind==CFX_LINE) {
   // Place the same approved strip along the actual damage corridor.
   for(int k=0;k<3;k++)approved_fx_one(vp,f,f->x+f->dx*f->length*(k+.5f)/3,
       f->z+f->dz*f->length*(k+.5f)/3,fr,f->length/3);
  } else if(f->kind==CFX_DECOY) {
   int dir=0;const ApprovedArt *s=approved_spec_for(f->campaign,2,CA_CONTACT,&dir);
   if(s && draw_count<DRAW_MAX-1) {
    int before=draw_count;emit(vp,(Anim)s->anim,0,0,false,f->x,0,f->z,s->world_height);
    if(draw_count>before) {draws[draw_count-1].y+=(s->height-s->floor)*draws[draw_count-1].scale;draws[draw_count-1].tint=0x9ACFFF90u;}
   }
   approved_fx_one(vp,f,f->x,f->z,fr,24);
  } else approved_fx_one(vp,f,f->x,f->z,fr,
       f->kind==CFX_PROJECTILE?24:f->radius*2);
 }
}



static Anim enemy_anim(EnemyType type,int kind) {
    static const Anim map[4][4]={
        {SK_WALK,SK_WALK,SK_ATTACK,SK_HURT},
        {DE_WALK,DE_WALK,DE_ATTACK,DE_DEATH},
        {CU_WALK,CU_WALK,CU_ATTACK,CU_DEATH},
        {MA_IDLE,MA_WALK,MA_ATTACK,MA_HURT}
    };
    return map[type][kind];
}
static bool add_cemetery_miniboss(T3DViewport *vp,const Enemy *e) {
    if(e->variant!=EV_MINIBOSS || enemy_campaign(e)!=0)return false;
    bool flip=false;int dir=skeleton_facing(e->yaw,&flip),fr=0;bool phase=e->boss_phase==2;
    bool moving=e->state==ENEMY_STATE_CHASE && e->vx*e->vx+e->vz*e->vz>64;
    Anim a=phase?(moving?VK_WALK:VK_IDLE):(moving?MH_WALK:MH_IDLE);
    if(moving)fr=boss_movement_frame(e->anim_timer,true,phase);
    bool combat=e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY;
    if(combat){a=phase?VK_SCYTHE:e->boss_move?MH_GROUND:MH_PUNCH;fr=e->state==ENEMY_STATE_TELEGRAPH?(e->state_timer>e->telegraph_duration*.45f?0:1):e->state==ENEMY_STATE_ATTACK?2:3;}
    if(e->state==ENEMY_STATE_DEAD){a=phase?VK_DIE:MH_GROUND;dir=0;flip=false;fr=phase?(int)((1.5f-e->state_timer)/.25f):3;if(fr>3)fr=3;if(fr<0)fr=0;}
    int before=draw_count;s_next_occl=1;s_next_shadow=0;
    emit(vp,a,dir,fr,flip,e->x,e->y,e->z,(phase?VARKHUL_ART_HEIGHT:MALAKOR_ART_HEIGHT)*.70f);
    if(draw_count>before){Draw*d=&draws[draw_count-1];float margin=boss_cardinal_art(a)?BOSS_MOVEMENT_DRAW_Y_SHIFT:phase?16:MALAKOR_COMBAT_DRAW_Y_SHIFT;d->y+=margin*d->scale;}
    return true;
}
static bool add_desert_warden(T3DViewport *vp,const Enemy *e,int index) {
    if(g_campaign!=3 || !world_boss_guardian(index))return false;
    bool flip=false;int dir=skeleton_facing(e->yaw,&flip),fr=0;Anim a=DW_CONTACT;
    if(e->state==ENEMY_STATE_DEAD){a=DW_DIE;dir=0;flip=false;fr=(int)((1.5f-e->state_timer)/.25f);}
    else if(e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY) {
        a=DW_CAST;fr=e->state==ENEMY_STATE_TELEGRAPH?(e->state_timer>e->telegraph_duration*.45f?0:1):e->state==ENEMY_STATE_ATTACK?2:3;
    } else if(e->state==ENEMY_STATE_CHASE && e->vx*e->vx+e->vz*e->vz>64) {
        a=DW_WALK;fr=(int)(fmaxf(0,e->anim_timer)/1.8f)%4;
    }
    if(fr<0)fr=0;
    if(fr>3)fr=3;
    int before=draw_count;s_next_occl=1;s_next_shadow=0;
    emit(vp,a,dir,fr,flip,e->x,e->y,e->z,58);
    if(draw_count>before)draws[draw_count-1].y+=20*draws[draw_count-1].scale;
    return true;
}
static void add_enemies(T3DViewport *vp) {
    Enemy *array=enemies_get_array();
    for(int i=0;i<MAX_ENEMIES;i++) {
        Enemy *e=&array[i]; if(!e->active)continue;
        if(add_cemetery_miniboss(vp,e))continue;
        if(add_desert_warden(vp,e,i))continue;
        if(add_approved_enemy(vp,e,i))continue;
        bool flip; int dir=facing(e->yaw,&flip);
        // Normal enemies stop on walk frame 0 instead of loading an idle.
        bool moving=e->state==ENEMY_STATE_CHASE && e->vx*e->vx+e->vz*e->vz>64;
        bool slam=e->type==ENEMY_TYPE_DEMON && e->boss_move==DEMON_MOVE_SLAM;
        Anim anim=enemy_anim(e->type,0); int fr=0;
        if(e->state==ENEMY_STATE_DEAD) {
            anim=e->type==ENEMY_TYPE_BOSS_MALAKOR?MA_DEATH:enemy_anim(e->type,3);
            fr=e->state_timer<.4f?3:2;
            if (e->type==ENEMY_TYPE_DEMON || e->type==ENEMY_TYPE_CULTIST) fr-=2; // Existing death poses only.
        } else if(e->type!=ENEMY_TYPE_DEMON && e->type!=ENEMY_TYPE_CULTIST &&
                  (e->hurt_timer>0 || e->state==ENEMY_STATE_HURT)) {
            anim=enemy_anim(e->type,3); fr=1;
        } else if(e->state==ENEMY_STATE_TELEGRAPH) {
            anim=e->type==ENEMY_TYPE_BOSS_MALAKOR && e->boss_phase==2
                ?MA_ENRAGE:enemy_anim(e->type,2); fr=0;
        } else if(e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY) {
            anim=enemy_anim(e->type,2);
            fr=e->state==ENEMY_STATE_RECOVERY?3:2-(int)(e->state_timer/.125f);
            if(fr<0)fr=0;
            if(fr>3)fr=3;
        } else if(moving) {
            anim=enemy_anim(e->type,1); fr=loop(e->anim_timer,.5f);
        }
        if (anim==SK_WALK || anim==SK_ATTACK) {
            dir=skeleton_facing(e->yaw,&flip);
            if (anim==SK_WALK)
                fr=moving?(int)(fmaxf(e->anim_timer,0.0f)/SKELETON_WALK_TIMER_PER_FRAME)%SKELETON_WALK_FRAMES:0;
            if (anim==SK_ATTACK)
                fr=skeleton_attack_frame(e->state,e->state_timer,e->telegraph_duration);
        }
        if (anim==DE_WALK) {
            dir=skeleton_facing(e->yaw,&flip);
            fr=demon_walk_frame(e->anim_timer,moving);
        }
        if (anim==CU_WALK || anim==CU_ATTACK || anim==CU_DEATH) {
            dir=skeleton_facing(e->yaw,&flip);
            if (anim==CU_WALK) fr=cultist_walk_frame(e->anim_timer,moving);
            if (anim==CU_ATTACK) fr=cultist_cast_frame(e->state,e->state_timer);
        }
        if (anim==DE_ATTACK) {
            dir=skeleton_facing(e->yaw,&flip);
            if (slam && e->state==ENEMY_STATE_RECOVERY &&
                e->state_timer<=DEMON_SLAM_RECOVERY-DEMON_SLAM_RECOVERY_ART) {
                // Rocks settle; hold the existing walk contact pose through
                // the rest of the vulnerable recovery, without an idle asset.
                anim=DE_WALK;fr=0;
            } else {
                if (slam) anim=DE_SLAM;
                fr=demon_attack_frame(e->state,e->state_timer,slam);
            }
        }
        if (e->type==ENEMY_TYPE_BOSS_MALAKOR && anim>=MA_IDLE && anim<=MA_ENRAGE) {
            // Abstract MA_* states always resolve to the campaign artwork.
            // Preserve the existing 1.8-second transformation and combat art.
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
        if (boss_cardinal_art(anim)) {
            dir=skeleton_facing(e->yaw,&flip);
            fr=(anim==MH_WALK || anim==VK_WALK)?boss_movement_frame(e->anim_timer,moving,anim==VK_WALK):0;
        }
        if(e->type==ENEMY_TYPE_BOSS_MALAKOR && g_campaign==0 && e->boss_phase==1 &&
           (e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY)) {
            anim=e->boss_move==MALAKOR_MOVE_GROUND?MH_GROUND:e->boss_move==MALAKOR_MOVE_PULL?MH_PULL:MH_PUNCH;
            dir=skeleton_facing(e->yaw,&flip);
            if(anim==MH_PULL) {dir=0;flip=false;} // suction deliberately has a single front view
            fr=malakor_combat_frame(e);
            if(e->state==ENEMY_STATE_RECOVERY && e->state_timer<.25f) {anim=MH_IDLE;dir=skeleton_facing(e->yaw,&flip);fr=0;}
        }
        // Shared fallback bosses also resolve legacy Varkhul ids to current art.
        // This makes old attack/hurt/enrage/death sheets unreachable in every map.
        if(anim==VK_ATTACK || anim==VK_HURT) {
            anim=VK_SCYTHE;dir=skeleton_facing(e->yaw,&flip);
            if(fr<0) {fr=0;} if(fr>3) {fr=3;}
        } else if(anim==VK_DEATH || anim==VK_ENRAGE) {
            anim=anim==VK_DEATH?VK_DIE:VK_MORPH;dir=0;flip=false;
            if(fr<0) {fr=0;} if(fr>3) {fr=3;}
        }
        if(e->type==ENEMY_TYPE_BOSS_MALAKOR && g_campaign==0 && e->boss_phase==2) {
            dir=skeleton_facing(e->yaw,&flip);
            if(e->state==ENEMY_STATE_DEAD) {
                anim=VK_DIE;dir=0;flip=false;
                fr=(int)((1.5f-e->state_timer)/.22f);if(fr<0) {fr=0;} if(fr>3) {fr=3;}
            } else if(e->morph_timer>0) {
                anim=VK_MORPH;dir=0;flip=false;
                fr=(int)((1.8f-e->morph_timer)/.45f);if(fr<0) {fr=0;} if(fr>3) {fr=3;}
            } else if(e->boss_move==VARKHUL_DROP &&
                (e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY)) {
                anim=VK_DROP;dir=0;flip=false;
                fr=e->state==ENEMY_STATE_TELEGRAPH?0:e->state==ENEMY_STATE_ATTACK?1:
                    e->state_timer>VARKHUL_DROP_RECOVERY-.18f?2:3;
                if(e->state==ENEMY_STATE_TELEGRAPH && e->y<=0) {anim=VK_IDLE;fr=0;}
            } else if(e->boss_move==VARKHUL_SCYTHE &&
                (e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK || e->state==ENEMY_STATE_RECOVERY)) {
                anim=VK_SCYTHE;
                fr=e->state==ENEMY_STATE_TELEGRAPH?(e->state_timer>.3f?0:1):
                    e->state==ENEMY_STATE_ATTACK?2:3;
            } else {
                anim=moving?VK_WALK:VK_IDLE;
                fr=moving?boss_movement_frame(e->anim_timer,true,true):0;
            }
        }
        float h=e->type==ENEMY_TYPE_BOSS_MALAKOR?(e->boss_phase==2?50.0f:
                (g_campaign==1 && forest_boss_art()?49.0f:44.0f)):   // Sylvara: figura menor no quadro   // quadros 144 px: chefe gigante
                e->type==ENEMY_TYPE_DEMON?DEMON_ART_HEIGHT:
                e->type==ENEMY_TYPE_SKELETON?SKELETON_ART_HEIGHT:
                e->type==ENEMY_TYPE_CULTIST?CULTIST_ART_HEIGHT:34;
        if (anim==MH_IDLE || anim==MH_WALK) h=MALAKOR_ART_HEIGHT;
        if (anim==VK_IDLE || anim==VK_WALK) h=VARKHUL_ART_HEIGHT;
        if (malakor_combat_art(anim)) h=MALAKOR_COMBAT_ART_HEIGHT;
        if(anim>=VK_SCYTHE && anim<=VK_DIE)h=VARKHUL_ART_HEIGHT;
        if (e->elite) h*=1.3f;
        s_next_shadow=(e->state==ENEMY_STATE_DEAD)?0.0f:e->radius*2.7f;
        int before=draw_count;
        s_next_occl=1;
        emit(vp,anim,dir,fr,flip,e->x,e->y,e->z,h);
        if (boss_cardinal_art(anim) && draw_count>before)
            draws[draw_count-1].y+=BOSS_MOVEMENT_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if (malakor_combat_art(anim) && draw_count>before)
            draws[draw_count-1].y+=MALAKOR_COMBAT_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if(anim>=VK_SCYTHE && anim<=VK_DIE && draw_count>before)draws[draw_count-1].y+=16*draws[draw_count-1].scale;
        if (anim==SK_ATTACK && draw_count>before)
            draws[draw_count-1].y+=SKELETON_ATTACK_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if (anim==DE_WALK && draw_count>before)
            draws[draw_count-1].y+=DEMON_WALK_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if (anim==DE_ATTACK && draw_count>before)
            draws[draw_count-1].y+=DEMON_ATTACK_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if (anim==DE_SLAM && draw_count>before)
            draws[draw_count-1].y+=DEMON_SLAM_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if (anim==DE_DEATH)
            video_art_size(before,DEMON_LEGACY_SCALE,DEMON_LEGACY_FOOT_DROP);
        if (anim==SK_HURT)
            video_art_size(before,SKELETON_LEGACY_SCALE,SKELETON_LEGACY_FOOT_DROP);
        if (anim==CU_ATTACK && draw_count>before)
            draws[draw_count-1].y+=CULTIST_CAST_DRAW_Y_SHIFT*draws[draw_count-1].scale;
        if (anim==CU_DEATH)
            video_art_size(before,CULTIST_LEGACY_SCALE,CULTIST_LEGACY_FOOT_DROP);
        if (e->elite && draw_count>before) draws[draw_count-1].tint=0xFF8C78FFu; // guardiao: tom rubro
        uint32_t mt=world_monster_tint();
        if (e->type==ENEMY_TYPE_BOSS_MALAKOR && campaign_boss_art()) mt=0;   // arte propria: sem tinta
        if (mt && draw_count>before) {   // campanha da floresta: monstros cor de musgo
            uint32_t base=draws[draw_count-1].tint?draws[draw_count-1].tint:0xFFFFFFFFu;
            uint32_t r=((base>>24)&255)*((mt>>24)&255)/255, g=((base>>16)&255)*((mt>>16)&255)/255, b=((base>>8)&255)*((mt>>8)&255)/255;
            draws[draw_count-1].tint=(r<<24)|(g<<16)|(b<<8)|255u;
        }
        if(e->type==ENEMY_TYPE_BOSS_MALAKOR && g_campaign==0 && e->boss_phase==2)add_vk_trails(vp,e,before);
    }
}
// Three short procedural wisps reuse the existing fire-glow texture.
// No particle pool, extra texture, or per-frame allocations.
// A fixed, non-damaging warning perimeter; impact itself uses the separate art.
static void add_varkhul_fx(T3DViewport *vp) {
    Enemy *e=enemies_get_boss();if(g_campaign!=0 || !e || e->boss_phase!=2 || e->state==ENEMY_STATE_DEAD)return;
    const VarkhulCombatFX *fx=enemies_get_varkhul_fx();
    bool warning=e->boss_move==VARKHUL_DROP &&
        (e->state==ENEMY_STATE_TELEGRAPH || e->state==ENEMY_STATE_ATTACK);
    if(warning && s_glow_fire)for(int i=0;i<20 && draw_count<DRAW_MAX-1;i++) {
        float angle=i*6.2831853f/20;
        float x=fx->x+cosf(angle)*VARKHUL_DROP_RADIUS,z=fx->z+sinf(angle)*VARKHUL_DROP_RADIUS;
        T3DVec3 pos;t3d_viewport_calc_viewspace_pos(vp,&pos,&(T3DVec3){{x,2,z}});
        draws[draw_count++]=(Draw){.anim=MH_PULL,.x=pos.v[0],.y=pos.v[1],
            .scale=ref_px_height(7)/60,.order=pos.v[1]-.5f,.direct=s_glow_fire,
            .centered=true,.tint=0xFF6A30C0u,.wx=x,.wz=z,.wy=2,.occl=1};
    }
    if(fx->impact>0) {
        int fr=(int)((VARKHUL_IMPACT_TIME-fx->impact)/(.15f));if(fr>3)fr=3;
        sprite_t *spr=frame_sprite(FX_VK_IMPACT,0,fr);if(!spr)return;
        T3DVec3 pos;t3d_viewport_calc_viewspace_pos(vp,&pos,&(T3DVec3){{fx->x,1,fx->z}});
        if(draw_count>=DRAW_MAX)return;
        draws[draw_count++]=(Draw){.anim=FX_VK_IMPACT,.dir=0,.frame=fr,
            .x=pos.v[0],.y=pos.v[1],.scale=2*VARKHUL_DROP_RADIUS*s_ref_ppu/110,
            .order=pos.v[1]-.5f,.direct=spr,.centered=true,
            .wx=fx->x,.wz=fx->z,.wy=1,.occl=1};
    }
}

static void add_malakor_suction(T3DViewport *vp) {
    if(g_campaign!=0 || !s_glow_fire || g_druid.hp<=0 ||
       g_druid.state==DRUID_STATE_DODGE || g_druid.leap_timer>0.0f)return;
    Enemy *e=enemies_get_boss();
    if(!e || e->boss_phase!=1 || e->boss_move!=MALAKOR_MOVE_PULL || e->state!=ENEMY_STATE_ATTACK)return;
    float dist=vec2_dist(e->x,e->z,g_druid.x,g_druid.z);
    if(dist>MALAKOR_PULL_RANGE || dist<=e->radius+g_druid.radius+3.0f)return;
    T3DVec3 a,b;
    t3d_viewport_calc_viewspace_pos(vp,&a,&(T3DVec3){{g_druid.x,g_druid.y+FOOT_LIFT,g_druid.z}});
    t3d_viewport_calc_viewspace_pos(vp,&b,&(T3DVec3){{e->x,e->y+FOOT_LIFT,e->z}});
    a.v[1]-=ref_px_height(22.0f);b.v[1]-=ref_px_height(52.0f);
    float dx=b.v[0]-a.v[0],dy=b.v[1]-a.v[1],length=sqrtf(dx*dx+dy*dy);
    if(length<8.0f)return;
    float nx=-dy/length,ny=dx/length,now=(float)get_ticks_ms()*.001f;
    for(int i=0;i<3 && draw_count<DRAW_MAX-1;i++) {
        float u=fmodf(now*1.15f+(float)i/3.0f,1.0f),v=fminf(u+.12f,.99f);
        float bend=(float)(i-1)*ref_px_height(5.0f);
        float x0=a.v[0]+dx*u+nx*sinf(u*3.14159265f)*bend;
        float y0=a.v[1]+dy*u+ny*sinf(u*3.14159265f)*bend;
        float x1=a.v[0]+dx*v+nx*sinf(v*3.14159265f)*bend;
        float y1=a.v[1]+dy*v+ny*sinf(v*3.14159265f)*bend;
        float sx=x1-x0,sy=y1-y0,span=sqrtf(sx*sx+sy*sy);
        if(span<1.5f)continue;
        float t=(u+v)*.5f;
        uint32_t alpha=(uint32_t)(180.0f*sinf(t*3.14159265f));
        Draw *d=&draws[draw_count++];
        *d=(Draw){.anim=MH_PULL,.x=(x0+x1)*.5f,.y=(y0+y1)*.5f,
            .scale=span/(float)s_glow_fire->width,.order=(1-t)*a.v[1]+t*b.v[1],
            .direct=s_glow_fire,.centered=true,.tint=0xFF503000u|alpha,
            .wx=g_druid.x+(e->x-g_druid.x)*t,.wz=g_druid.z+(e->z-g_druid.z)*t,
            .wy=(1-t)*(g_druid.y+22.0f)+t*(e->y+52.0f),.occl=1,
            .rotation=(int16_t)lrintf(atan2f(-sy,sx)*4096.0f)};
    }
}

static int16_t cultist_skull_rotation(float dx, float dy, bool *flip) {
    *flip=dx<0.0f;
    float angle=atan2f(-dy,fabsf(dx));if(*flip)angle=-angle;
    return (int16_t)lroundf(angle*4096.0f);
}
static void add_spells(T3DViewport *vp) {
    add_campaign_fx(vp);
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
         if(p->type==PROJ_MAGE_ORB || p->type==PROJ_ROGUE_KNIFE || (p->type==PROJ_ARROW && !p->is_enemy)) {
             int art=p->type==PROJ_MAGE_ORB?HART_FX_MAGO_FX_ORBE_NEGRA:p->type==PROJ_ROGUE_KNIFE?HART_LADINA_EFEITOS_ADAGA_ARREMESSADA:
                p->pierce?HART_CACADORA_EFEITOS_VIROTE_PERFURANTE:HART_FX_CACADORA_FX_FLECHAS_TRIPLAS;
            int frame=(int)(get_ticks_ms()/85)%12;int before=draw_count;
            emit_hero_art(vp,art,0,frame,false,p->x,p->y,p->z,p->type==PROJ_MAGE_ORB?24:30,true);
            if(draw_count>before) {
                T3DVec3 a,b;t3d_viewport_calc_viewspace_pos(vp,&a,&(T3DVec3){{p->x,p->y,p->z}});
                t3d_viewport_calc_viewspace_pos(vp,&b,&(T3DVec3){{p->x+p->vx*.1f,p->y,p->z+p->vz*.1f}});
                Draw *d=&draws[draw_count-1];d->rotation=cultist_skull_rotation(b.v[0]-a.v[0],b.v[1]-a.v[1],&d->flip);
            }
            continue;
        }
        if (p->type==PROJ_MALAKOR_STONE_WAVE) {
            int fr=(int)(fmaxf(0.0f,MALAKOR_WAVE_LIFETIME-p->life)/MALAKOR_WAVE_FRAME_SECONDS)%4;
            int before=draw_count;
            emit(vp,FX_MH_WAVE,0,fr,false,p->x,p->y+10.0f,p->z,40.0f);
            if(draw_count>before) {
                T3DVec3 a,b;
                t3d_viewport_calc_viewspace_pos(vp,&a,&(T3DVec3){{p->x,p->y,p->z}});
                t3d_viewport_calc_viewspace_pos(vp,&b,&(T3DVec3){{p->x+p->vx*.1f,p->y,p->z+p->vz*.1f}});
                Draw *d=&draws[draw_count-1];d->centered=true;
                d->rotation=cultist_skull_rotation(b.v[0]-a.v[0],b.v[1]-a.v[1],&d->flip);
                d->scale=clampf(ref_px_height(MALAKOR_WAVE_WORLD_WIDTH)/96.0f,.16f,2.2f);
            }
            continue;
        }
        if (p->type==PROJ_CULTIST_SKULL) {
            int fr=(int)(fmaxf(0.0f,CULTIST_SKULL_LIFETIME-p->life)/CULTIST_SKULL_FRAME_SECONDS)%4;
            int before=draw_count;
            emit(vp,FX_CULTIST_SKULL,0,fr,false,p->x,p->y,p->z,20);
            if (draw_count>before) {
                T3DVec3 a,b;
                t3d_viewport_calc_viewspace_pos(vp,&a,&(T3DVec3){{p->x,p->y,p->z}});
                t3d_viewport_calc_viewspace_pos(vp,&b,&(T3DVec3){{p->x+p->vx*.1f,p->y,p->z+p->vz*.1f}});
                float dx=b.v[0]-a.v[0],dy=b.v[1]-a.v[1];
                Draw *d=&draws[draw_count-1];d->centered=true;
                d->rotation=cultist_skull_rotation(dx,dy,&d->flip);
                d->scale=clampf(ref_px_height(28.0f)/80.0f,.16f,2.2f);
            }
            continue;
        }
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
    const MageOrbImpact *impact=spells_get_mage_impacts();
    for(int i=0;i<MAX_MAGE_IMPACTS;i++)if(impact[i].life>0) {
        int before=draw_count;
        emit_hero_art(vp,HART_FX_MAGO_FX_IMPACTO_ORBE_AREA,0,once_frame(1-impact[i].life/1.02f,12),false,impact[i].x,0,impact[i].z,64,true);
        if(draw_count>before)draws[draw_count-1].order-=.1f;
    }
    const EnemyHealEffect *heal=enemies_get_heal_effects();
    const Enemy *allies=enemies_get_array();
    for(int i=0;i<MAX_ENEMY_HEAL_EFFECTS;i++) {
        if(heal[i].life<=0.0f || heal[i].target<0 || heal[i].target>=MAX_ENEMIES)continue;
        const Enemy *target=&allies[heal[i].target];
        if(!target->active || target->hp<=0 || target->state==ENEMY_STATE_DEAD)continue;
        int fr=(int)(fmaxf(0.0f,CULTIST_HEAL_EFFECT_LIFETIME-heal[i].life)/CULTIST_HEAL_FRAME_SECONDS);
        if(fr>3)fr=3;
        int before=draw_count;
        emit(vp,FX_CULTIST_HEAL,0,fr,false,target->x,target->y,target->z,38);
        if(draw_count>before) {
            draws[draw_count-1].y+=CULTIST_HEAL_DRAW_Y_SHIFT*draws[draw_count-1].scale;
            draws[draw_count-1].order+=1.0f;
        }
    }
    const RootTrap *roots=spells_get_roots();
    for(int i=0;i<MAX_ROOTS;i++) {
        const RootTrap *t=&roots[i]; if(!t->active)continue;
        // armada: so o circulo de runas (quadro 0); disparada: as raizes crescem (1..3) e seguram
        int fr=t->sprung?1+(int)(t->anim*8.0f):0; if(fr>3)fr=3;
        int before=draw_count;
        emit_hero_art(vp,HART_CACADORA_EFEITOS_ARMADILHA_PRENSORA,0,t->sprung?once_frame(t->anim/1.02f,12):0,false,t->x,0,t->z,30,true);
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
static int chest_opening_frame(const Chest *chest) {
    if(!chest->opened)return 0;
    int frame=1+(int)(fmaxf(chest->open_timer,0.0f)/CHEST_ART_FRAME_SECONDS);
    return frame>=CHEST_ART_FRAMES?CHEST_ART_FRAMES-1:frame;
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
            add_world_glow(d->type==DROP_CONTINUE?s_glow_gold:d->type==DROP_ESSENCE?s_glow_fire:s_glow_blue,f.v[0],f.v[1],(d->type==DROP_CONTINUE?46.0f:d->type==DROP_ESSENCE?22.0f:30.0f)*pulse,d->x,d->z);
        }
    }
    const Chest *chests=items_get_chests();
    for(int i=0;i<MAX_CHESTS;i++) {
        const Chest *c=&chests[i]; if(!c->active)continue;
        int fr=chest_opening_frame(c);
        if (!c->opened && s_glow_gold && in_view(c->x,c->z)) {
            // Bau fechado emite um brilho dourado pulsante: chama o jogador.
            T3DVec3 cf;
            t3d_viewport_calc_viewspace_pos(vp,&cf,&(T3DVec3){{c->x,0.0f,c->z}});
            // The lock still signals a mission, while every visual shares one size.
            float pulse=1.0f+0.14f*sinf((float)get_ticks_ms()*(c->locked?0.009f:0.005f));
            sprite_t *g=(c->locked&&s_glow_fire)?s_glow_fire:s_glow_gold;
            add_world_glow(g,cf.v[0],cf.v[1]-2.0f,46.0f*pulse,c->x,c->z);
        }
        static const Anim chest_art[]={CHEST,CHEST_STEEL,CHEST_QUEST};
        int kind=c->kind<=CHEST_KIND_QUEST?c->kind:CHEST_KIND_WOOD;
        s_next_shadow=30.0f;
        int before=draw_count;
        emit(vp,chest_art[kind],0,fr,false,c->x,0,c->z,CHEST_ART_HEIGHT);
        if(draw_count>before)draws[draw_count-1].y+=CHEST_ART_DRAW_Y_SHIFT*draws[draw_count-1].scale;
    }
}
static float wrap_range(float v, float lo, float hi) {
    float span=hi-lo;
    float m=fmodf(v-lo,span); if (m<0.0f) m+=span;
    return lo+m;
}
// Visao limitada: manchas de nevoa derivando e, por cima, a escuridao que fecha em volta do druida.
static void draw_vision_overlay(T3DViewport *vp) {
    // The circular darkness mask is disabled globally, including the crypt.
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
    // The modular cemetery uses the actual geometry depth, rather than a
    // rectangle inferred from the old wall grid.
    if (cemetery_stage_ready()) return 0;
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
static T3DViewport *s_depth_viewport;
static SpritePerf s_sprite_perf;
const SpritePerf *sprites_perf(void) {return &s_sprite_perf;}
static void count_blit_palette(sprite_t *sprite) {
    tex_format_t format=sprite_get_format(sprite);
    if((format==FMT_CI4 || format==FMT_CI8) && sprite_get_palette(sprite))s_sprite_perf.palette_uploads++;
}
static void blit_ground_draw(const Draw *d,sprite_t *spr) {
    // A decal is horizontal. A single Z value across a centered screen image
    // puts half the ritual under the ground on a tilted camera.
    float hx=spr->width*d->scale/(2*s_ref_ppu);
    float hz=hx*(float)spr->height/spr->width;
    // Height is normally resolved at placement, not by scanning terrain each
    // frame. The fallback also supports an upright prop changing into a decal.
    float y=d->wy>0?d->wy:cemetery_stage_ground_y(d->wx,d->wz,hx*2)+1;
    tex_format_t fmt=sprite_get_format(spr);
    rdpq_mode_push();rdpq_mode_zbuf(true,false);rdpq_mode_zoverride(false,0,0);
    rdpq_mode_persp(false);
    rdpq_mode_tlut(rdpq_tlut_from_format(fmt));
    uint16_t *pal=sprite_get_palette(spr);
    if(pal && (fmt==FMT_CI4 || fmt==FMT_CI8)) {
        rdpq_tex_upload_tlut(pal,0,fmt==FMT_CI4?16:256);s_sprite_perf.palette_uploads++;
    }
    surface_t pixels=sprite_get_pixels(spr);
    unsigned row_bytes=(TEX_FORMAT_PIX2BYTES(fmt,spr->width)+7u)&~7u;
    unsigned tmem=(fmt==FMT_CI4 || fmt==FMT_CI8)?2048u:4096u;
    int rows=tmem/row_bytes;if(rows<1)rows=1;if(rows>64)rows=64;
    for(int t0=0;t0<spr->height;t0+=rows) {
        int t1=t0+rows;if(t1>spr->height)t1=spr->height;
        float z0=d->wz+hz-2*hz*t0/spr->height,z1=d->wz+hz-2*hz*t1/spr->height;
        T3DVec3 points[4];
        T3DVec3 world[4]={{{d->wx+hx,y,z0}},{{d->wx-hx,y,z0}},{{d->wx-hx,y,z1}},{{d->wx+hx,y,z1}}};
        if(d->rotation) {
            float a=(float)d->rotation/4096.0f,c=cosf(a),s=sinf(a);
            for(int i=0;i<4;i++) {
                float x=world[i].v[0]-d->wx,z=world[i].v[2]-d->wz;
                world[i].v[0]=d->wx+x*c-z*s;
                world[i].v[2]=d->wz+x*s+z*c;
            }
        }
        float verts[4][6];
        for(int i=0;i<4;i++) {
            t3d_viewport_calc_viewspace_pos(s_depth_viewport,&points[i],&world[i]);
            verts[i][0]=points[i].v[0];verts[i][1]=points[i].v[1];
            verts[i][2]=clampf(points[i].v[2]*.5f+.5f-1.0f/32768.0f,0,1);
            verts[i][3]=(i==1 || i==2)?spr->width:0;
            verts[i][4]=i<2?t0:t1;verts[i][5]=1;
        }
        rdpq_tex_upload_sub(TILE0,&pixels,NULL,0,t0,spr->width,t1);
        rdpq_triangle(&TRIFMT_ZBUF_TEX,verts[0],verts[1],verts[2]);
        rdpq_triangle(&TRIFMT_ZBUF_TEX,verts[0],verts[2],verts[3]);
        s_sprite_perf.texture_tiles++;s_sprite_perf.triangles+=2;
        s_sprite_perf.uploaded_bytes+=row_bytes*(t1-t0);
    }
    rdpq_mode_pop();
}
static void blit_draw(const Draw *d, sprite_t *spr) {
    bool suction_wisp=d->direct && d->anim==MH_PULL;
    if(d->centered && d->direct && d->occl && s_depth_viewport && !suction_wisp) {blit_ground_draw(d,spr);return;}
    float width_scale=d->scale*(d->anim==DR_MOVE_BACK?DRUID_BACK_WIDTH_BOOST:1.0f);
    rdpq_blitparms_t bp={.cx=spr->width/2,.cy=d->centered?spr->height/2:spr->height,
        .scale_x=width_scale,.scale_y=suction_wisp?fmaxf(1.0f,ref_px_height(1.4f))/(float)spr->height:d->scale,
        .flip_x=d->flip,.theta=(float)d->rotation/4096.0f};
    if (!cemetery_stage_ready() || !d->occl || !s_depth_viewport) {
        s_sprite_perf.blits++;
        count_blit_palette(spr);
        rdpq_sprite_blit(spr,d->x,d->y,&bp);return;
    }
    T3DVec3 foot,top;
    t3d_viewport_calc_viewspace_pos(s_depth_viewport,&foot,&(T3DVec3){{d->wx,d->wy,d->wz}});
    if(d->centered) {
        s_sprite_perf.blits++;count_blit_palette(spr);
        rdpq_mode_zoverride(true,clampf(foot.v[2]*.5f+.5f-1.0f/32768.0f,0,1),0);
        rdpq_sprite_blit(spr,d->x,d->y,&bp);return;
    }
    t3d_viewport_calc_viewspace_pos(s_depth_viewport,&top,&(T3DVec3){{d->wx,d->wy+64,d->wz}});
    float dy=top.v[1]-foot.v[1];
    float slope=fabsf(dy)>0.001f?(top.v[2]-foot.v[2])/dy:0;
    // One upright rectangle with interpolated Z. Subdivide only where texture
    // memory requires it, never into arbitrary four-screen-pixel depth bands.
    // This keeps animation canvas, anchors, flip, painted appearance and tint.
    tex_format_t fmt=sprite_get_format(spr);
    rdpq_mode_tlut(rdpq_tlut_from_format(fmt));
    uint16_t *palette=sprite_get_palette(spr);
    if(palette && (fmt==FMT_CI4 || fmt==FMT_CI8)) {
        rdpq_tex_upload_tlut(palette,0,fmt==FMT_CI4?16:256);
        s_sprite_perf.palette_uploads++;
    }
    surface_t pixels=sprite_get_pixels(spr);
    unsigned row_bytes=(TEX_FORMAT_PIX2BYTES(fmt,spr->width)+7u)&~7u;
    unsigned tmem=(fmt==FMT_CI4 || fmt==FMT_CI8)?2048u:4096u;
    int rows=tmem/row_bytes;
    if(rows<1)rows=1;
    if(rows>64)rows=64;
    float x0=d->x-(spr->width/2)*width_scale;
    float x1=x0+spr->width*width_scale;
    if(x1<=0 || x0>=SCREEN_WIDTH)return;
    if(d->flip){float tmp=x0;x0=x1;x1=tmp;}
    rdpq_mode_zoverride(false,0,0);
    rdpq_mode_persp(false); // Screen artwork stays affine; only depth varies.
    for(int t0=0;t0<spr->height;t0+=rows) {
        int t1=t0+rows;if(t1>spr->height)t1=spr->height;
        float y0=d->y+(t0-spr->height)*d->scale;
        float y1=d->y+(t1-spr->height)*d->scale;
        if(y1<=0 || y0>=SCREEN_HEIGHT)continue;
        float z0=clampf((foot.v[2]+(y0-foot.v[1])*slope)*0.5f+0.5f-1.0f/32768.0f,0,1);
        float z1=clampf((foot.v[2]+(y1-foot.v[1])*slope)*0.5f+0.5f-1.0f/32768.0f,0,1);
        rdpq_tex_upload_sub(TILE0,&pixels,NULL,0,t0,spr->width,t1);
        float a[6]={x0,y0,z0,0,t0,1},b[6]={x1,y0,z0,spr->width,t0,1};
        float c[6]={x1,y1,z1,spr->width,t1,1},e[6]={x0,y1,z1,0,t1,1};
        rdpq_triangle(&TRIFMT_ZBUF_TEX,a,b,c);rdpq_triangle(&TRIFMT_ZBUF_TEX,a,c,e);
        s_sprite_perf.texture_tiles++;s_sprite_perf.triangles+=2;
        s_sprite_perf.uploaded_bytes+=row_bytes*(t1-t0);
    }
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
    memset(&s_sprite_perf,0,sizeof(s_sprite_perf));
    s_frame_loads=0;
    if(!ready)return;
    uint64_t queued_at=get_ticks();
    s_depth_viewport=vp;
    if (g_hero!=HERO_BARBARIAN) clear_barb_trails();
    Enemy *vk=enemies_get_boss();
    if(g_campaign!=0 || !vk || vk->boss_phase!=2 || vk->state==ENEMY_STATE_DEAD)clear_vk_trails();
    frame_number++; draw_count=0;
    // Dinamicos primeiro: personagem, inimigos e itens nunca perdem vaga para o cenario.
    add_druid(vp); add_enemies(vp); add_spells(vp); add_malakor_suction(vp); add_varkhul_fx(vp); add_items(vp); add_scenery(vp);
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
        // Increased sampling density must never increase the actor's world size.
        // All directions/frames share an action canvas and registered foot root.
        const SpriteResolution *resolution=&sprite_resolution[d->anim][d->dir];
        if(resolution->density && !(d->direct && d->anim==MH_PULL)) {
            d->scale/=resolution->density;
            float anchor=d->centered?(resolution->top+resolution->bottom)*.5f:resolution->bottom;
            d->y+=(anchor-resolution->old_height*resolution->density*(d->centered?.5f:1.0f))*d->scale;
            if(d->centered && d->direct && d->occl && d->anim!=MH_PULL) {
                float middle=(resolution->top+resolution->bottom)*.5f-resolution->old_height*resolution->density*.5f;
                d->wz-=middle*d->scale/s_ref_ppu;
            }
        }
        sprite_t *spr=d->direct?d->direct:frame_sprite(d->anim,d->dir,d->frame);
        if(!spr)continue;
        s_sprite_perf.draws++;
        if (cemetery_stage_ready()) {
            if(d->occl==2) {
                // Paint the silhouette first, then let the depth-tested normal
                // sprite cover it wherever the actual wall does not occlude it.
                // Use precisely the same triangle rasterization as the normal
                // sprite. The ghost only owns fully opaque texels: otherwise
                // bilinear edge alpha leaves a permanent green fringe behind
                // the normal image even where no wall covers the character.
                rdpq_mode_push();
                rdpq_mode_zbuf(false,false);rdpq_mode_zoverride(false,0,0);
                rdpq_mode_combiner(RDPQ_COMBINER1((ZERO,ZERO,ZERO,PRIM),(TEX0,ZERO,PRIM,ZERO)));
                rdpq_set_prim_color(RGBA32(150,255,185,150));
                // Leave two integer levels for the combiner's fixed-point
                // multiplication; bilinear fringe alpha still fails this cut.
                rdpq_mode_alphacompare(148);
                Draw silhouette=*d;
                blit_draw(&silhouette,spr);
                rdpq_mode_pop();
            }
            if (d->occl) {
                rdpq_mode_zbuf(true,false);
            } else {
                rdpq_mode_zoverride(false,0,0);rdpq_mode_zbuf(false,false);
            }
        }
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
        if (d->occl && !(d->centered && d->direct)) {
            float width_scale=d->scale*(d->anim==DR_MOVE_BACK?DRUID_BACK_WIDTH_BOOST:1.0f);
            float cx=(float)(spr->width/2)*width_scale;
            sp.x0=(int)floorf(d->x-cx); sp.x1=(int)ceilf(d->x-cx+(float)spr->width*width_scale);
            if (d->centered) {   // decalques: desenhados centrados no ponto
                float hh=(float)spr->height*d->scale*0.5f;
                sp.y0=(int)floorf(d->y-hh); sp.y1=(int)ceilf(d->y+hh);
                if(d->rotation) {
                    float angle=(float)d->rotation/4096.0f,c=fabsf(cosf(angle)),s=fabsf(sinf(angle));
                    float hw=(c*spr->width*width_scale+s*spr->height*d->scale)*.5f;
                    float rh=(s*spr->width*width_scale+c*spr->height*d->scale)*.5f;
                    sp.x0=(int)floorf(d->x-hw);sp.x1=(int)ceilf(d->x+hw);
                    sp.y0=(int)floorf(d->y-rh);sp.y1=(int)ceilf(d->y+rh);
                }
            } else {
                sp.y0=(int)floorf(d->y-(float)spr->height*d->scale); sp.y1=(int)ceilf(d->y);
            }
            nocc=wall_occluders(vp,d,sp,occ);
        }
        if (nocc==0) {
            uint64_t begin=get_ticks();
            blit_draw(d,spr);
            uint32_t us=(get_ticks()-begin)*1000000/TICKS_PER_SECOND;
            if(d->direct)s_sprite_perf.scenery_us+=us;else s_sprite_perf.actors_us+=us;
            continue;
        }

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
    if (cemetery_stage_ready()) {
        rdpq_mode_zoverride(false,0,0);rdpq_mode_zbuf(false,false);
    }
    draw_vision_overlay(vp);
    s_sprite_perf.queue_us=(get_ticks()-queued_at)*1000000/TICKS_PER_SECOND;
    s_sprite_perf.frame_loads=s_frame_loads;
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
    if (!ready) return;
    frame_number++;
    Anim pa = hero == HERO_HUNTRESS ? HU_IDLE : (hero == HERO_BARBARIAN && barb_art()) ? BA_IDLE : DR_IDLE;
    int fr = (int)((float)get_ticks_ms() * 0.001f * (pa == DR_IDLE ? DRUID_IDLE_FPS : 5.0f * NF_PLAYER / 4.0f)) % NF_PLAYER;
    if (pa==BA_IDLE || pa==DR_IDLE) fr=video_idle_frame(pa==BA_IDLE?0:1,0,get_ticks_ms());
    int foot=-1;
    if(hero==HERO_ROGUE || hero==HERO_MAGE){
        const HeroApprovedArt *a=&hero_approved_art[hero==HERO_ROGUE?HART_LADINA_BASE_CONTACT:HART_MAGO_BASE_CONTACT];
        pa=(Anim)a->anim;fr=0;foot=a->floor;
    }
    sprite_t *spr = frame_sprite(pa, 0, fr);
    if (!spr) return;
    const SpriteResolution *resolution=&sprite_resolution[pa][0];
    if(resolution->density) {
        int old_foot=foot>=0?foot:resolution->old_height;
        foot=old_foot*resolution->density-resolution->top;
        scale/=resolution->density;
    }
    rdpq_set_mode_standard();
    if (dark) { rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT); rdpq_set_prim_color(RGBA32(14, 10, 10, 255)); }   // silhueta
    else rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    rdpq_mode_alphacompare(1);
    rdpq_blitparms_t bp = { .cx = spr->width / 2, .cy = foot>=0?foot:spr->height, .scale_x = scale, .scale_y = scale };
    rdpq_sprite_blit(spr, x, y, &bp);
}
