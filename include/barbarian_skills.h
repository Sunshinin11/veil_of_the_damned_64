#ifndef BARBARIAN_SKILLS_H
#define BARBARIAN_SKILLS_H
#include <math.h>
#define BARB_LEAP_TIME 0.96f
#define BARB_LEAP_TAKEOFF 0.16f
#define BARB_LEAP_IMPACT 0.64f
#define BARB_SKILL_FRAME_TIME 0.08f
#define BARB_WHIRLWIND_FRAME_TIME 0.04f
static inline int barb_leap_frame(float elapsed) {
    int f=(int)(elapsed/BARB_SKILL_FRAME_TIME);
    return f<0?0:f>11?11:f;
}
static inline float barb_leap_travel(float elapsed) {
    float t=(elapsed-BARB_LEAP_TAKEOFF)/(BARB_LEAP_IMPACT-BARB_LEAP_TAKEOFF);
    return t<0?0:t>1?1:t;
}
static inline int barb_whirlwind_frame(float elapsed) {
    int f=(int)(elapsed/BARB_WHIRLWIND_FRAME_TIME);
    return f<0?0:f%12;
}
#endif
