#ifndef VARKHUL_COMBAT_H
#define VARKHUL_COMBAT_H
enum { VARKHUL_SCYTHE=20, VARKHUL_DROP, VARKHUL_RUSH };
#define VARKHUL_WARNING 2.0f
#define VARKHUL_FALL .35f
#define VARKHUL_DROP_HEIGHT 70.0f
#define VARKHUL_DROP_RADIUS 44.0f
#define VARKHUL_DROP_RECOVERY 1.15f
#define VARKHUL_SCYTHE_WINDUP .40f
#define VARKHUL_SCYTHE_ACTIVE .20f
#define VARKHUL_SCYTHE_RECOVERY .40f
#define VARKHUL_SCYTHE_RANGE 54.0f
#define VARKHUL_RUSH_TIME 5.0f
#define VARKHUL_RUSH_MULT 1.50f
#define VARKHUL_SPEED 72.0f // fast category; runtime resolves the approved profile
#define VARKHUL_IMPACT_TIME .60f
typedef struct { float x,z,impact,rush; } VarkhulCombatFX;
const VarkhulCombatFX *enemies_get_varkhul_fx(void);
#endif
