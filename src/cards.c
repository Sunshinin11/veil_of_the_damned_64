#include "cards.h"
#include "druid.h"
#include "lang.h"

CardMods g_mods;
static int s_owned[MAX_OWNED_CARDS];
static int s_owned_n = 0;

static const CardRarity RARITY[CARD_COUNT] = {
    RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON,
    RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE,
    RARITY_LEGENDARY, RARITY_LEGENDARY, RARITY_LEGENDARY,
    RARITY_CURSED, RARITY_CURSED, RARITY_CURSED,
    // cacadora (24-47): mesma distribuicao
    RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON,
    RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE,
    RARITY_LEGENDARY, RARITY_LEGENDARY, RARITY_LEGENDARY,
    RARITY_CURSED, RARITY_CURSED, RARITY_CURSED,
    // barbaro (48-71)
    RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON, RARITY_COMMON,
    RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE, RARITY_RARE,
    RARITY_LEGENDARY, RARITY_LEGENDARY, RARITY_LEGENDARY,
    RARITY_CURSED, RARITY_CURSED, RARITY_CURSED,
};
// Tipo (espaco) de cada carta
static const CardType TYPE[CARD_COUNT] = {
    CTYPE_ATTACK, CTYPE_SPELL, CTYPE_SPELL, CTYPE_BEAST, CTYPE_BODY, CTYPE_BODY, CTYPE_BODY, CTYPE_BODY,   // 0-7
    CTYPE_SPELL, CTYPE_SPELL, CTYPE_SPELL, CTYPE_BEAST, CTYPE_BEAST, CTYPE_ATTACK, CTYPE_ATTACK, CTYPE_BODY, // 8-15
    CTYPE_BODY, CTYPE_SPELL, CTYPE_ATTACK, CTYPE_BEAST, CTYPE_SPELL,                                       // 16-20
    CTYPE_ATTACK, CTYPE_BODY, CTYPE_BEAST,                                                                 // 21-23
    // cacadora: Ataque = Flecha, Magia = Armadilha/Manto, Fera = Gaviao/Foco, Corpo
    CTYPE_ATTACK, CTYPE_SPELL, CTYPE_SPELL, CTYPE_BEAST, CTYPE_BODY, CTYPE_BODY, CTYPE_BODY, CTYPE_BODY,   // 24-31
    CTYPE_ATTACK, CTYPE_ATTACK, CTYPE_SPELL, CTYPE_BEAST, CTYPE_BEAST, CTYPE_ATTACK, CTYPE_BEAST, CTYPE_BODY, // 32-39
    CTYPE_BODY, CTYPE_SPELL, CTYPE_ATTACK, CTYPE_BEAST, CTYPE_SPELL,                                       // 40-44
    CTYPE_ATTACK, CTYPE_BODY, CTYPE_BEAST,                                                                 // 45-47
    // barbaro: Ataque = Machado/Salto, Magia = Grito/Redemoinho, Fera = Furia/Berserker, Corpo
    CTYPE_ATTACK, CTYPE_SPELL, CTYPE_SPELL, CTYPE_BEAST, CTYPE_BODY, CTYPE_BODY, CTYPE_BODY, CTYPE_BODY,   // 48-55
    CTYPE_ATTACK, CTYPE_ATTACK, CTYPE_SPELL, CTYPE_SPELL, CTYPE_ATTACK, CTYPE_ATTACK, CTYPE_BEAST, CTYPE_BODY, // 56-63
    CTYPE_BODY, CTYPE_SPELL, CTYPE_ATTACK, CTYPE_BEAST, CTYPE_SPELL,                                       // 64-68
    CTYPE_ATTACK, CTYPE_BODY, CTYPE_BEAST,                                                                 // 69-71
};
// Nomes e descricoes vem da tabela de idiomas (S_C<id>, S_C<id>a, S_C<id>b)
static CardDef s_def;
static int s_offers[8];
static int s_offer_n = 0;

