#ifndef DEMON_COMBAT_H
#define DEMON_COMBAT_H

// Reuse the existing special-move fields; no per-enemy allocation or particles.
enum { DEMON_MOVE_NORMAL=0, DEMON_MOVE_SLAM=1 };
#define DEMON_BASE_HP 900
#define DEMON_NORMAL_WINDUP 0.90f
#define DEMON_NORMAL_WINDUP_MIN 0.80f
#define DEMON_NORMAL_REACH 24.0f
#define DEMON_NORMAL_ACTIVE 0.25f
#define DEMON_NORMAL_RECOVERY 0.85f
#define DEMON_NORMAL_COOLDOWN 1.80f
#define DEMON_NORMAL_PERCENT_CAP 12
#define DEMON_SLAM_WINDUP 1.40f
#define DEMON_SLAM_PREP_POSE 0.25f
#define DEMON_SLAM_TRIGGER_RANGE 54.0f
#define DEMON_SLAM_RADIUS 44.0f
#define DEMON_SLAM_ACTIVE 0.16f
#define DEMON_SLAM_RECOVERY 1.10f
#define DEMON_SLAM_RECOVERY_ART 0.30f
#define DEMON_SLAM_COOLDOWN 8.0f
#define DEMON_SLAM_PERCENT 30

// Use maximum HP, never current HP. Safe multiplication and a full-health
// non-one-shot bound, including unusually small HP values.
static inline int demon_percent_damage(int max_hp, int percent) {
    if (max_hp<2 || percent<=0) return 0;
    if (percent>30) percent=30;
    int amount=(max_hp/100)*percent+((max_hp%100)*percent)/100;
    if (amount<1) amount=1;
    return amount>=max_hp?max_hp-1:amount;
}
#endif
