# Prompt para o ChatGPT — Mapa 3 (O Altar Negro) + Malakor

Peça **uma prancheta por resposta**, na ordem. Salve tudo em `Sprites/Altar_Negro/` com os nomes indicados e me avise.

---

## BLOCO PARA COLAR

Você é artista de pintura 2D / pixel art para um jogo de Nintendo 64 chamado VEIL OF THE DAMNED (ARPG roguelike gótico, estilo Diablo II, câmera isométrica de cima). Preciso das pranchetas da fase final: **O ALTAR NEGRO**, que é literalmente o **INFERNO**: um abismo de rocha vulcânica e basalto rachado, rios e poças de lava borbulhante, chamas saindo das fendas do chão, fumaça e cinzas, espinhos de rocha, ossos e almas acorrentadas, arquitetura demoníaca de obsidiana com chifres e crânios. Tudo iluminado por baixo pelo brilho alaranjado da lava. É a fase mais perigosa do jogo: tudo deve parecer quente, hostil e ameaçador. O resultado será reduzido para sprites pequenos, então precisa ser legível em tamanho minúsculo.

### Estilo (vale para TODAS as pranchetas)
- Visão 3/4 de cima (Diablo II): vemos topo e frente, levemente de cima.
- Pintura 2D escura, contorno escuro e grosso, sombreado em 3 a 4 tons por material, pouco detalhe fino. MESMO estilo das pranchetas anteriores do jogo (caverna/cripta).
- Paleta do inferno: basalto preto e marrom-avermelhado, obsidiana, lava laranja/amarela com núcleo branco-amarelado, brasas vermelhas, sangue vinho, osso bege queimado, ferro negro enferrujado. Nada de verde, nada de azul.
- Luz principal vinda de BAIXO/de frente, como se a lava iluminasse tudo (bordas inferiores alaranjadas), mais uma luz fraca de cima à esquerda. Sem sombra projetada no chão e sem halo externo (o jogo adiciona brilho).
- Formas orgânicas e irregulares. Nada de triângulos, cones lisos ou simetria perfeita.
- Sem texto, letras, molduras, grades desenhadas ou marca d'água.
- Fundo transparente (PNG com alfa real). Se não conseguir, magenta chapado #FF00FF. Nunca o quadriculado de transparência.
- Cada objeto num quadro de 512x512, centrado, base encostando embaixo (~6% de margem). Objetos não encostam nos vizinhos.
- Grade de quadros iguais, ordem esquerda→direita, cima→baixo.

### Cenário
**A1 `a1_selos.png`** — O SELO DE SANGUE (objetivo da fase): obelisco de obsidiana alto e torto, com runas profanas gravadas. Quadro 1: ATIVO (runas e fissuras vermelhas brilhando, sangue escorrendo). Quadro 2: DESTRUÍDO (rachado ao meio, runas apagadas, pedaços caídos). Proporção 1:2. Grade 2x1.

**A2 `a2_rochas.png`** — 4 rochas vulcânicas / pedaços de basalto com veios de lava brilhando, tamanhos variados. Proporção 3:2. Grade 2x2.

**A2b `a2b_espinhos.png`** — 3 grupos de espinhos/estalagmites de rocha vulcânica pontiagudos e tortos (como chifres saindo do chão), com brasas na base. Proporção 1:2. Grade 3x1.

**A2c `a2c_fogo.png`** — fenda no chão soltando uma coluna de fogo: **3 quadros de animação** (fogo baixo, médio, alto; a fenda é idêntica). Proporção 1:2. Grade 3x1.

**A3 `a3_braseiro.png`** — braseiro infernal: tigela de ferro negro com chifres, sobre pedestal de obsidiana, **3 quadros de animação do fogo vermelho-alaranjado** (só a chama muda). Grade 3x1.

**A4 `a4_decoracao.png`** — (1) poste com corpo acorrentado pendurado, (2) estaca com crânios empalados, (3) jaula de ferro com ossos, (4) estátua demoníaca ajoelhada quebrada. Proporção 1:2. Grade 2x2.

**A5 `a5_trono.png`** — o trono de Malakor: enorme, de obsidiana e ossos, com chifres e correntes, sangue escorrendo. Proporção 1:1. Grade 1x1.

**A6 `a6_decals.png`** — marcas FLAT no chão vistas de cima (proporção 2:1): (1) poça de LAVA borbulhante, (2) fissura de lava no chão, (3) círculo ritual de sangue com runas, (4) ossos queimados espalhados, (5) cinzas e brasas, (6) poça de sangue fervendo. Grade 3x2, cada uma centralizada.

**A7 `a7_portal.png`** — portal de saída: arco de obsidiana com vórtice vermelho-escuro, 3 quadros de animação (só o vórtice gira). Grade 3x1.

### Texturas (separadas, SEM transparência, 100% sem emendas, vistas retas, baixo contraste)
- **T1 `t1_chao_altar.png`** (1024x1024): chão de basalto/rocha vulcânica rachada, com finos veios de lava alaranjada entre as placas, cinzas e manchas de sangue queimado. Sem símbolos grandes.
- **T2 `t2_parede_altar.png`** (2048x1024): paredão de rocha vulcânica do inferno vista de frente, com ossos e crânios incrustados, veios de lava escorrendo e base iluminada de laranja, sem emenda nas laterais.
- **T3 `t3_topo_altar.png`** (1024x512): topo do paredão visto de cima, rocha vulcânica escura irregular com brasas.
- **T4 `t4_lava.png`** (1024x1024): superfície de LAVA vista de cima, sem emenda, com crosta escura rachada boiando sobre lava laranja/amarela.

### Malakor (chefe final, GRANDE)
Malakor é um senhor demônio gigante: chifres enormes curvados, pele de obsidiana rachada com brasas por dentro, armadura de ossos e ferro negro, capa rasgada, olhos vermelhos, uma lâmina/foice enorme. Deve parecer MUITO maior e mais poderoso que qualquer outro inimigo.
- Uma prancheta por AÇÃO, cada uma com **4 linhas (direções) x 4 colunas (quadros)**, células de 512x512 (imagem 2048x2048):
  - Linha 1: de FRENTE (vindo para a câmera)
  - Linha 2: DIAGONAL frente-esquerda
  - Linha 3: de LADO, virado para a ESQUERDA
  - Linha 4: de COSTAS
- Mesma escala em todos os quadros, pés na mesma linha de chão, figura inteira dentro da célula sem encostar nas vizinhas.
- Ações (uma prancheta cada):
  - **M1 `m1_malakor_parado.png`** — respirando, lâmina na mão (loop).
  - **M2 `m2_malakor_andando.png`** — passos pesados (loop).
  - **M3 `m3_malakor_ataque.png`** — preparo, golpe, impacto, recuperação.
  - **M4 `m4_malakor_dano.png`** — recebendo golpe.
  - **M5 `m5_malakor_morte.png`** — cai de joelhos e desmorona em brasas.
  - **M6 `m6_malakor_furia.png`** — fase 2: urra com os braços abertos, brasas e chamas saindo do corpo.

### Entrega
Uma imagem PNG por prancheta, com o nome indicado, sem texto. Se um quadro sair com forma geométrica perfeita ou fora da grade, refaça só ele.
