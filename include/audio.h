#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"

typedef enum {
    SFX_SWING,
    SFX_SPIRIT_BOLT_CAST,
    SFX_SPIRIT_BOLT_HIT,
    SFX_THORN_BURST,
    SFX_WOLF_HOWL,
    SFX_WOLF_BITE,
    SFX_SHAPESHIFT,
    SFX_POTION,
    SFX_PLAYER_HURT,
    SFX_ENEMY_HURT,
    SFX_ENEMY_DIE,
    SFX_BOSS_SLAM,
    SFX_BOSS_ENRAGE,
    SFX_VICTORY,
    SFX_BEAST_SWING,
    SFX_SKELETON_SWING,
    SFX_DEMON_SWING,
    SFX_CULTIST_CAST,
    SFX_DARK_ORB_CAST,
    SFX_WOLF_SUMMON,
    SFX_DODGE,
    SFX_SKELETON_HURT,
    SFX_DEMON_HURT,
    SFX_CULTIST_HURT,
    SFX_BOSS_HURT,
    SFX_SKELETON_DIE,
    SFX_DEMON_DIE,
    SFX_CULTIST_DIE,
    SFX_BOSS_DIE,
    SFX_CHEST,
    SFX_PICKUP,
    SFX_UI_SELECT,
    SFX_UI_UNAVAILABLE,
    SFX_BARB_SHOUT,
    SFX_BARB_BERSERK,
    SFX_BARB_AXE,
    SFX_COUNT
} SoundEffect;

void sound_init(void);
void sound_play(SoundEffect sfx);
void sound_update(void);
void sound_set_context(GameState state);
void sound_set_volumes(float music, float sfx);   // 0..1 (Opcoes)
void sound_pump(void);
void sound_close(void);

#endif // AUDIO_H
