#ifndef BARBARIAN_ATTACK_H
#define BARBARIAN_ATTACK_H
#include <stdbool.h>
#include <math.h>

#define BARB_SWING_TIME 0.24f
#define BARB_HEAVY_TIME 0.44f
#define BARB_COMBO_FRAMES 12

// Tempos relativos dos PNGs aprovados; o golpe rapido chega ao contato em 80 ms.
static inline float barb_combo_hit_phase(bool heavy) {
    return heavy ? 23.0f / 55.0f : 12.0f / 36.0f;
}
static inline int barb_combo_frame(float progress, bool heavy) {
    static const unsigned char light_end[12] = {4,7,10,12,14,16,18,20,23,27,31,36};
    static const unsigned char heavy_end[12] = {4,8,12,17,21,23,25,28,37,44,50,55};
    const unsigned char *end = heavy ? heavy_end : light_end;
    float position = progress * (heavy ? 55.0f : 36.0f);
    for (int f = 0; f < BARB_COMBO_FRAMES; f++) if (position < end[f]) return f;
    return BARB_COMBO_FRAMES - 1;
}
// Quatro artes completas, sem flip: S, E, N, W. A camera espelha X do mundo.
static inline int barb_combo_direction(float yaw) {
    float angle = atan2f(sinf(yaw), -cosf(yaw));
    int sector = (int)floorf((angle + 2.35619449f) / 1.57079633f);
    return (sector + 4) & 3;
}
#endif
