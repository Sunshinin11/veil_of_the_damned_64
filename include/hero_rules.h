#ifndef HERO_RULES_H
#define HERO_RULES_H
#include <stdbool.h>
#include <stdint.h>

// Existing save IDs stay fixed. Append new classes at the end.
typedef enum {
    HERO_DRUID=0, HERO_HUNTRESS=1, HERO_BARBARIAN=2,
    HERO_MAGE=3, HERO_ROGUE=4, HERO_COUNT=5
} HeroId;

static inline bool hero_has_art(int hero) {
    return hero>=0 && hero<HERO_COUNT && hero!=HERO_ROGUE;
}
static inline int hero_card_family(int hero) {
    // Mage temporarily shares the druid kit and its card modifiers.
    if (hero==HERO_MAGE) return HERO_DRUID;
    if (hero==HERO_ROGUE) return HERO_HUNTRESS;
    return hero>=0 && hero<=HERO_BARBARIAN ? hero : HERO_DRUID;
}
static inline uint8_t hero_pack_location(int area,int campaign,int hero) {
    return (uint8_t)((area&3)|((campaign&3)<<2)|((hero&15)<<4));
}
static inline int hero_unpack_location(uint8_t location) {
    return (location>>4)&15;
}
#endif
