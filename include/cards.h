#ifndef CARDS_H
#define CARDS_H
#include "common.h"

// Cartas (estilo Ravenswatch): cada bau aberto da uma carta aleatoria. Todas as cartas coletadas ficam
// ativas e mudam habilidades e atributos. Algumas sao AMALDICOADAS: um bonus forte com um custo.
typedef enum { RARITY_COMMON, RARITY_RARE, RARITY_LEGENDARY, RARITY_CURSED, RARITY_COUNT } CardRarity;

typedef struct {
    const char *name;      // nome no idioma atual
    CardRarity rarity;
    const char *desc;      // linha 1
    const char *desc2;     // linha 2 (pode ser "")
} CardDef;

// Modificadores somados de todas as cartas
typedef struct {
    float all_pct, melee_pct, bolt_pct, thorn_pct, wolf_pct, beast_pct;   // dano (+0.15 = +15%)
    int   bolt_extra;          // projeteis extras
    bool  bolt_pierce;
    float bolt_cost;           // somado ao custo (negativo = mais barato)
    float thorn_radius_pct;
    int   wolf_extra;
    float wolf_life;           // segundos extras
    float beast_time;          // segundos extras
    float speed_pct;
    int   max_hp_add;
    float max_hp_pct;          // multiplicador de vida (negativo = menos)
    float max_mana_add;
    float regen_pct;
    float potion_pct;
    float lifesteal;           // fracao do dano corpo a corpo que vira vida
    float crit;                // chance de critico (dano x2)
    float armor;               // reducao de dano recebido (negativo = recebe mais)
    // Cacadora
    float root_time;           // segundos extras das raizes
    int   trap_extra;          // armadilhas extras ao mesmo tempo
    float poison;              // veneno das flechas (dano por segundo, fracao do dano da flecha)
    float camo_time;           // segundos extras do manto
    float camo_cost;           // somado ao custo do manto
    float aim_jitter;          // maldicao: flecha sai torta (radianos)
    // Barbaro
    float heavy_pct;           // dano do 3o golpe do combo
    float leap_pct;            // dano do Salto Esmagador
    float leap_radius_pct;     // area do Salto
    bool  shout_dmg;           // o grito tambem machuca
    float shout_heal;          // o grito cura (fracao da vida maxima)
    bool  no_dodge;            // maldicao: sem esquiva
} CardMods;

#define CARD_COUNT 72            // 0-23 druida, 24-47 cacadora, 48-71 barbaro
#define CARDS_PER_HERO 24
#define MAX_OWNED_CARDS 24
// 4 espacos, um por TIPO de carta (Ataque, Magia, Fera, Corpo): obriga a montar uma build.
#define CARD_SLOTS 4
typedef enum { CTYPE_ATTACK, CTYPE_SPELL, CTYPE_BEAST, CTYPE_BODY } CardType;
CardType cards_type(int id);
int  cards_slot_card(int type);               // carta equipada no espaco do tipo (-1 = vazio)

extern CardMods g_mods;

void cards_reset(void);                       // novo jogo: sem cartas
// Sorteio: a carta NAO e equipada sozinha: ela vai para a fila de ofertas; o jogo pausa e o
// jogador escolhe pegar (cards_take) ou deixar.
int  cards_roll_weighted(int w_common, int w_rare, int w_legend, int w_cursed);   // id ou -1
void cards_offer(int id);                     // poe na fila de ofertas
int  cards_offer_pending(void);               // id da proxima oferta ou -1
void cards_offer_resolve(bool take);          // pega ou deixa a oferta atual
int  cards_offer_occupant(void);              // carta que a oferta substituiria (-1 = espaco livre)
void cards_take(int id);
void cards_set_owned(const int *ids, int n);  // carregar save
int  cards_grant_random(void);                // (compat) bau: oferta com pesos do bau
// oferta com pesos por raridade (comum, rara, lendaria, amaldicoada) - chefes usam pesos melhores
int  cards_grant_weighted(int w_common, int w_rare, int w_legend, int w_cursed);
int  cards_owned_count(void);
int  cards_owned_id(int i);
const CardDef *cards_def(int id);
void cards_refresh_stats(void);               // recalcula vida/espirito maximos (nivel + cartas)
int  cards_roll_damage(int base, float pct);  // aplica % de dano e critico
// Cores por raridade (moldura/texto)
color_t cards_rarity_color(CardRarity r);
const char *cards_rarity_name(CardRarity r);
#endif
