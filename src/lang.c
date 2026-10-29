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
const char *campaign_boss_name(int campaign,bool second_phase) {
    static const StrId ids[5][2]={{S_BOSS1,S_BOSS2},{S_BOSS1F,S_BOSS2F},{S_BOSS1N,S_BOSS2N},{S_BOSS1D,S_BOSS2D},{S_BOSS1V,S_BOSS2V}};
    return T(ids[campaign>=0 && campaign<5?campaign:0][second_phase?1:0]);
}
