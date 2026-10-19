# Build: Druida animado (Rota A - skinning esqueletico Tiny3D)

## O que mudou
- Novo asset: `assets/models/druid.glb` (malha riggada do Mixamo, 2863 triangulos,
  cor por regiao do corpo, 6 animacoes: idle, walk, attack, cast, hurt, death).
- `Makefile`: a regra glb->t3dm agora usa `--base-scale=64 --ignore-materials`.
- `src/druid.c`: removido o modelo estatico do druida; adicionado o sistema
  esqueletico (t3d_model_load / t3d_skeleton_create_buffered / t3d_anim_*),
  mapeamento estado->clipe, e `t3d_model_draw_skinned` em `druid_draw`.
- `include/druid.h`: novo prototipo `druid_model_init()`.
- `src/main.c`: chama `druid_model_init()` uma vez no init.
- O lobisomem (forma bestial) continua usando o modelo estatico por enquanto.

## Como compilar
No WSL, na raiz do projeto:
```
./build.sh
```
(ou `make`). A regra nova vai gerar `filesystem/druid.t3dm` a partir do GLB e
empacotar no DFS automaticamente.

## O que observar na ROM
1. O druida deve aparecer como humanoide animado (cabeca/maos em tom de pele,
   manto verde, pernas marrom, botas escuras), tocando idle parado, walk ao andar,
   e attack/cast ao usar A/B. Ao morrer, toca a animacao de queda.
2. Se o druida ficar MUITO grande ou MUITO pequeno: ajuste apenas
   `#define DRUID_MODEL_SCALE` no topo de `src/druid.c` (valor atual 0.0055).
   Maior = maior na tela.
3. Se ele ficar de costas/lado errado ao andar: o offset de direcao esta em
   `druid_build_matrix()` (`g_druid.yaw - 1.5707963f`). Testar somar/subtrair
   1.5707963 (90 graus) se necessario.
4. Se as cores nao aparecerem (druida preto): me avise - trocamos o combiner.

## Se o build falhar
Copie a saida do erro do compilador/importador aqui.
