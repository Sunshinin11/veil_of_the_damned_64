#ifndef SAVE_H
#define SAVE_H
#include "common.h"

// Save no cartucho (EEPROM 16 kbit via eepromfs). Um save de partida + as opcoes.
typedef struct {
    int lang, fog, music, sfx, controls;   // music/sfx 0..10, controls 0 classico / 1 invertido
} GameOptions;
extern GameOptions g_opt;

void save_init(void);
bool save_available(void);          // existe EEPROM?
bool save_has_game(void);
bool save_write_game(int area);     // salva a partida (fase atual + druida + cartas)
bool save_load_game(int *area);     // carrega; o jogo recomeca no inicio da fase salva
bool save_write_next_campaign(int campaign);
bool save_forget_game(void);        // game over ends this run; permanent unlocks survive
void run_reset_progress(void);
void run_complete_campaign(int campaign);
bool run_campaign_completed(int campaign);
void save_write_options(void);
void options_apply(void);           // aplica idioma, neblina e volumes

// Progresso permanente (guardado junto das opcoes): herois liberados e dificuldades vencidas.
bool hero_unlocked(int hero);
bool campaign_unlocked(int campaign);
int  campaign_max_difficulty(int campaign);    // maior dificuldade selecionavel (0..2)
int  campaign_beaten(int campaign);            // quantas dificuldades ja venceu (0..3)
void progress_unlock_hero(int hero);
void progress_beat(int campaign, int difficulty);
void progress_toggle_all(void);                // atalho de teste (sem aviso no jogo)
#endif
