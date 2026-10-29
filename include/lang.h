#ifndef LANG_H
#define LANG_H
#include "lang_strings.h"
#include <stdbool.h>

// Idiomas: ingles (padrao), portugues (BR), espanhol. Textos em tools/gen_lang.py.
enum { LANG_EN = 0, LANG_PT = 1, LANG_ES = 2, LANG_COUNT = 3 };
extern int g_lang;
const char *T(StrId id);
const char *area_name(int area);
const char *area_subtitle(int area);
const char *campaign_boss_name(int campaign,bool second_phase);
#endif
