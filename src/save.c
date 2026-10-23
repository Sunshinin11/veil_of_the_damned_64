#include "save.h"
#include "test_flags.h"
#include "lang.h"
#include "druid.h"
#include "cards.h"
#include "audio.h"
#include "gfx.h"

#define SAVE_MAGIC 0x56454931u   // "VEI1"

typedef struct {
    uint32_t magic;
    uint8_t  area, level, continues, ncards;
    int32_t  exp, exp_next, attack;
    uint8_t  potions, vials, ess_lo, ess_hi;   // essencia demoniaca (16 bits, antes era padding)
    uint8_t  cards[MAX_OWNED_CARDS];
} SaveGame;

typedef struct {
    uint32_t magic;
    uint8_t  lang, fog, music, sfx, controls, pad[3];
} SaveOpts;

GameOptions g_opt = { LANG_EN, 1, 8, 8, 0 };
// Progresso: bits dos herois liberados + dificuldades vencidas por campanha (2 bits cada).
// Fica nos bytes livres do arquivo de opcoes, assim o layout da EEPROM nao muda (saves antigos valem).
static uint8_t s_heroes = (1u << HERO_BARBARIAN) | (1u << HERO_MAGE);
static uint8_t s_beaten = 0;
static uint8_t s_all = 0;                       // atalho de teste: tudo liberado
static bool s_eeprom = false;

static const eepfs_entry_t SAVE_FILES[] = {
    // inicializacao por nome: funciona com qualquer versao da libdragon (campos extras ficam zerados)
    { .path = "/game.sav", .size = sizeof(SaveGame), .checksum = true },   // checksum detecta save corrompido
    { .path = "/opts.sav", .size = sizeof(SaveOpts), .checksum = true },
};

void options_apply(void) {
    g_lang = g_opt.lang;
    g_dbg_fog = g_opt.fog != 0;
    sound_set_volumes(g_opt.music / 10.0f, g_opt.sfx / 10.0f);
}

void save_init(void) {
    s_eeprom = eeprom_present() != EEPROM_NONE;
    if (s_eeprom) {
        if (eepfs_init(SAVE_FILES, 2) != EEPFS_ESUCCESS) s_eeprom = false;
        else if (!eepfs_verify_signature()) eepfs_wipe();
    }
    if (s_eeprom) {
        SaveOpts o;
        if (eepfs_read("/opts.sav", &o, sizeof(o)) == EEPFS_ESUCCESS && o.magic == SAVE_MAGIC) {
            g_opt.lang = o.lang % LANG_COUNT; g_opt.fog = o.fog ? 1 : 0;
            g_opt.music = o.music > 10 ? 10 : o.music; g_opt.sfx = o.sfx > 10 ? 10 : o.sfx;
            g_opt.controls = o.controls ? 1 : 0;
            // bit 7 = progresso no formato novo. Saves antigos (antes da arte do Barbaro)
            // gravavam o Druida aberto por padrao: sem o bit, o Druida volta a ficar travado.
            s_heroes = (o.pad[0] & 0x7F) | (1u << HERO_BARBARIAN) | (1u << HERO_MAGE);
            if (!(o.pad[0] & 0x80)) s_heroes &= (uint8_t)~(1u << HERO_DRUID);
            s_beaten = o.pad[1];
            s_all = o.pad[2] == 0xA5;
        }
    }
    options_apply();
}

bool save_available(void) { return s_eeprom; }

bool save_has_game(void) {
    if (!s_eeprom) return false;
    SaveGame g;
    return eepfs_read("/game.sav", &g, sizeof(g)) == EEPFS_ESUCCESS && g.magic == SAVE_MAGIC;
}

bool save_write_game(int area) {
    if (!s_eeprom) return false;
    SaveGame g;
    memset(&g, 0, sizeof(g));
    g.magic = SAVE_MAGIC;
    g.area = hero_pack_location(area,g_campaign,g_hero);
    g.level = (uint8_t)g_druid.level;
    g.continues = (uint8_t)g_druid.continues;
    g.exp = g_druid.exp; g.exp_next = g_druid.exp_next; g.attack = g_druid.attack_damage;
    g.potions = (uint8_t)g_druid.health_potions; g.vials = (uint8_t)g_druid.spirit_potions;
    int ess = g_druid.essence < 0 ? 0 : g_druid.essence > 65535 ? 65535 : g_druid.essence;
    g.ess_lo = (uint8_t)(ess & 0xFF); g.ess_hi = (uint8_t)(ess >> 8);
    int n = cards_owned_count();
    g.ncards = (uint8_t)(n | (g_difficulty << 4) | (g_seeds << 6));   // cartas (0..4) + dificuldade + sementes
    for (int i = 0; i < n && i < MAX_OWNED_CARDS; i++) g.cards[i] = (uint8_t)cards_owned_id(i);
    return eepfs_write("/game.sav", &g, sizeof(g)) == EEPFS_ESUCCESS;
}

