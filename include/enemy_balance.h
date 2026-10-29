#ifndef ENEMY_BALANCE_H
#define ENEMY_BALANCE_H
enum { EB_SLOW,EB_NORMAL,EB_FAST };
enum { EV_STANDARD,EV_MINIBOSS,EV_THRONE,EV_PRIEST,EV_CHEST,EV_CHAMPION,EV_COURIER };
typedef struct {int hp,damage;unsigned char movement,attack;} EnemyBalance;
static inline EnemyBalance enemy_balance(int campaign,int role) {
    static const EnemyBalance table[5][6]={
        {{325,16,EB_NORMAL,EB_NORMAL},{900,30,EB_SLOW,EB_SLOW},{150,14,EB_SLOW,EB_SLOW},{10000,55,EB_SLOW,EB_SLOW},{8000,50,EB_FAST,EB_FAST},{175,18,EB_SLOW,EB_SLOW}},
        {{300,16,EB_NORMAL,EB_NORMAL},{650,28,EB_SLOW,EB_SLOW},{175,18,EB_SLOW,EB_NORMAL},{5000,35,EB_NORMAL,EB_NORMAL},{14000,60,EB_SLOW,EB_SLOW},{175,18,EB_SLOW,EB_SLOW}},
        {{350,16,EB_NORMAL,EB_NORMAL},{550,24,EB_FAST,EB_FAST},{175,18,EB_SLOW,EB_NORMAL},{6000,40,EB_SLOW,EB_NORMAL},{8000,50,EB_NORMAL,EB_NORMAL},{175,18,EB_SLOW,EB_SLOW}},
        {{400,18,EB_SLOW,EB_NORMAL},{1000,32,EB_SLOW,EB_SLOW},{450,22,EB_FAST,EB_NORMAL},{8000,45,EB_SLOW,EB_NORMAL},{12000,60,EB_SLOW,EB_SLOW},{175,18,EB_SLOW,EB_SLOW}},
        {{300,15,EB_FAST,EB_FAST},{700,30,EB_SLOW,EB_SLOW},{150,18,EB_NORMAL,EB_NORMAL},{7000,45,EB_NORMAL,EB_NORMAL},{12000,60,EB_NORMAL,EB_FAST},{175,18,EB_SLOW,EB_SLOW}}
    };
    return table[campaign>=0 && campaign<5?campaign:0][role>=0 && role<6?role:0];
}
static inline float enemy_move_speed(unsigned char category) {
    const float values[3]={30,50,72};return values[category<3?category:1];
}
static inline float enemy_attack_windup(unsigned char category) {
    const float values[3]={1.0f,.55f,.32f};return values[category<3?category:1];
}
static inline float enemy_attack_recovery(unsigned char category) {
    const float values[3]={1.10f,.65f,.40f};return values[category<3?category:1];
}
static inline float enemy_attack_cooldown(unsigned char category) {
    const float values[3]={1.4f,.90f,.50f};return values[category<3?category:1];
}
#define PACK_STRONG_LIMIT 3
#define PACK_SUPPORT_LIMIT 2
#define ACTIVE_STRONG_LIMIT 6
#define ACTIVE_SUPPORT_LIMIT 4
#endif
