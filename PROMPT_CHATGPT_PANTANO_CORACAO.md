# Prompt ChatGPT: Floresta Morta, fase 2 (Pântano) e fase 3 (Coração Seco)

Peça **uma prancheta por resposta**.

| Fase | Pranchetas | Salvar em |
|---|---|---|
| Fase 2 | P1–P6 e T1–T3 | `Sprites/Pantano/` |
| Fase 3 | C1–C7 e T4–T7 | `Sprites/Coracao_Seco/` |

Depois é só me avisar, que eu importo. Até a arte chegar, o jogo usa provisórios para não quebrar.

---

## ESTILO (cole no começo de TODA prancheta)

Você é artista 2D de um jogo de Nintendo 64 chamado VEIL OF THE DAMNED (ARPG roguelike gótico estilo **Diablo II**, câmera de cima em 3/4). Preciso de pranchetas de CENÁRIO no mesmo estilo dos sprites do jogo: aparência de Diablo II, pintura escura, contorno escuro grosso, formas grandes e simples, 3 a 4 tons por material, pouco detalhe fino. Tudo será reduzido para sprites pequenos, então precisa ser **legível em tamanho minúsculo**.

- Visão 3/4 de cima: vemos o topo e a frente, levemente de cima.
- Luz suave de cima à esquerda. **Sem sombra projetada no chão e sem halo externo** (o jogo adiciona brilho).
- Formas orgânicas e irregulares, nada de simetria perfeita.
- **Sem texto**, letras, molduras, grades desenhadas ou marca d'água.
- **Fundo transparente** (PNG com alfa real). Se não conseguir, use magenta chapado #FF00FF. Nunca o quadriculado de transparência.
- Cada objeto num quadro de **512x512**, centrado, com a base encostando embaixo (cerca de 6% de margem). Objetos não encostam nos vizinhos.
- Grade de quadros iguais, na ordem esquerda→direita, cima→baixo.

---

## FASE 2: O PÂNTANO PODRE (`Sprites/Pantano/`)

**Clima:** pântano morto e venenoso dentro da floresta amaldiçoada. Água parada verde-escura e lamacenta, névoa baixa, árvores de mangue mortas com raízes aéreas, juncos, cogumelos pálidos, barcos afundados e totens de ossos de bruxa. Lanternas de fogo-fátuo azul-esverdeado.

**Paleta:**
- água verde-petróleo escura;
- lama marrom-acinzentada;
- madeira podre cinza-esverdeada;
- musgo oliva;
- osso bege;
- pontos de luz azul-esverdeada fria (fogo-fátuo).

Nada de vermelho forte.

- **P1 `p1_arvores.png`**: 3 árvores mortas de mangue, tortas, com raízes aéreas em arco saindo da água e musgo pendurado. Tamanhos levemente diferentes. Proporção 1:2. Grade 3x1.
- **P2 `p2_plantas.png`**:
  1. touceira de juncos altos;
  2. taboas (juncos com pendão marrom);
  3. grupo de cogumelos pálidos brilhando fraco;
  4. tronco podre caído com musgo.

  Proporção 3:2. Grade 2x2.
- **P3 `p3_decoracao.png`**:
  1. barco de madeira afundado pela metade na lama;
  2. totem de bruxa feito de ossos, galhos e um crânio de cervo;
  3. cabana de palafita pequena e quebrada;
  4. pilha de ossos com musgo.

  Proporção 1:1. Grade 2x2.
- **P4 `p4_lanterna.png`**: poste de madeira torto com lanterna de ferro presa, com uma chama de fogo-fátuo azul-esverdeada dentro. **3 quadros de animação** (só a chama muda). Proporção 1:2. Grade 3x1.
- **P5 `p5_decals.png`**: marcas planas no chão vistas de cima (proporção 2:1):
  1. poça grande de água escura e lamacenta com bolhas;
  2. poça pequena;
  3. mancha de lama;
  4. vitórias-régias / folhas boiando;
  5. limo verde;
  6. ossos meio afundados.

  Grade 3x2, cada uma centralizada.
- **P6 `p6_portal.png`**: portal de saída feito de duas raízes enormes formando um arco, com um vórtice de luz verde-pálida no meio. **3 quadros de animação** (só o vórtice gira). Proporção 1:1. Grade 3x1.