static void apply(int id, CardMods *m) {
    switch (id) {
        case 0:  m->melee_pct += .15f; break;
        case 1:  m->bolt_pct += .15f; break;
        case 2:  m->thorn_pct += .20f; break;
        case 3:  m->beast_time += 4.0f; break;
        case 4:  m->speed_pct += .08f; break;
        case 5:  m->max_hp_add += 25; break;
        case 6:  m->max_mana_add += 25.0f; break;
        case 7:  m->potion_pct += .35f; break;
        case 8:  m->bolt_extra += 1; break;
        case 9:  m->bolt_pierce = true; break;
        case 10: m->thorn_radius_pct += .35f; break;
        case 11: m->wolf_extra += 1; break;
        case 12: m->wolf_life += 8.0f; m->wolf_pct += .20f; break;
        case 13: m->lifesteal += .06f; break;
        case 14: m->crit += .10f; break;
        case 15: m->armor += .10f; break;
        case 16: m->regen_pct += .40f; break;
        case 17: m->bolt_cost -= 5.0f; break;
        case 18: m->all_pct += .30f; break;
        case 19: m->wolf_extra += 2; m->wolf_pct += .25f; break;
        case 20: m->thorn_pct += .50f; m->thorn_radius_pct += .25f; break;
        case 21: m->all_pct += .40f; m->max_hp_pct -= .30f; break;
        case 22: m->lifesteal += .12f; m->regen_pct -= .50f; break;
        case 23: m->beast_time += 8.0f; m->beast_pct += .30f; m->armor -= .15f; break;
        // ---- cacadora ----
        case 24: m->melee_pct += .15f; break;                              // Ponta Afiada
        case 25: m->thorn_pct += .25f; break;                              // Raizes Espinhosas
        case 26: m->root_time += 1.5f; break;                              // Laco Firme
        case 27: m->wolf_pct += .20f; break;                               // Pena de Gaviao
        case 28: m->speed_pct += .08f; break;                              // Botas de Couro
        case 29: m->max_hp_add += 25; break;                               // Couro Curtido
        case 30: m->max_mana_add += 25.0f; break;                          // Folego
        case 31: m->potion_pct += .35f; break;                             // Bolsa de Ervas
        case 32: m->bolt_extra += 2; break;                                // Rajada Larga
        case 33: m->bolt_pierce = true; break;                             // Flecha Perfurante
        case 34: m->trap_extra += 1; m->thorn_radius_pct += .30f; break;   // Armadilha Dupla
        case 35: m->wolf_extra += 1; break;                                // Bando de Gavioes
        case 36: m->wolf_life += 8.0f; m->wolf_pct += .20f; break;         // Gaviao Veterano
        case 37: m->poison += .35f; break;                                 // Ponta Envenenada
        case 38: m->crit += .10f; break;                                   // Olho de Aguia
        case 39: m->armor += .10f; break;                                  // Couraca de Casca
        case 40: m->regen_pct += .40f; break;                              // Seiva
        case 41: m->camo_time += 3.0f; m->camo_cost -= 10.0f; break;       // Manto de Folhas
        case 42: m->all_pct += .30f; break;                                // Cacadora Lendaria
        case 43: m->wolf_extra += 2; m->wolf_pct += .25f; break;           // Senhora dos Gavioes
        case 44: m->thorn_pct += .50f; m->thorn_radius_pct += .25f; m->root_time += 1.0f; break; // Floresta Viva
        case 45: m->all_pct += .40f; m->aim_jitter += .22f; break;         // Maos Tremulas (maldita)
        case 46: m->lifesteal += .10f; m->regen_pct -= .50f; break;        // Sede de Sangue (maldita)
        case 47: m->beast_time += 12.0f; m->beast_pct += .40f; m->armor -= .15f; break;   // Olho Frio (maldita)
        // ---- barbaro ----
        case 48: m->melee_pct += .15f; break;                                   // Fio do Machado
        case 49: m->thorn_radius_pct += .25f; break;                            // Garganta de Ferro (area do grito)
        case 50: m->root_time += 1.0f; break;                                   // Pulmoes de Urso (atordoa mais)
        case 51: m->beast_time += 4.0f; break;                                  // Sangue Quente (berserker)
        case 52: m->speed_pct += .08f; break;                                   // Botas de Pele
        case 53: m->max_hp_add += 25; break;                                    // Pele de Lobo
        case 54: m->max_mana_add += 25.0f; break;                               // Folego do Norte (furia maxima)
        case 55: m->potion_pct += .35f; break;                                  // Hidromel
        case 56: m->heavy_pct += .60f; break;                                   // Golpe Duplo
        case 57: m->leap_pct += .40f; m->leap_radius_pct += .30f; break;        // Terremoto
        case 58: m->shout_dmg = true; break;                                    // Grito Ensurdecedor
        case 59: m->wolf_pct += .30f; m->wolf_life += 1.0f; break;              // Redemoinho Sangrento
        case 60: m->bolt_cost -= 10.0f; break;                                  // Salto do Gigante
        case 61: m->lifesteal += .06f; break;                                   // Machado Faminto
        case 62: m->crit += .10f; break;                                        // Olho de Corvo
        case 63: m->armor += .10f; break;                                       // Escudo de Gelo
        case 64: m->armor += .08f; m->max_hp_add += 15; break;                  // Pele de Pedra
        case 65: m->shout_heal += .15f; break;                                  // Grito do Lobo Branco
        case 66: m->all_pct += .30f; break;                                     // Furia dos Ancestrais
        case 67: m->wolf_pct += .50f; m->beast_pct += .20f; break;              // Martelo do Trovao
        case 68: m->thorn_radius_pct += .40f; m->root_time += 1.5f; break;      // Inverno Eterno
        case 69: m->all_pct += .40f; m->no_dodge = true; break;                 // Furia Cega (maldita)
        case 70: m->lifesteal += .12f; m->max_hp_pct -= .20f; break;            // Pacto de Sangue (maldita)
        case 71: m->beast_time += 8.0f; m->beast_pct += .30f; m->armor -= .15f; break;   // Coracao Gelado (maldita)
        default: break;
    }
}

