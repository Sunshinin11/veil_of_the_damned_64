// Montador de mapas estilo Diablo II: layout aleatorio de salas + pedacos desenhados a mao.
// Nao depende da libdragon (da para testar no PC com tools/test_mapgen.c).
#include "mapgen.h"
#include "map_chunks.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MG_TILE 72.0f
enum { CN = 1, CE = 2, CS = 4, CW_ = 8 };
enum { R_NONE, R_NORMAL, R_DEAD, R_GUARD, R_START, R_EXIT };

typedef struct { uint8_t used, conn, role, branch; } Cell;

static char s_buf[MG_ROWS][MG_COLS + 1];
static const char *s_rows[MG_ROWS];
static Cell s_cell[MG_CH][MG_CW];
static FloorQuad s_floor[MG_MAX_FLOOR];
static int s_floor_n = 0;

static float mg_frand(void) { return (float)(rand() % 10000) / 10000.0f; }
static float mg_range(float a, float b) { return a + (b - a) * mg_frand(); }

static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
static const uint8_t BIT[4] = { CN, CE, CS, CW_ }, OPP[4] = { CS, CW_, CN, CE };

static void connect(int x, int y, int d) {
    s_cell[y][x].conn |= BIT[d];
    s_cell[y + DY[d]][x + DX[d]].conn |= OPP[d];
}
static bool inside(int x, int y) { return x >= 0 && y >= 0 && x < MG_CW && y < MG_CH; }
static int popcount4(uint8_t c) { return (c & 1) + ((c >> 1) & 1) + ((c >> 2) & 1) + ((c >> 3) & 1); }

// ---------------------------------------------------------------- layout
static void make_layout(int *sx, int *sy, int *ex, int *ey) {
    memset(s_cell, 0, sizeof(s_cell));
    int x = rand() % MG_CW, y = MG_CH - 1;
    *sx = x; *sy = y;
    s_cell[y][x].used = 1;
    int side_run = 0;
    while (y > 0) {                     // caminho principal: sobe, as vezes de lado
        int opts[2], n = 0;
        for (int d = 1; d <= 3; d += 2)
            if (inside(x + DX[d], y) && !s_cell[y][x + DX[d]].used && side_run < 2) opts[n++] = d;
        int d = 0;
        if (n > 0 && rand() % 100 < 50) { d = opts[rand() % n]; side_run++; }
        else side_run = 0;
        connect(x, y, d);
        x += DX[d]; y += DY[d];
        s_cell[y][x].used = 1;
    }
    *ex = x; *ey = y;
    // as vezes anda mais um pouco de lado no topo (a saida nao fica sempre logo acima)
    if (rand() % 100 < 45) {
        int d = (rand() & 1) ? 1 : 3;
        if (!inside(x + DX[d], y) || s_cell[y][x + DX[d]].used) d = 4 - d;
        if (inside(x + DX[d], y) && !s_cell[y][x + DX[d]].used) {
            connect(x, y, d); x += DX[d]; s_cell[y][x].used = 1; *ex = x;
        }
    }
    // becos (tesouro / guardiao) saindo do caminho
    for (int tries = 0, made = 0; tries < 40 && made < 4; tries++) {
        int bx = rand() % MG_CW, by = rand() % MG_CH;
        Cell *c = &s_cell[by][bx];
        if (!c->used || (bx == *ex && by == *ey)) continue;
        int d = rand() % 4;
        int nx = bx + DX[d], ny = by + DY[d];
        if (!inside(nx, ny) || s_cell[ny][nx].used) continue;
        connect(bx, by, d);
        s_cell[ny][nx].used = 1; s_cell[ny][nx].branch = 1;
        made++;
        if (rand() % 100 < 35) {        // beco de 2 salas
            int d2 = rand() % 4, mx = nx + DX[d2], my = ny + DY[d2];
            if (inside(mx, my) && !s_cell[my][mx].used) {
                connect(nx, ny, d2); s_cell[my][mx].used = 1; s_cell[my][mx].branch = 1;
            }
        }
    }
    // um ou outro atalho entre salas vizinhas (circuitos, nao so arvore)
    for (int y2 = 0; y2 < MG_CH; y2++) for (int x2 = 0; x2 < MG_CW; x2++) {
        if (!s_cell[y2][x2].used) continue;
        for (int d = 1; d <= 2; d++) {
            int nx = x2 + DX[d], ny = y2 + DY[d];
            if (inside(nx, ny) && s_cell[ny][nx].used && !(s_cell[y2][x2].conn & BIT[d]) && rand() % 100 < 22)
                connect(x2, y2, d);
        }
    }
}

