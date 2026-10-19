# Caminho dos Condenados — primeiras peças de cenário

Estas imagens são **artes fonte**. As versões reduzidas estão em `assets/scenery/`; a compilação e a inspeção no emulador ainda precisam ser feitas.

| Arquivo | Uso planejado | Fundo |
| --- | --- | --- |
| `Arvore seca.png` | Árvores mortas nas bordas do caminho | Transparente |
| `Lapide gotica.png` | Lápides do cemitério | Transparente |
| `Arco capela.png` | Saída da primeira área | Transparente, inclusive no vão central |
| `Piso terra musgo.png` | Base do chão com terra, pedras e musgo | Opaco |
| `Muro cemiterio.png` | Painéis do muro ao fundo | Opaco |
| `Fogueira.png` | Pontos de luz e orientação junto ao caminho | Transparente |
| `Corpo viajante.png` | Decoração funerária de chão, longe da entrada | Transparente |

**Direção das imagens geradas:** fantasia sombria gótica pintada à mão, câmera elevada de três quartos para os objetos, silhueta legível no tamanho de jogo, pedra cinza fria, musgo oliva e terra marrom escura. Cada objeto aparece inteiro, isolado e com a base visível para posicionamento no mapa. O piso usa vista ortográfica de cima, detalhes uniformes e bordas pensadas para repetição; conferir a emenda visual após a redução para textura N64. O corpo é decoração fixa de chão e não substitui os inimigos animados.

**Montagem atual:** o chão recebe a textura; muro, árvores, lápides, santuário, fogueiras, corpos, grade e portal usam sprites. Conferir em Ares a repetição do chão e a posição dos painéis.

**Animação:** `Fogueira.png`, `Fogueira 1.png`, `Fogueira 2.png` formam um ciclo de três quadros. `Portal 0.png` a `Portal 2.png` formam outro. `Grade cemiterio.png` é um objeto estático. O conversor usa um recorte e uma escala comuns por animação, com base fixa. Conferir movimento e escala em Ares.
