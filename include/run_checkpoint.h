#ifndef RUN_CHECKPOINT_H
#define RUN_CHECKPOINT_H
#include <stdint.h>
// Use the obsolete saved weapon-damage word, not a larger EEPROM file.
// Old saves stored a small damage value here and still load normally.
#define RUN_META_MAGIC 0x56400000u
#define RUN_META_MASK 0xFFE00000u
static inline int run_ratio_byte(float value,float maximum) {
    if(maximum<=0 || value<=0)return 0;
    if(value>=maximum)return 255;
    return (int)(value*255.0f/maximum);
}
static inline uint32_t run_checkpoint_pack(unsigned completed,int hp,int max_hp,float spirit,float max_spirit) {
    unsigned h=(unsigned)run_ratio_byte((float)hp,(float)max_hp);
    return RUN_META_MAGIC|(completed&31u)|(h<<5)|((unsigned)run_ratio_byte(spirit,max_spirit)<<13);
}
static inline int run_checkpoint_current(uint32_t word) {return (word&RUN_META_MASK)==RUN_META_MAGIC;}
static inline unsigned run_checkpoint_completed(uint32_t word) {return word&31u;}
static inline int run_checkpoint_hp(uint32_t word,int maximum) {
    int hp=(int)((((word>>5)&255u)*(unsigned)maximum+254u)/255u);
    return hp<1?1:hp>maximum?maximum:hp;
}
static inline float run_checkpoint_spirit(uint32_t word,float maximum) {return (float)((word>>13)&255u)*maximum/255.0f;}
#endif