**Texturas** (separadas, SEM transparência, 100% sem emendas, vistas retas, baixo contraste):
- **T1 `t1_chao_pantano.png`** (1024x1024): chão de lama escura com poças rasas, folhas podres e limo.
- **T2 `t2_parede_pantano.png`** (2048x1024): parede de raízes de mangue e troncos emaranhados vista de frente, densa e escura, sem emenda nas laterais.
- **T3 `t3_topo_pantano.png`** (1024x512): o topo dessa parede visto de cima: copa de raízes, musgo e folhas.

---

## FASE 3: O CORAÇÃO SECO (`Sprites/Coracao_Seco/`)

**Clima:** o coração da floresta, onde a Sylvara apodreceu tudo. Uma clareira enorme cercada de raízes gigantes, folhas mortas cor de âmbar cobrindo o chão, troncos secos rachados, espinhos e casulos de raízes com corpos dentro. Fungos grandes soltando esporos amarelo-esverdeados. É outono morto: dourado e marrom, com luz de fim de tarde.

**Paleta:**
- âmbar e ocre (folhas secas);
- marrom de casca;
- osso bege;
- verde-amarelado doentio só nos fungos, esporos e seiva brilhando;
- toques de roxo-escuro nas sombras.

**NADA de vermelho-sangue e NADA de verde forte no cenário.**

- **C1 `c1_totens.png`**: o TOTEM PODRE, objetivo da fase. Tronco grosso esculpido com runas druídicas, com seiva amarelo-esverdeada brilhando nas runas. Quadro 1: ATIVO (runas acesas, seiva escorrendo). Quadro 2: PURIFICADO (rachado ao meio, runas apagadas, casca caindo). Proporção 1:2. Grade 2x1.
- **C2 `c2_raizes.png`**:
  - quadros 1–3: raízes gigantes saindo do chão em arco, de tamanhos diferentes;
  - quadros 4–6: arbustos secos de espinhos, de tamanhos diferentes.

  Proporção 1:1. Grade 3x2.
- **C3 `c3_fungos.png`**: grupo de cogumelos gigantes brilhantes, com chapéus que pulsam com luz amarelo-esverdeada. **3 quadros de animação** (só o brilho muda). É a "tocha" da fase. Proporção 1:1. Grade 3x1.
- **C4 `c4_decoracao.png`**:
  1. casulo de raízes pendurado com um corpo dentro;
  2. pilha de crânios envolta em raízes;
  3. pedra de círculo druídico quebrada, coberta de musgo seco;
  4. carcaça de cervo gigante com raízes atravessando.

  Proporção 1:2. Grade 2x2.
- **C5 `c5_trono.png`**: o trono da Sylvara: um trono enorme formado por raízes entrelaçadas e um tronco oco, com folhas secas e fungos brilhando. Proporção 1:1. Grade 1x1.
- **C6 `c6_esporos.png`**: cogumelo baixo no chão soltando uma nuvem de esporos amarelo-esverdeada. **3 quadros de animação** (nuvem pequena, média, grande; o cogumelo é igual). Proporção 1:2. Grade 3x1.
- **C7 `c7_decals.png`**: marcas planas no chão vistas de cima (proporção 2:1):
  1. poça de seiva venenosa amarelo-esverdeada;
  2. chão rachado e seco;
  3. círculo de raízes no chão com runas (onde fica o totem);
  4. folhas mortas âmbar espalhadas;
  5. ossos espalhados entre folhas;
  6. mancha de seiva escura.

  Grade 3x2.

**Texturas** (separadas, SEM transparência, 100% sem emendas, vistas retas, baixo contraste):
- **T4 `t4_chao_coracao.png`** (1024x1024): terra seca rachada coberta de folhas mortas âmbar e pequenas raízes.
- **T5 `t5_parede_coracao.png`** (2048x1024): muralha de raízes gigantes e casca de árvore retorcida vista de frente, sem emenda nas laterais.
- **T6 `t6_topo_coracao.png`** (1024x512): o topo dessa muralha visto de cima: raízes, casca e folhas secas.
- **T7 `t7_seiva.png`** (1024x1024): superfície de seiva venenosa vista de cima, sem emenda, amarelo-esverdeada e viscosa, com bolhas e crosta escura. É usada nas poças de veneno.

---

### Entrega
Uma imagem PNG por prancheta, com o nome indicado e sem texto. Se um quadro sair fora da grade ou com detalhes miúdos demais, refaça só aquele.