static void assign_roles(int sx, int sy, int ex, int ey) {
    int dead[MG_CW * MG_CH][2], nd = 0, mid[MG_CW * MG_CH][2], nm = 0;
    for (int y = 0; y < MG_CH; y++) for (int x = 0; x < MG_CW; x++) {
        Cell *c = &s_cell[y][x];
        if (!c->used) continue;
        c->role = R_NORMAL;
        if ((x == sx && y == sy) || (x == ex && y == ey)) continue;
        if (popcount4(c->conn) == 1) { dead[nd][0] = x; dead[nd][1] = y; nd++; }
        else if (abs(x - sx) + abs(y - sy) > 1) { mid[nm][0] = x; mid[nm][1] = y; nm++; }
    }
    s_cell[sy][sx].role = R_START;
    s_cell[ey][ex].role = R_EXIT;
    // 3 guardioes: primeiro nos becos, depois no meio do caminho (longe do inicio)
    for (int i = nd - 1; i > 0; i--) { int j = rand() % (i + 1); int t0 = dead[i][0], t1 = dead[i][1]; dead[i][0] = dead[j][0]; dead[i][1] = dead[j][1]; dead[j][0] = t0; dead[j][1] = t1; }
    for (int i = nm - 1; i > 0; i--) { int j = rand() % (i + 1); int t0 = mid[i][0], t1 = mid[i][1]; mid[i][0] = mid[j][0]; mid[i][1] = mid[j][1]; mid[j][0] = t0; mid[j][1] = t1; }
    int g = 0;
    for (int i = 0; i < nd && g < 2; i++, g++) s_cell[dead[i][1]][dead[i][0]].role = R_GUARD;
    for (int i = 0; i < nm && g < 3; i++, g++) s_cell[mid[i][1]][mid[i][0]].role = R_GUARD;
    for (int y = 0; y < MG_CH && g < 3; y++) for (int x = 0; x < MG_CW && g < 3; x++)   // mapa curto: qualquer sala serve
        if (s_cell[y][x].used && s_cell[y][x].role == R_NORMAL) { s_cell[y][x].role = R_GUARD; g++; }
    for (int i = 0; i < nd; i++) if (s_cell[dead[i][1]][dead[i][0]].role == R_NORMAL) s_cell[dead[i][1]][dead[i][0]].role = R_DEAD;
}

// ---------------------------------------------------------------- grade de caracteres
static bool is_lane(int i) { return i == 2 || i == 3; }

static void stamp(const ChunkSet *set, int x, int y, bool crypt) {
    Cell *c = &s_cell[y][x];
    int cat = c->role == R_START ? 3 : c->role == R_EXIT ? 4 : c->role == R_GUARD ? 2 : c->role == R_DEAD ? 1 : 0;
    const char *t = set->list[cat][rand() % set->count[cat]];
    bool mx = rand() & 1;
    bool my = (cat < 3) && (rand() & 1);   // inicio/saida nao viram de cabeca para baixo (entrada ao sul, trono ao norte)
    (void)crypt;
    int x0 = x * MG_STEP + 1, y0 = y * MG_STEP + 1;
    for (int r = 0; r < 6; r++) for (int col = 0; col < 6; col++) {
        int sr = my ? 5 - r : r, sc = mx ? 5 - col : col;
        s_buf[y0 + r][x0 + col] = t[sr * 6 + sc];
    }
}