static void recompute(void) {
    memset(&g_mods, 0, sizeof(g_mods));
    for (int i = 0; i < s_owned_n; i++) apply(s_owned[i], &g_mods);
}

void cards_refresh_stats(void) {
    int lvl = g_druid.level > 0 ? g_druid.level : 1;
    const HeroStats *hs = hero_stats();
    int base_hp = hs->hp + hs->hp_lv * (lvl - 1);
    float base_sp = (float)(hs->sp + hs->sp_lv * (lvl - 1));
    int old_max = g_druid.max_hp;
    int hp = (int)((base_hp + g_mods.max_hp_add) * (1.0f + g_mods.max_hp_pct) * (1.0f - 0.10f * g_seeds));   // sementes pesam
    if (hp < 20) hp = 20;
    g_druid.max_hp = hp;
    if (hp > old_max) g_druid.hp += hp - old_max;          // ganhou vida maxima: ganha a diferenca
    if (g_druid.hp > g_druid.max_hp) g_druid.hp = g_druid.max_hp;
    g_druid.max_spirit = base_sp + g_mods.max_mana_add;
    if (g_druid.spirit > g_druid.max_spirit) g_druid.spirit = g_druid.max_spirit;
}

void cards_reset(void) {
    s_owned_n = 0;
    s_offer_n = 0;
    recompute();
}

static bool owned(int id) {
    for (int k = 0; k < s_owned_n; k++) if (s_owned[k] == id) return true;
    for (int k = 0; k < s_offer_n; k++) if (s_offers[k] == id) return true;   // ja oferecida agora
    return false;
}

int cards_roll_weighted(int w_common, int w_rare, int w_legend, int w_cursed) {
    int total = w_common + w_rare + w_legend + w_cursed;
    if (total <= 0) total = 1;
    int roll = rand() % total;
    CardRarity want = roll < w_common ? RARITY_COMMON :
                      roll < w_common + w_rare ? RARITY_RARE :
                      roll < w_common + w_rare + w_legend ? RARITY_LEGENDARY : RARITY_CURSED;
    for (int pass = 0; pass < 2; pass++) {             // se acabou a raridade, qualquer outra
        int pool[CARD_COUNT], n = 0;
        int first = hero_card_family(g_hero) * CARDS_PER_HERO;
        for (int id = first; id < first + CARDS_PER_HERO; id++) {
            if (pass == 0 && RARITY[id] != want) continue;
            if (!owned(id)) pool[n++] = id;
        }
        if (n > 0) return pool[rand() % n];
    }
    return -1;
}