bool save_load_game(int *area) {
    if (!s_eeprom) return false;
    SaveGame g;
    if (eepfs_read("/game.sav", &g, sizeof(g)) != EEPFS_ESUCCESS || g.magic != SAVE_MAGIC) return false;
    int loaded_hero=hero_unpack_location(g.area);
    if (loaded_hero>=HERO_COUNT) loaded_hero=HERO_DRUID;
    if (!hero_has_art(loaded_hero)) return false;
    g_hero=loaded_hero;
    g_campaign = (g.area >> 2) & 3; if (g_campaign >= CAMPAIGN_COUNT) g_campaign = 0;
    g.area &= 3;
    g_difficulty = (g.ncards >> 4) & 3; if (g_difficulty >= DIFF_COUNT) g_difficulty = DIFF_NORMAL;
    g_seeds = (g.ncards >> 6) & 3;
    g.ncards &= 15;
    druid_init();
    g_druid.level = g.level ? g.level : 1;
    g_druid.continues = g.continues;
    g_druid.exp = g.exp; g_druid.exp_next = g.exp_next > 0 ? g.exp_next : 100;
    // dano da arma recalculado pela classe (saves antigos tinham o mesmo dano para todos)
    g_druid.attack_damage = hero_stats()->dmg + hero_stats()->dmg_lv * (g_druid.level - 1);
    g_druid.health_potions = g.potions; g_druid.spirit_potions = g.vials;
    g_druid.essence = g.ess_lo | (g.ess_hi << 8);
    int ids[MAX_OWNED_CARDS], n = g.ncards > MAX_OWNED_CARDS ? MAX_OWNED_CARDS : g.ncards;
    for (int i = 0; i < n; i++) ids[i] = g.cards[i];
    cards_set_owned(ids, n);              // tambem recalcula vida/espirito maximos
    g_druid.hp = g_druid.max_hp;
    g_druid.spirit = g_hero == HERO_BARBARIAN ? 0.0f : g_druid.max_spirit;   // furia comeca vazia
    *area = g.area < AREA_COUNT ? g.area : 0;
    return true;
}

void save_write_options(void) {
    options_apply();
    if (!s_eeprom) return;
    SaveOpts o;
    memset(&o, 0, sizeof(o));
    o.magic = SAVE_MAGIC;
    o.lang = (uint8_t)g_opt.lang; o.fog = (uint8_t)g_opt.fog;
    o.music = (uint8_t)g_opt.music; o.sfx = (uint8_t)g_opt.sfx; o.controls = (uint8_t)g_opt.controls;
    o.pad[0] = s_heroes | 0x80; o.pad[1] = s_beaten; o.pad[2] = s_all ? 0xA5 : 0;
    eepfs_write("/opts.sav", &o, sizeof(o));
}

// ---- Progresso ----
int campaign_beaten(int c) { return (c < 0 || c >= CAMPAIGN_COUNT) ? 0 : (s_beaten >> (c * 2)) & 3; }
bool hero_unlocked(int h) {
    if (!hero_has_art(h)) return false; // never bypass the rogue art gate, including unlock-all
    return h==HERO_MAGE || s_all || (TEST_UNLOCK_DRUID && h==HERO_DRUID) || (s_heroes&(1u<<h));
}
// Ordem da historia: Norte Esquecido (2) -> Floresta Morta (1) -> Caminho dos Condenados (0).
static const int CAMP_ORDER[CAMPAIGN_COUNT] = { 2, 1, 0 };
bool campaign_unlocked(int c) {
    if (s_all || c == 2) return true;                            // o Norte esta sempre aberto
    for (int i = 1; i < CAMPAIGN_COUNT; i++)
        if (CAMP_ORDER[i] == c) return campaign_beaten(CAMP_ORDER[i - 1]) >= 1;   // zerar a anterior abre a proxima
    return false;
}
int campaign_max_difficulty(int c) {
    if (s_all) return DIFF_HELL;
    int b = campaign_beaten(c);
    return b > DIFF_HELL ? DIFF_HELL : b;
}
void progress_unlock_hero(int h) {
    if (h < 0 || h >= HERO_COUNT || (s_heroes & (1u << h))) return;
    s_heroes |= (uint8_t)(1u << h);
    save_write_options();
}
void progress_beat(int c, int d) {
    if (c < 0 || c >= CAMPAIGN_COUNT) return;
    int b = campaign_beaten(c);
    if (d + 1 <= b) return;
    s_beaten = (uint8_t)((s_beaten & ~(3u << (c * 2))) | ((unsigned)(d + 1) << (c * 2)));
    save_write_options();
}
void progress_toggle_all(void) { s_all = !s_all; save_write_options(); }