static void open_walls(bool crypt) {
    for (int y = 0; y < MG_CH; y++) for (int x = 0; x < MG_CW; x++) {
        Cell *c = &s_cell[y][x];
        if (!c->used) continue;
        int x0 = x * MG_STEP + 1, y0 = y * MG_STEP + 1;
        // cemiterio: passagens largas (as vezes o lado todo); cripta: tuneis de 2 tiles
        for (int side = 0; side < 2; side++) {
            if (!(c->conn & (side ? CS : CE))) continue;
            int a = crypt ? 2 : 1, b = crypt ? 3 : 4;
            if (!crypt && rand() % 100 < 35) { a = 0; b = 5; }
            for (int i = a; i <= b; i++) {
                if (side) s_buf[y0 + 6][x0 + i] = '.';
                else      s_buf[y0 + i][x0 + 6] = '.';
            }
        }
    }
}

// Borda de floresta (cemiterio) ou pedras (cripta) junto aos muros, fora das faixas.
static void decorate_edges(bool crypt) {
    for (int y = 0; y < MG_CH; y++) for (int x = 0; x < MG_CW; x++) {
        if (!s_cell[y][x].used) continue;
        int x0 = x * MG_STEP + 1, y0 = y * MG_STEP + 1;
        for (int r = 0; r < 6; r++) for (int c = 0; c < 6; c++) {
            if (is_lane(r) || is_lane(c)) continue;
            int gr = y0 + r, gc = x0 + c;
            if (s_buf[gr][gc] != '.') continue;
            bool wall = s_buf[gr - 1][gc] == '#' || s_buf[gr + 1][gc] == '#' || s_buf[gr][gc - 1] == '#' || s_buf[gr][gc + 1] == '#';
            if (!wall) continue;
            int roll = rand() % 100;
            if (crypt) { if (roll < 14) s_buf[gr][gc] = "VHBW"[rand() % 4]; }
            else if (roll < 42) s_buf[gr][gc] = roll < 32 ? 'R' : 'K';
        }
    }
}

// Garante pelo menos 5 vagas de bau (4 baus + mascate).
static void ensure_chests(void) {
    int n = 0;
    for (int r = 0; r < MG_ROWS; r++) for (int c = 0; c < MG_COLS; c++) if (s_buf[r][c] == 'C') n++;
    for (int tries = 0; n < 5 && tries < 400; tries++) {
        int x = rand() % MG_CW, y = rand() % MG_CH;
        Cell *cl = &s_cell[y][x];
        if (!cl->used || cl->role == R_START) continue;
        int r = rand() % 6, c = rand() % 6;
        if (is_lane(r) || is_lane(c)) continue;
        char *t = &s_buf[y * MG_STEP + 1 + r][x * MG_STEP + 1 + c];
        if (*t != '.' && *t != 'R' && *t != 'K') continue;
        *t = 'C'; n++;
    }
}

// ---------------------------------------------------------------- chao do cemiterio
static float s_hw, s_hh;
static void tile_center(float c, float r, float *x, float *z) { *x = (c + 0.5f) * MG_TILE - s_hw; *z = s_hh - (r + 0.5f) * MG_TILE; }

