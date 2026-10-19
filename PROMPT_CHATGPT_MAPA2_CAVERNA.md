# Prompt para o ChatGPT — pranchetas do Mapa 2 (A Cripta Oca / caverna)

Cole o bloco abaixo no ChatGPT (modelo com geração de imagem). Peça **uma prancheta por vez** (uma resposta = uma imagem), na ordem da lista. Depois salve cada PNG em `Sprites/mapa2_caverna/` com o nome indicado e me avise que eu corto, reduzo para a resolução do N64 e coloco no jogo.

---

## BLOCO PARA COLAR

Você é artista de pixel art / pintura 2D para um jogo de Nintendo 64 chamado VEIL OF THE DAMNED (ARPG roguelike gótico, estilo Diablo II, câmera isométrica de cima). Preciso de **pranchetas (sprite sheets) de cenário** para o segundo mapa: uma **cripta escavada numa caverna úmida**, com pedra escura, musgo, ossos, sangue seco e cristais azuis fracos. O resultado será reduzido para sprites MUITO pequenos (entre 24 e 128 pixels), então o desenho precisa ser **legível em tamanho minúsculo**.

### Estilo (vale para TODAS as pranchetas)
- Visão **3/4 de cima** (a mesma de Diablo II): vemos o topo e a frente do objeto, ligeiramente de cima, nunca de lado puro e nunca de cima puro.
- Pintura 2D escura e suja, com contorno escuro e grosso (1 a 2 px na escala final), sombreado em 3 a 4 tons por material. Pouquíssimo detalhe fino.
- **Paleta**: pedra cinza-azulada e marrom-acinzentado, musgo verde-escuro apagado, sangue vinho escuro, osso bege sujo, cristais ciano/azul-gelo, fogo laranja. Nada saturado demais.
- **Luz sempre vinda de cima à esquerda** em todos os objetos. Sem sombra projetada no chão (o jogo desenha a sombra). Sem brilho/halo (o jogo adiciona o brilho dos cristais e do fogo).
- **Formas orgânicas e irregulares.** NÃO quero formas geométricas perfeitas: nada de triângulos, cones lisos, pirâmides, losangos ou simetria perfeita. Rochas devem parecer rochas de verdade: lascadas, tortas, com topo quebrado.
- Sem texto, sem letras, sem moldura, sem grade desenhada, sem legendas, sem marca d'água.
- **Fundo**: transparente (PNG com alfa real). Se não conseguir transparência real, use fundo **magenta chapado #FF00FF** sem degradê e sem sombra, que eu removo depois. Nunca desenhe o "quadriculado" de transparência.
- Cada objeto num quadro de **512x512 px**, centrado horizontalmente, com os **pés (base do objeto) encostando na parte de baixo do quadro, a ~6% da borda**, e ~6% de margem nas outras bordas. Objetos não podem encostar nos vizinhos nem passar do quadro.
- A prancheta é uma grade de quadros iguais, na ordem indicada (esquerda→direita, cima→baixo). Mantenha **o mesmo tamanho relativo** entre variações do mesmo objeto.

### Pranchetas, uma por vez

**P1 — `p1_estalagmites.png`** — 3 estalagmites/colunas de caverna ALTAS (variações A, B, C), proporção de cerca de 1:2,5 (largura:altura), base larga irregular com pedrinhas, topo irregular/quebrado, pingos de umidade e um pouco de musgo na base. Cada uma diferente da outra. Grade 3x1.

**P2 — `p2_pedras.png`** — 4 grupos de pedras pequenas no chão (2 a 5 pedras cada, de alturas diferentes, com um pouco de cascalho). Largura maior que a altura (proporção 3:2). Grade 2x2.

**P3 — `p3_cristais.png`** — 3 aglomerados de cristais azul-gelo saindo de uma base de rocha escura (variações A, B, C). Cristais de tamanhos diferentes, inclinados em direções diferentes, faces claras na parte de cima e escuras embaixo, sem brilho externo. Proporção 3:4. Grade 3x1.

**P4 — `p4_cogumelos.png`** — 3 grupos de cogumelos de caverna pálidos (chapéu azul-esverdeado claro, caule fino), alturas pequenas, em base de terra com musgo. Grade 3x1.

**P5 — `p5_sarcofagos.png`** — 2 sarcófagos de pedra deitados (variações: um fechado com a tampa levemente deslocada, um quebrado com uma ponta da tampa caída), em 3/4 de cima, com rachaduras, musgo e um pouco de sangue seco. Proporção 3:2 (comprido). Grade 2x1.

