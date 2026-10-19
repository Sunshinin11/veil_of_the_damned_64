#ifndef MAPGEN_H
#define MAPGEN_H
#include <stdint.h>
#include <stdbool.h>

// Mapas montados a cada partida (estilo Diablo II): pedacos 6x6 desenhados a mao
// (include/map_chunks.h) encaixados num layout aleatorio, com muros entre as salas.
#define MG_CW 3            // pedacos na horizontal
#define MG_CH 5            // pedacos na vertical
#define MG_STEP 7          // 6 tiles de miolo + 1 de muro
#define MG_COLS (MG_CW * MG_STEP + 1)
#define MG_ROWS (MG_CH * MG_STEP + 1)
#define MG_MAX_FLOOR 640

// Chao: cada entrada = 2 triangulos (6 vertices). kind: 0 trilha, 1 terra revirada, 2 lajes antigas, 3 grama morta.
typedef struct { int16_t x[6], z[6]; uint8_t kind; } FloorQuad;

// 0 Caminho dos Condenados, 1 Cripta, 2 Floresta Morta, 3 Pantano Podre, 4 Passo Congelado, 5 Cavernas de Gelo. Devolve as linhas do mapa (validas ate a proxima chamada).
const char *const *mapgen_build(int area, int *rows, int *cols);
// Chao organico do cemiterio gerado junto com o mapa (trilha ligando as salas, terra, lajes, grama).
int mapgen_floor(const FloorQuad **out);
#endif
