// GERADO por tools/gen_chunks.py - edite la e rode de novo.
#pragma once
// Pedaco = 36 caracteres (6x6, linha a linha). Categorias: N normal, D beco, G guardiao, S inicio, X saida.
typedef struct { const char (*list[5])[37]; int count[5]; } ChunkSet;

static const char CHK_CEM_N[9][37] = {
    "TX..RKX.......s..........T..XTRK..T.",
    "R....R.T..XT.........S...X..T.K....R",
    "RR..KRR.......Z......a..T....XXT..TR",
    "U....U.o......v......s......o.T.Y..X",
    "JJ..JJTX..XT..S.........XT..TXJJ..JJ",
    "RF...R~.......k......Z........RK..YR",
    "KR..RKRT......a.....s.......XRRK..KR",
    "XU...T.o............S.......o.T...UX",
    "RK..RRR.......b.s.......RT...KRT..RR",
};
static const char CHK_CEM_D[3][37] = {
    "RC..CRR.......s.........T....XRK..KR",
    "U.C..U.Y......a..........T..X.RR..RR",
    "JC..CJJ.o...........S...T....XRR..RR",
};
static const char CHK_CEM_G[3][37] = {
    "U....U.T..X...q..........X..T.RF...R",
    "RX..TRR..o.R..v......q..K....KRR..RR",
    "FU...F.........q....o...T....XRK..KR",
};
static const char CHK_CEM_S[2][37] = {
    "R....R.F..F..........E....~...RY..KR",
    "KR..RKRF.............E..Y.....RR..RR",
};
static const char CHK_CEM_X[2][37] = {
    "UF...U...............P........TU...X",
    "RF..FR.........P........X....TRR..RR",
};
static const ChunkSet CHK_CEM_SET = { { CHK_CEM_N, CHK_CEM_D, CHK_CEM_G, CHK_CEM_S, CHK_CEM_X }, { 9, 3, 3, 2, 2 } };

static const char CHK_CRYPT_N[7][37] = {
    "L....L.V......s.............H.L....L",
    "QF...Q........S..........I..M.H....V",
    "LL..LLL.......Z......a........LV..HL",
    "I....M.k......v......s......k.M.Y..I",
    "Q....QQ.......S..............QQ....Q",
    "FL...F........a......Z........HL...W",
    "VB..WVH.......k.....s...M.....VH..BV",
};
static const char CHK_CRYPT_D[2][37] = {
    "LC..CLL.......s.........H....VLL..LL",
    "QC..CQ.Y......a..........I..M.LL..LL",
};
static const char CHK_CRYPT_G[3][37] = {
    "LU...L........q.........F....FL....L",
    "AF...L.........q....o...L....HLV..VL",
    "U....U.L..L...q..........L..L.Q....Q",
};
static const char CHK_CRYPT_S[2][37] = {
    "L....L.F..F..........E........H....V",
    "LL..LLL....L..k......E........VH..HV",
};
static const char CHK_CRYPT_X[2][37] = {
    "LF...L...............P........FL...F",
    "LF..FL...............P..I....MLL..LL",
};
static const ChunkSet CHK_CRYPT_SET = { { CHK_CRYPT_N, CHK_CRYPT_D, CHK_CRYPT_G, CHK_CRYPT_S, CHK_CRYPT_X }, { 7, 2, 3, 2, 2 } };

static const char CHK_FOREST_N[8][37] = {
    "RR..KRR.......s.........K....RRH..RR",
    "G....R...............S..H....KRK..RR",
    "RK..HRK.......Z......a..R....VRR..KR",
    "R.v..RH.............s.......HKRK..YR",
    "RG...RR.......a......S..V.....KR..RR",
    "HR..RHR....K..k......Z..K....RRR..RR",
    "R....GKH......o.....s.........RV..KR",
    "RK..RRR.......b......S..~....RRK..RR",
};
static const char CHK_FOREST_D[3][37] = {
    "RC..CRR.......s.........H....KRR..RR",
    "GC...RR....C..a.........K....VRR..RR",
    "RC..HRK.Y.....S.........R....KRR..RR",
};
static const char CHK_FOREST_G[3][37] = {
    "U....U.R..K...q..........H..R.RF...R",
    "RG...RR..o.K..v......q..K....HRR..RR",
    "FU...F.........q....o...R....KRK..KR",
};
static const char CHK_FOREST_S[2][37] = {
    "R....R.F..F..........E....~...RY..KR",
    "KR..RKRF.............E..Y.....RR..RR",
};
static const char CHK_FOREST_X[2][37] = {
    "UF...U...............P........RU...R",
    "RF..FR.........P........K....RRR..RR",
};
static const ChunkSet CHK_FOREST_SET = { { CHK_FOREST_N, CHK_FOREST_D, CHK_FOREST_G, CHK_FOREST_S, CHK_FOREST_X }, { 8, 3, 3, 2, 2 } };

