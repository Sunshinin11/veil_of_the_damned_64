#define LANG_IMPL
#include "lang.h"

int g_lang = LANG_EN;

const char *T(StrId id) {
    if ((int)id < 0 || id >= S_COUNT) return "";
    int l = (g_lang >= 0 && g_lang < LANG_COUNT) ? g_lang : 0;
    return LANG_TABLE[id][l];
}

const char *area_name(int area)     { return T((StrId)(S_AREA0 + area * 2)); }
const char *area_subtitle(int area) { return T((StrId)(S_AREA0S + area * 2)); }
