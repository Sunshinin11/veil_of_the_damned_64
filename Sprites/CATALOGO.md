# Catálogo de arte da ROM

Estado em 17/10/2026. As folhas desta pasta são fontes de arte. O conversor em `tools/build_sprite_assets.py` separa as figuras completas e gera os quadros do jogo em `assets/sprites/`.

## Personagens

- **Druida:** duas folhas de caminhada e duas de corrida que cobrem oito direções, além de ataque, magia, esquiva, parado, dano, morte e transformação. `Druid Werewolf.png` é uma folha de referência da transformação.
- **Lobisomem:** parado, caminhada, corrida, ataque, magia, esquiva, dano e morte. São as mesmas categorias de ação do Druida.
- **Lobo espiritual:** parado, corrida, ataque e invocação em `Spirit Wolf Idle.png`, `Spirit Wolf Run.png`, `Spirit Wolf Attack.png` e `Spirit Wolf Summon.png`. O exportador aplica 64% de opacidade real em RGBA32. `Spirit Wolf.png` é uma tentativa anterior e não é usado.

## Inimigos

- **Skeleton Guardian, Demon Brute e Blood Cultist:** folha inicial de poses, caminhada, ataque, dano e morte.
- **Malakor:** folha inicial de poses, caminhada, ataque, dano, fúria e morte. A exportação usa uma escala comum entre essas folhas.

## Efeitos e itens

- `Spirit Bolt FX.png`, `Thorn Burst FX.png`, `Dark Blood Orb FX.png` e `Item Drops.png`.
- `Chest Open.png`: abertura do baú, com vistas frontal e de três quartos.

## HUD

- `HUD Icons.png`: seis ações da barra (ataque, raio espiritual, espinhos, invocação do lobo, forma de fera, poção de vida).
- `HUD Frames.png`: orbes de vida/espírito cheios, barra de seis espaços, barra do chefe e faixa de avisos.
- `HUD Empty Orbs.png`: versões vazias dos orbes para leitura dinâmica de vida e espírito.
- `HUD States.png`: seleção, indisponibilidade, recarga e selo de nível.

## Cenário (arte fonte)

- `Cenario/Caminho dos Condenados/`: árvore seca, lápide gótica, arco da capela, piso de terra com musgo, muro de cemitério, fogueira e corpo de viajante.
- `Cenario/Cripta Vazia/`: piso de lajes, parede de pedra, vitral gótico, braseiro de ferro e corpo de cultista.
- `Cenario/Altar Negro/`: piso e parede de obsidiana, altar sacrificial, trono de Malakor, pentagrama, monólito e braseiro ritual.

Os 19 recursos de cenário convertidos ficam em `assets/scenery/`. Os três pisos são texturas no chão; as paredes e os elementos adicionais são colocados pelo mapa em `src/world.c` e desenhados com a câmera do jogo em `src/sprites.c`. As fontes pintadas permanecem nesta pasta.

## Direção usada na geração

Arte pintada de fantasia sombria, silhuetas legíveis no tamanho de jogo, folhas organizadas em quatro direções e quatro quadros por direção. Lobo espiritual azul-claro translúcido, mesmo personagem e mesma sequência de movimento vistos de frente, três quartos, perfil e costas. HUD em metal escurecido e bronze envelhecido, com ícones individuais e espaço para indicadores dinâmicos.

Integração: os conversores geraram 743 quadros de personagens/efeitos e 19 recursos de cenário. Conferir em Ares a legibilidade a 320 × 240, animações e escala de cada ação, composição do cenário, velocidade e uso de memória em N64 de 4 MB. A ROM será compilada pelo Antigravity.