static const char CHK_SWAMP_N[8][37] = {
    "RK..WRK....W..s.........w....KRH..KR",
    "R....R.W..l..........S..K.m..wRK..RR",
    "WW..RKW.......Z......a..R....HKR..WW",
    "R.g..RH....w........s.......HKRK..YR",
    "RW...RK....B..a......S..V.....KR..RW",
    "HR..RHR....K..k......Z..w....RRR..WR",
    "R....WKH......l.....s.........RV..KR",
    "RK..RRR....h..g......S..~....RRK..WR",
};
static const char CHK_SWAMP_D[3][37] = {
    "RC..CRR.......s.........H....wRR..RR",
    "WC...RW....C..a.........K....VRR..RR",
    "RC..HRK.Y.....S.........R....WRR..WW",
};
static const char CHK_SWAMP_G[3][37] = {
    "UW...U.R..K...q..........H..R.RF...R",
    "RW...RR..m.K..g......q..K....HRR..RR",
    "FU...F.........q....m...R....WRK..KR",
};
static const char CHK_SWAMP_S[2][37] = {
    "R....R.F..F..........E....~...RY..KR",
    "KR..RKRF.............E..Y....WRR..RR",
};
static const char CHK_SWAMP_X[2][37] = {
    "UF...U...............P........RU...R",
    "RF..FR.........P........W....KRR..RR",
};
static const ChunkSet CHK_SWAMP_SET = { { CHK_SWAMP_N, CHK_SWAMP_D, CHK_SWAMP_G, CHK_SWAMP_S, CHK_SWAMP_X }, { 8, 3, 3, 2, 2 } };

static const char CHK_SNOW_N[8][37] = {
    "RK..VRR....I..s.........K....dRV..KR",
    "R....R.U..t..........S..KI...VRK..RR",
    "RR..RKR....I..Z......a..V....KKR..RR",
    "R.d..RK....~........s.......KVRK..YR",
    "JJ..JJRV......a......S..B....KJJ..JJ",
    "KR..RKR....K..t......Z..I....RRR..VR",
    "R....VKI......d.....s.........RV..KR",
    "RK..RRR....N..b......S..~....RRK..IR",
};
static const char CHK_SNOW_D[3][37] = {
    "RC..CRR.......s.........K....VRR..RR",
    "VC...RI....C..a.........K....BRR..RR",
    "RC..KRK.Y.....S.........R....IRR..RR",
};
static const char CHK_SNOW_G[3][37] = {
    "UN...U.R..K...q..........V..R.RF...R",
    "RI...RR..d.K..t......q..K....VRR..RR",
    "FU...F.........q....d...R....IRK..KR",
};
static const char CHK_SNOW_S[2][37] = {
    "R....R.F..F..........E....~...RY..KR",
    "KR..RKRF.............E..B....IRR..RR",
};
static const char CHK_SNOW_X[2][37] = {
    "UF...U...............P........RU...R",
    "RF..FR.........P........I....KRR..RR",
};
static const ChunkSet CHK_SNOW_SET = { { CHK_SNOW_N, CHK_SNOW_D, CHK_SNOW_G, CHK_SNOW_S, CHK_SNOW_X }, { 8, 3, 3, 2, 2 } };

static const char CHK_ICE_N[7][37] = {
    "L....L.V......s.............H.L....L",
    "QF...Q........S..........I..M.H....V",
    "LL..LLL.......Z......a........LV..HL",
    "I....M.k......o......s......k.MB...I",
    "w....Qw.......S..............wQ....w",
    "FL...F........a......Z........HL...W",
    "VB..WVH.......g.....s...M.....VH..BV",
};
static const char CHK_ICE_D[2][37] = {
    "LC..CLL.......s.........H....VLL..LL",
    "QC..CQ.B......a..........I..M.LL..LL",
};
static const char CHK_ICE_G[3][37] = {
    "LQ...L........q.........F....FL....L",
    "AF...L.........q....o...L....HLV..VL",
    "Q....Q.L..L...q..........L..L.w....w",
};
static const char CHK_ICE_S[2][37] = {
    "L....L.F..F..........E........H....V",
    "LL..LLL....L..k......E........VH..HV",
};
static const char CHK_ICE_X[2][37] = {
    "LF...L...............P........FL...F",
    "LF..FL...............P..I....MLL..LL",
};
static const ChunkSet CHK_ICE_SET = { { CHK_ICE_N, CHK_ICE_D, CHK_ICE_G, CHK_ICE_S, CHK_ICE_X }, { 7, 2, 3, 2, 2 } };

