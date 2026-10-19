#ifndef MENU_H
#define MENU_H
#include "common.h"

// Menus: tela inicial (fundo com efeito Ken Burns + logo), escolha de personagem e pausa.
typedef enum {
    MENU_NONE, MENU_NEW_GAME, MENU_LOAD, MENU_OPTIONS,
    MENU_CONFIRM, MENU_BACK, MENU_RESUME, MENU_SAVE, MENU_QUIT
} MenuAction;

void menu_init(void);
void menu_reset_cursor(void);

MenuAction menu_title_update(const joypad_inputs_t *in, const joypad_buttons_t *pressed, int *start_area);
void menu_title_draw(int start_area);

// start_area: fase inicial (seletor de teste com L/R ou C-esq/C-dir)
MenuAction menu_char_update(const joypad_inputs_t *in, const joypad_buttons_t *pressed, int *start_area);
void menu_char_draw(int start_area);
int  menu_selected_hero(void);
int  menu_selected_campaign(void);
int  menu_selected_difficulty(void);

MenuAction menu_pause_update(const joypad_inputs_t *in, const joypad_buttons_t *pressed);
void menu_pause_draw(void);
// Aviso no menu de pausa depois de salvar
void menu_notice(bool saved);
// Oferta de carta (jogo pausado): A pega, B deixa. MENU_CONFIRM = resolvida, MENU_BACK = nao ha oferta
MenuAction menu_offer_update(const joypad_inputs_t *in, const joypad_buttons_t *pressed);
void menu_offer_draw(void);
// Mascate: MENU_BACK = saiu, MENU_CONFIRM = comprou carta (mostrar a oferta)
void menu_shop_open(void);
MenuAction menu_shop_update(const joypad_inputs_t *in, const joypad_buttons_t *pressed);
void menu_shop_draw(void);
#endif