**P6 — `p6_altar_sangue.png`** — 1 altar de sacrifício de pedra negra, laje retangular baixa sobre base grossa, com canais gravados por onde escorre sangue escuro e uma poça no pé. Proporção 3:2. Grade 1x1.

**P7 — `p7_braseiro.png`** — braseiro: bacia de ferro escuro sobre base de pedra, **3 quadros de animação do fogo** (mesma bacia idêntica nos 3 quadros, só a chama muda: pequena, média, alta; chama laranja com núcleo amarelo). Grade 3x1. Proporção 1:1.

**P8 — `p8_estatuas.png`** — 2 estátuas de pedra desgastadas (figura encapuzada de mãos postas; e figura com a cabeça quebrada), sobre pedestal baixo, com musgo. Proporção 5:7. Grade 2x1.

**P9 — `p9_ossos_corpos.png`** — (a) 2 pilhas de ossos e crânios (proporção 10:7), (b) 1 cadáver de cultista caído de capuz roxo escuro (proporção 2:1, deitado), (c) 1 caixa torácica grande no chão. Grade 2x2 (a,a,b,c).

**P10 — `p10_decals.png`** — marcas FLAT no chão, vistas de cima achatadas (proporção 2:1, sem volume): (1) poça de sangue, (2) ossos espalhados, (3) rachaduras no chão, (4) cova aberta/buraco escuro, (5) poça de água escura com reflexo azul, (6) teias de aranha no chão. Grade 3x2. Aqui a base NÃO precisa encostar embaixo: centralize cada decal no quadro.

**P11 — `p11_portal.png`** — portal de saída: arco/anel de pedra antiga encaixado na caverna, com vórtice roxo-ciano girando no meio. **3 quadros de animação** (o arco idêntico, só o vórtice muda de fase). Proporção 1:1. Grade 3x1.

**P12 — `p12_entrada.png`** — entrada de caverna/escadaria de pedra descendo (onde o jogador nasce): boca de rocha irregular com degraus gastos e um pouco de luz fria vindo de dentro. Proporção 8:7. Grade 1x1.

**P13 — `p13_trono_ossos.png`** — trono do guardião final feito de ossos e rocha negra, alto, com espaldar irregular de costelas e crânios, sem simetria perfeita. Proporção 1:1. Grade 1x1.

### Texturas (entregar em separado, SEM transparência)
Devem ser **100% sem emendas (seamless/tileable)**, vistas **retas de cima**, sem objetos grandes e sem elementos que se destaquem (para não ficar repetitivo quando ladrilhado), baixo contraste de luz. Tamanho 1024x1024 cada.

- **T1 `t1_chao_caverna.png`** — chão de caverna: pedra irregular cinza-escura com terra, algumas lascas, poucas pedrinhas e manchas de umidade. Sem rachaduras longas e sem formas que pareçam símbolos.
- **T2 `t2_parede_caverna.png`** — parede de rocha vista de frente: estratos horizontais de rocha escura, grandes lascas, leve musgo embaixo. Proporção 2:1 (2048x1024), sem emenda nas laterais.
- **T3 `t3_topo_parede.png`** — topo da parede vista de cima: rocha clara cinza-acastanhada, irregular, sem emenda. 1024x512.

### Entrega
Para cada prancheta devolva **uma única imagem PNG**, sem texto, com o nome de arquivo indicado. Se algum quadro ficar ruim ou com formas geométricas perfeitas (triângulos/cones), refaça só aquele quadro. Não misture dois estilos na mesma prancheta.

---

## Observações minhas (não colar)
- Do lado do jogo, eu reduzo para: estalagmite ≈40x96, pedras ≈56x36, cristal ≈48x64, sarcófago ≈112x72, altar ≈96x64, braseiro 64x64, estátua ≈80x112, decals 64x32, portal 96x96, entrada ≈128x112, chão 32x32, parede 64x32 (limite da memória de textura do N64).
- Se o ChatGPT insistir em desenhar triângulos/cones nas estalagmites, peça "rocha lascada de verdade, assimétrica, tipo foto de caverna, não um cone".
- Os itens de parede (nicho de caveira, correntes, vitral, grades de cela) foram removidos do mapa porque ficavam boiando no chão; não precisa desenhar.