static void add_tri2(uint8_t kind, const float p[6][2]) {
    if (s_floor_n >= MG_MAX_FLOOR) return;
    FloorQuad *f = &s_floor[s_floor_n++];
    for (int i = 0; i < 6; i++) { f->x[i] = (int16_t)p[i][0]; f->z[i] = (int16_t)p[i][1]; }
    f->kind = kind;
}
static void add_quad(uint8_t kind, const float a[2], const float b[2], const float c[2], const float d[2]) {
    float p[6][2] = { {a[0], a[1]}, {b[0], b[1]}, {c[0], c[1]}, {a[0], a[1]}, {c[0], c[1]}, {d[0], d[1]} };
    add_tri2(kind, p);
}
static void blob(uint8_t kind, float cx, float cz, float radius, float seed, float squash) {
    enum { N = 10 };
    float ring[N][2];
    for (int i = 0; i < N; i++) {
        float a = (float)i / N * 6.2831853f;
        float rr = radius * (0.72f + 0.5f * (sinf(a * 2 + seed) * 0.5f + 0.5f) * (0.6f + 0.4f * sinf(a * 3 + seed * 1.3f)));
        ring[i][0] = cx + cosf(a) * rr; ring[i][1] = cz + sinf(a) * rr * squash;
    }
    for (int i = 0; i < N; i += 2) {
        float p[6][2] = { {cx, cz}, {ring[i][0], ring[i][1]}, {ring[(i + 1) % N][0], ring[(i + 1) % N][1]},
                          {cx, cz}, {ring[(i + 1) % N][0], ring[(i + 1) % N][1]}, {ring[(i + 2) % N][0], ring[(i + 2) % N][1]} };
        add_tri2(kind, p);
    }
}
// Faixa de trilha com bordas onduladas entre dois pontos (passando por um ponto do meio).
static uint8_t s_trail_kind = 0;   // 0 trilha de pedra (cemiterio), 1 terra batida (floresta)
static void trail(float ax, float az, float mx, float mz, float bx, float bz, float half_w, float seed) {
    enum { SEG = 14 };
    float L[SEG + 1][2], Rr[SEG + 1][2], P[SEG + 1][2];
    for (int i = 0; i <= SEG; i++) {     // curva quadratica de Bezier A-M-B
        float t = (float)i / SEG, u = 1.0f - t;
        P[i][0] = u * u * ax + 2 * u * t * mx + t * t * bx;
        P[i][1] = u * u * az + 2 * u * t * mz + t * t * bz;
    }
    float s = 0.0f;
    for (int i = 0; i <= SEG; i++) {
        int i0 = i > 0 ? i - 1 : 0, i1 = i < SEG ? i + 1 : SEG;
        float dx = P[i1][0] - P[i0][0], dz = P[i1][1] - P[i0][1], ln = sqrtf(dx * dx + dz * dz);
        if (ln < 0.001f) ln = 1.0f;
        float nx = -dz / ln, nz = dx / ln;
        if (i) s += sqrtf((P[i][0] - P[i - 1][0]) * (P[i][0] - P[i - 1][0]) + (P[i][1] - P[i - 1][1]) * (P[i][1] - P[i - 1][1]));
        float wl = half_w * (1 + 0.20f * sinf(s * 0.019f + seed) + 0.13f * sinf(s * 0.051f + seed * 1.7f));
        float wr = half_w * (1 + 0.20f * sinf(s * 0.019f + seed + 2.1f) + 0.13f * sinf(s * 0.051f + seed * 1.7f + 4.0f));
        L[i][0] = P[i][0] + nx * wl; L[i][1] = P[i][1] + nz * wl;
        Rr[i][0] = P[i][0] - nx * wr; Rr[i][1] = P[i][1] - nz * wr;
    }
    for (int i = 0; i < SEG; i++) add_quad(s_trail_kind, L[i], Rr[i], Rr[i + 1], L[i + 1]);
}