void cards_take(int id) {
    if (id < 0 || id >= CARD_COUNT) return;
    for (int k = 0; k < s_owned_n; k++) if (s_owned[k] == id) return;
    bool placed = false;
    for (int k = 0; k < s_owned_n; k++)                  // mesmo tipo: troca a carta daquele espaco
        if (TYPE[s_owned[k]] == TYPE[id]) { s_owned[k] = id; placed = true; break; }
    if (!placed) {
        if (s_owned_n >= CARD_SLOTS) return;
        s_owned[s_owned_n++] = id;
    }
    recompute();
    cards_refresh_stats();
}

void cards_set_owned(const int *ids, int n) {
    s_owned_n = 0;
    s_offer_n = 0;
    for (int i = 0; i < n && s_owned_n < CARD_SLOTS; i++) {
        if (ids[i] < 0 || ids[i] >= CARD_COUNT) continue;
        bool dup = false;
        for (int k = 0; k < s_owned_n; k++) if (TYPE[s_owned[k]] == TYPE[ids[i]]) dup = true;
        if (!dup) s_owned[s_owned_n++] = ids[i];
    }
    recompute();
    cards_refresh_stats();
}

void cards_offer(int id) {
    if (id >= 0 && s_offer_n < 8) s_offers[s_offer_n++] = id;
}
int cards_offer_pending(void) { return s_offer_n > 0 ? s_offers[0] : -1; }
void cards_offer_resolve(bool take) {
    if (s_offer_n <= 0) return;
    int id = s_offers[0];
    for (int i = 1; i < s_offer_n; i++) s_offers[i - 1] = s_offers[i];
    s_offer_n--;
    if (take) cards_take(id);
}

CardType cards_type(int id) { return (id >= 0 && id < CARD_COUNT) ? TYPE[id] : CTYPE_BODY; }

int cards_slot_card(int type) {
    for (int k = 0; k < s_owned_n; k++) if ((int)TYPE[s_owned[k]] == type) return s_owned[k];
    return -1;
}

int cards_offer_occupant(void) {
    return s_offer_n > 0 ? cards_slot_card(TYPE[s_offers[0]]) : -1;
}

// Bau: 75% comum, 16% rara, 3% lendaria, 6% amaldicoada (raras eram frequentes demais).
int cards_grant_random(void) {
    if (g_difficulty == DIFF_HELL)      return cards_grant_weighted(52, 30, 10, 8);
    if (g_difficulty == DIFF_NIGHTMARE) return cards_grant_weighted(64, 24, 5, 7);
    return cards_grant_weighted(75, 16, 3, 6);
}
int cards_grant_weighted(int w_common, int w_rare, int w_legend, int w_cursed) {
    int id = cards_roll_weighted(w_common, w_rare, w_legend, w_cursed);
    cards_offer(id);
    return id;
}

int cards_owned_count(void) { return s_owned_n; }
int cards_owned_id(int i) { return (i >= 0 && i < s_owned_n) ? s_owned[i] : -1; }
const CardDef *cards_def(int id) {
    if (id < 0 || id >= CARD_COUNT) return NULL;
    s_def.name = T((StrId)(S_C0 + id * 3));
    s_def.desc = T((StrId)(S_C0a + id * 3));
    s_def.desc2 = T((StrId)(S_C0b + id * 3));
    s_def.rarity = RARITY[id];
    return &s_def;
}

int cards_roll_damage(int base, float pct) {
    float d = (float)base * (1.0f + pct + g_mods.all_pct);
    float crit = g_mods.crit + hero_crit();              // critico da classe + cartas
    if (crit > 0.0f && (float)(rand() % 1000) < crit * 1000.0f) d *= 2.0f;
    int r = (int)(d + 0.5f);
    return r < 1 ? 1 : r;
}

color_t cards_rarity_color(CardRarity r) {
    switch (r) {
        case RARITY_RARE:      return RGBA32(90, 150, 230, 255);
        case RARITY_LEGENDARY: return RGBA32(230, 170, 60, 255);
        case RARITY_CURSED:    return RGBA32(200, 30, 40, 255);
        default:               return RGBA32(170, 160, 140, 255);
    }
}

const char *cards_rarity_name(CardRarity r) {
    return T((StrId)(S_R_COMMON + (int)r));
}