static void build_floor(void) {
    s_floor_n = 0;
    static FloorQuad tmp[MG_MAX_FLOOR];
    // grama e terra primeiro (camadas de baixo), depois trilha e lajes
    for (int y = 0; y < MG_CH; y++) for (int x = 0; x < MG_CW; x++) {
        if (!s_cell[y][x].used) continue;
        for (int k = 0; k < (s_trail_kind ? 3 : 2); k++) {   // floresta: mais grama morta
            float cx, cz;
            float oc = (rand() & 1) ? mg_range(0.3f, 1.4f) : mg_range(3.6f, 4.7f);
            float orr = (rand() & 1) ? mg_range(0.3f, 1.4f) : mg_range(3.6f, 4.7f);
            tile_center(x * MG_STEP + 1 + oc, y * MG_STEP + 1 + orr, &cx, &cz);
            blob(3, cx, cz, mg_range(46, 90), mg_range(0, 6), 0.85f);
        }
    }
    for (int r = 0; r < MG_ROWS; r++) for (int c = 0; c < MG_COLS; c++)
        if (s_buf[r][c] == 'o' || s_buf[r][c] == 'a') { float cx, cz; tile_center(c, r, &cx, &cz); blob(1, cx, cz, 34, c * 1.7f + r, 0.8f); }
    // trilha: centro de cada sala ate o centro da vizinha conectada (passa pelo vao do muro)
    for (int y = 0; y < MG_CH; y++) for (int x = 0; x < MG_CW; x++) {
        Cell *c = &s_cell[y][x];
        if (!c->used) continue;
        float ax, az; tile_center(x * MG_STEP + 3.5f, y * MG_STEP + 3.5f, &ax, &az);
        for (int d = 1; d <= 2; d++) {
            if (!(c->conn & BIT[d])) continue;
            int nx = x + DX[d], ny = y + DY[d];
            float bx, bz; tile_center(nx * MG_STEP + 3.5f, ny * MG_STEP + 3.5f, &bx, &bz);
            float mx = (ax + bx) * 0.5f, mz = (az + bz) * 0.5f;
            float jit = mg_range(-22.0f, 22.0f);
            if (d == 1) mz += jit; else mx += jit;
            bool side = s_cell[y][x].branch || s_cell[ny][nx].branch;
            trail(ax, az, mx, mz, bx, bz, side ? 34.0f : 44.0f, mg_range(0, 6));
        }
        if (c->role == R_START || c->role == R_EXIT || popcount4(c->conn) >= 3)
            blob(c->role == R_EXIT ? 2 : 0, ax, az, c->role == R_EXIT ? 120.0f : 70.0f, mg_range(0, 6), 0.95f);
    }
    // ordena por camada (grama, terra, trilha, lajes) para desenhar em sequencias
    static const int order[4] = { 3, 1, 0, 2 };
    int n = 0;
    for (int o = 0; o < 4; o++) for (int i = 0; i < s_floor_n; i++) if (s_floor[i].kind == order[o]) tmp[n++] = s_floor[i];
    memcpy(s_floor, tmp, sizeof(FloorQuad) * n);
}

const char *const *mapgen_build(int area, int *rows, int *cols) {
    // 0 cemiterio, 1 cripta (tuneis estreitos), 2 floresta morta, 3 pantano podre,
    // 4 passo congelado, 5 cavernas de gelo (tuneis estreitos)
    bool crypt = area == 1 || area == 5;
    const ChunkSet *set = area == 1 ? &CHK_CRYPT_SET : area == 2 ? &CHK_FOREST_SET : area == 3 ? &CHK_SWAMP_SET :
                          area == 4 ? &CHK_SNOW_SET : area == 5 ? &CHK_ICE_SET : &CHK_CEM_SET;
    s_trail_kind = (area == 2 || area == 3 || area == 4) ? 1 : 0;   // floresta, pantano e neve: trilha de terra
    int sx, sy, ex, ey;
    make_layout(&sx, &sy, &ex, &ey);
    assign_roles(sx, sy, ex, ey);
    for (int r = 0; r < MG_ROWS; r++) { memset(s_buf[r], '#', MG_COLS); s_buf[r][MG_COLS] = 0; s_rows[r] = s_buf[r]; }
    for (int y = 0; y < MG_CH; y++) for (int x = 0; x < MG_CW; x++) if (s_cell[y][x].used) stamp(set, x, y, crypt);
    open_walls(crypt);
    decorate_edges(crypt);
    ensure_chests();
    s_hw = MG_COLS * MG_TILE * 0.5f; s_hh = MG_ROWS * MG_TILE * 0.5f;
    if (!crypt) build_floor(); else s_floor_n = 0;
    *rows = MG_ROWS; *cols = MG_COLS;
    return s_rows;
}

int mapgen_floor(const FloorQuad **out) { *out = s_floor; return s_floor_n; }
