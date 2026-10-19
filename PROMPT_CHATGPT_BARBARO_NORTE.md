# Prompts ChatGPT: o Bárbaro e a campanha "O Norte Esquecido"

Peça **uma prancheta por resposta**, salve com o nome exato na pasta indicada e me avise.
- O jogo já roda com **provisórios**: o Bárbaro usa o Druida pintado de vermelho e o cenário usa a arte antiga pintada de gelo.
- Cada prancheta nova que chegar substitui o provisório dela.

**Ordem sugerida:** Parte 1 (o Bárbaro), depois a Parte 4 (cenário das 3 fases), depois as Partes 2, 3 e 5.

| Parte | O quê | Pasta |
|---|---|---|
| 1 | Bárbaro (9 pranchetas) | `Sprites/Barbaro/` |
| 2 | Efeitos e ícones (3) | `Sprites/Barbaro/` |
| 3 | 24 cartas (3) | `Sprites/Cartas_Barbaro/` |
| 4 | Cenário das 3 fases (30) | `Sprites/Norte_Passo/`, `Sprites/Norte_Caverna/`, `Sprites/Trono_Jarl/` |
| 5 | Chefe (12) | `Sprites/Norte_Boss/` |

---

## PARTE 1: O BÁRBARO (`Sprites/Barbaro/`)

Mesmo processo e **mesma identidade visual do Druida e da Caçadora v2**: os três heróis têm que parecer da mesma equipe de arte, lado a lado.
1. **Anexe `Sprites/Druid Idle.png` e `Sprites/Cacadora_v2/Huntress Idle.png` como referência**, mais os dois recortes do jogo em `Sprites/Barbaro/_ref/` (Druida e Caçadora em 64x64). Eles mostram como os heróis ficam de verdade no N64.
2. Cole o bloco abaixo.
3. Mande primeiro só o `Barbarian Idle.png` para eu conferir.
4. Nas pranchetas seguintes, anexe também o `Barbarian Idle.png` aprovado. Assim o rosto, as cores e o machado não mudam de uma prancheta para outra.

> Anexei as pranchetas de sprites do DRUIDA e da CAÇADORA do meu jogo (Nintendo 64, ARPG gótico estilo **Diablo II**). Preciso de um terceiro herói, o **BÁRBARO**, desenhado **exatamente no mesmo estilo, tamanho, enquadramento e formato** das referências, como se fosse o Bárbaro do Diablo II ao lado do Druida e da Amazona.
>
> **Estilo (copiar da referência):**
> - Aparência de modelo 3D pré-renderizado (Diablo II).
> - Sombreado fosco, luz de cima à esquerda.
> - Cores terrosas e dessaturadas: as mesmas do Druida e da Caçadora.
> - Contorno escuro limpo, formas grandes e simples, POUCOS detalhes.
> - Sem pelos soltos, fiapos, franjas, correntes finas ou enfeites pequenos.
> - Figura compacta e robusta, com cabeça, mãos, pés e machado bem legíveis.
> - Mesma altura de figura por célula que o Druida (ele pode ser um pouco mais largo de ombros).
>
> **Identidade visual dos heróis (anexei também como eles aparecem no jogo, em 64x64):**
> - Paleta escura e suja, quase monocromática: verde-oliva, marrom e cinza encardido. Nada limpo nem saturado.
> - **Uma única cor de destaque por herói**: o Druida tem o cristal verde do cajado, a Caçadora tem o ruivo-alaranjado. A do Bárbaro é o **vermelho-ferrugem** (barba e fúria). Nada mais brilha.
> - Rosto quase todo na sombra, olhos e boca pouco visíveis.
> - Roupa gasta, de couro e pano grosso, com rasgos grandes e simples nas bordas (sem fiapos). A silhueta é que identifica o herói, não os detalhes.
> - Precisa continuar reconhecível reduzido a 64x64: silhueta forte, cabeça, ombros e machado bem destacados do corpo.
>
> **O Bárbaro:**
> - Guerreiro nórdico enorme e musculoso, pele clara queimada de frio.
> - Barba ruiva trançada, cabelo preso, cicatrizes.
> - Ombreira de pele de lobo cinza, cinto largo de couro, calça de couro marrom e botas de pele.
> - Braçadeiras de ferro.
> - Um **machado de guerra grande de duas mãos**, de lâmina de ferro escuro.
> - Nada de capa longa; a silhueta é larga e pesada.
>
> **Formato (igual à referência):**
> - Imagem de **1536x1024**, grade de **4 colunas x 4 linhas**, células de 384x256.
> - **Colunas** = os 4 quadros, da esquerda para a direita.
> - **Linhas** = as 4 direções, **na MESMA ordem e viradas para o MESMO lado que o Druida**: 1 frente, 2 diagonal, 3 perfil, 4 costas.
> - Fundo **TRANSPARENTE** (PNG com alfa real).
> - Figura inteira na célula, pés na mesma linha de chão, mesma escala em todas.
> - Sem texto, sem sombra no chão, sem brilho em volta.

| Arquivo | Animação (4 quadros) |
|---|---|
| `Barbarian Idle.png` | Parado, respirando, machado apoiado no ombro (loop) |
| `Barbarian Run.png` | Correndo pesado, machado na mão (loop) |
| `Barbarian Attack.png` | Golpe lateral rápido: preparo, corte da direita para a esquerda, acompanhamento, volta |
| `Barbarian Attack2.png` | Golpe pesado de cima para baixo: ergue o machado, desce com força, impacto no chão, recupera |
| `Barbarian Whirlwind.png` | Girando com o machado esticado, 4 posições do giro (Redemoinho) |
| `Barbarian Shout.png` | Grito de guerra: inspira, abre os braços, urra com a boca aberta, relaxa |
| `Barbarian Hurt.png` | Recebe um golpe e recua |
| `Barbarian Death.png` | Cai de joelhos e tomba de lado |
| `Barbarian Escape.png` | Esquiva: ele **NÃO rola**, ele **SALTA** para a frente. Agacha, pula longe com o machado erguido, no ar com as pernas dobradas, aterrissa pesado |

---

## PARTE 2: EFEITOS E ÍCONES DO BÁRBARO (`Sprites/Barbaro/`)

**Mesma identidade dos efeitos e ícones do Druida e da Caçadora.** Anexe `Sprites/Thorn Burst FX.png` (efeito do Druida) como referência de estilo para os efeitos.

### BLOCO PARA COLAR (antes de cada um)

> Jogo de Nintendo 64 VEIL OF THE DAMNED, ARPG gótico estilo Diablo II. Anexei um efeito de magia do Druida do jogo. Faça o efeito/ícone do **BÁRBARO** no **mesmo estilo**:
> - pintura escura de Diablo II, contorno marcado, formas grandes e simples;
> - 3 a 4 tons por cor, sem partículas minúsculas;
> - legível em tamanho minúsculo.
>
> Os poderes do Bárbaro são **físicos**: poeira, pedra, gelo partido, sangue e fúria vermelho-alaranjada. **Nada de magia brilhante, runas ou luz verde** (isso é do Druida e da Caçadora). Sem texto.

- **`fx_grito.png`**: onda de choque do Grito de Guerra vista de cima: um anel de ar e poeira se expandindo.
  - **4 quadros** lado a lado: o anel começa pequeno, cresce e se desfaz.
  - Imagem de 2048x512, fundo **transparente**.
- **`fx_impacto.png`**: impacto do Salto Esmagador no chão: rachaduras, pedras e poeira subindo.
  - **4 quadros** lado a lado (2048x512), fundo **transparente**.
- **`icones_barbaro.png`**: 6 ícones de habilidade quadrados, **grade 3x2**, células de 512x512.
  - Fundo escuro (não transparente), no mesmo estilo dos ícones de habilidade do Druida e da Caçadora no HUD.
  - Um elemento grande e central, moldura nenhuma.
  - Os ícones:
    1. machado cortando;
    2. salto com o machado descendo;
    3. boca gritando com ondas;
    4. machado girando em redemoinho;
    5. olhos vermelhos de fúria (Berserker);
    6. punho com sangue (reserva).

---

## PARTE 3: CARTAS DO BÁRBARO (`Sprites/Cartas_Barbaro/`)

**Mesmo formato e mesma identidade das cartas do Druida e da Caçadora.** As cores de fundo por raridade são as mesmas nos três heróis: lendária = dourado escuro, amaldiçoada = vermelho-sangue. Só as comuns e as raras ganham o tom do Bárbaro.

### BLOCO PARA COLAR (antes de B1, B2 e B3)

Você é ilustrador de um jogo de Nintendo 64 chamado VEIL OF THE DAMNED (ARPG roguelike gótico, estilo Diablo II). O herói é um **BÁRBARO nórdico** enorme:
- barba ruiva trançada, ombreira de pele de lobo e braçadeiras de ferro;
- um machado de guerra de duas mãos;
- os poderes dele são físicos: fúria vermelha, gritos de guerra, saltos que racham o chão e redemoinho de lâminas.

Preciso das ilustrações das cartas mágicas que ele encontra nos baús.

**Estilo (vale para todas):**
- Pintura 2D gótica escura, contorno marcado, poucas cores fortes, como ícones de habilidade de Diablo II e Darkest Dungeon. Mesmo estilo das cartas do Druida e da Caçadora.
- Um elemento principal grande e centralizado, **legível em 64x80 pixels**, com fundo escuro simples (névoa ou brilho).
- **SEM texto, letras, números, molduras ou bordas.**
- Fundo preenchendo toda a célula, sem transparência.
- **Grade de 4 colunas x 2 linhas**, células de **512x640 px** (imagem de 2048x1280).
- Ordem esquerda→direita, cima→baixo. As células não invadem as vizinhas.

### B1 `b1_cartas.png`: comuns (fundo cinza-pedra frio)
1. Fio do Machado: lâmina de machado com brilho no fio.
2. Garganta de Ferro: boca de guerreiro gritando com ondas de som.
3. Pulmões de Urso: peito largo com tatuagem de urso.
4. Sangue Quente: gotas de sangue fumegando no gelo.
5. Botas de Pele: botas de pele correndo na neve.
6. Pele de Lobo: ombreira de pele de lobo.
7. Fôlego do Norte: vapor saindo da boca no frio.
8. Hidromel: chifre de hidromel transbordando.

### B2 `b2_cartas.png`: raras (fundo azul-gelo escuro)
9. Golpe Duplo: dois machados cruzados.
10. Terremoto: punho batendo no chão com rachaduras.
11. Grito Ensurdecedor: crânio rachando com ondas sonoras.
12. Redemoinho Sangrento: tornado vermelho de lâminas.
13. Salto do Gigante: pegada gigante rachando o gelo.
14. Machado Faminto: machado com dentes, pingando sangue.
15. Olho de Corvo: corvo com olho vermelho.
16. Escudo de Gelo: escudo redondo coberto de gelo.

### B3 `b3_cartas.png`
1ª linha: raras e lendárias. 2ª linha: lendária e amaldiçoadas.

17. Pele de Pedra: braço virando pedra (fundo azul-gelo).
18. Grito do Lobo Branco: lobo branco uivando (fundo azul-gelo).
19. Fúria dos Ancestrais: espíritos de guerreiros atrás do herói (LENDÁRIA, fundo dourado escuro).
20. Martelo do Trovão: machado com raios (LENDÁRIA, fundo dourado escuro).
21. Inverno Eterno: coroa de gelo com nevasca (LENDÁRIA, fundo dourado escuro).
22. Fúria Cega: olhos vermelhos sem pupila e correntes quebradas (AMALDIÇOADA, fundo vermelho-sangue).
23. Pacto de Sangue: mão cortada segurando um machado (AMALDIÇOADA, fundo vermelho-sangue).
24. Coração Gelado: coração congelado rachando (AMALDIÇOADA, fundo vermelho-sangue).

---

## PARTE 4: CENÁRIO DO NORTE

### ESTILO (cole antes de TODA prancheta de cenário)

Você é artista 2D de um jogo de Nintendo 64 chamado VEIL OF THE DAMNED (ARPG roguelike gótico estilo **Diablo II**, câmera de cima em 3/4). Preciso de pranchetas de CENÁRIO no estilo dos sprites do jogo: aparência de Diablo II, pintura escura, contorno escuro grosso, formas grandes e simples, 3 a 4 tons por material, pouco detalhe fino, **legível em tamanho minúsculo**.

- Visão 3/4 de cima (topo e frente).
- Luz fria de cima à esquerda. **Sem sombra projetada e sem halo externo.**
- Formas orgânicas, sem simetria perfeita.
- **Sem texto**, molduras ou marca d'água.
- **Fundo transparente** (PNG com alfa real). Se não conseguir, use magenta #FF00FF.
- Cada objeto num quadro de **512x512**, centrado, base encostando embaixo.
- Grade de quadros iguais, na ordem esquerda→direita, cima→baixo.

**Paleta do Norte:**
- branco-azulado da neve;
- cinza-azul das rochas;
- madeira escura;
- pele e couro marrom;
- ferro escuro;
- luz de fogo laranja só nas fogueiras;
- brilho azul-gelo nos cristais e runas.

### FASE 1: O PASSO CONGELADO (`Sprites/Norte_Passo/`)
Desfiladeiro nevado entre montanhas, com pinheiros mortos cobertos de neve, rochas, pedras rúnicas nórdicas, restos de uma caravana atacada e acampamentos de guerra abandonados.

- **N1 `n1_pinheiros.png`**: 3 pinheiros escuros com neve nos galhos, de alturas diferentes. Proporção 1:2. Grade 3x1.
- **N2 `n2_rochas.png`**:
  1. rocha grande com neve em cima;
  2. grupo de pedras menores com neve;
  3. pedra rúnica nórdica em pé com runas entalhadas;
  4. carroça de madeira quebrada e coberta de neve.

  Proporção 1:1. Grade 2x2.
- **N3 `n3_decoracao.png`**:
  1. guerreiro congelado caído na neve;
  2. estandarte de guerra rasgado num mastro;
  3. paliçada de estacas de madeira pontudas;
  4. presas e ossos de mamute meio enterrados.

  Proporção 1:1. Grade 2x2.
- **N4 `n4_fogueira.png`**: fogueira de acampamento sobre a neve, com pedras em volta. **3 quadros de animação** (só o fogo muda). Proporção 1:1. Grade 3x1.
- **N5 `n5_decals.png`**: marcas planas no chão vistas de cima (proporção 2:1):
  1. placa de gelo liso e brilhante (escorregadia);
  2. monte de neve acumulada;
  3. sangue na neve;
  4. pegadas na neve;
  5. poça congelada;
  6. ossos na neve.

  Grade 3x2.
- **N6 `n6_portal.png`**: portal de saída: arco de duas pedras rúnicas com um vórtice de luz azul-gelo no meio. **3 quadros de animação** (só o vórtice gira). Proporção 1:1. Grade 3x1.

**Texturas** (separadas, SEM transparência, sem emendas, vistas retas, baixo contraste):
- **T1 `t1_chao_neve.png`** (1024x1024): neve pisada com pedrinhas e trechos de gelo.
- **T2 `t2_parede_penhasco.png`** (2048x1024): paredão de rocha cinza-azulada com neve nas saliências, visto de frente, sem emenda nas laterais.
- **T3 `t3_topo_penhasco.png`** (1024x512): o topo desse paredão visto de cima, rocha coberta de neve.

### FASE 2: AS CAVERNAS DE GELO (`Sprites/Norte_Caverna/`)
Cavernas dentro da geleira, com paredes de gelo azul translúcido, estalagmites de gelo, cristais brilhando e guerreiros e feras presos no gelo há séculos.

- **G1 `g1_estalagmites.png`**: 3 estalagmites de gelo azul, pontudas, de alturas diferentes. Proporção 1:2. Grade 3x1.
- **G2 `g2_cristais.png`**: 3 grupos de cristais de gelo brilhando em azul-claro. Proporção 1:1. Grade 3x1.
- **G3 `g3_decoracao.png`**:
  1. guerreiro em pé preso dentro de um bloco de gelo;
  2. lobo gigante preso no gelo;
  3. esqueleto de mamute;
  4. ossos e escudos quebrados no chão.

  Proporção 1:1. Grade 2x2.
- **G4 `g4_braseiro.png`**: braseiro de pedra com chama azul fria. **3 quadros de animação** (só a chama muda). Proporção 1:2. Grade 3x1.
- **G5 `g5_decals.png`**: marcas planas vistas de cima (proporção 2:1):
  1. rachadura no gelo;
  2. água congelada com bolhas;
  3. brilho azul no chão;
  4. ossos;
  5. neve acumulada;
  6. sangue congelado.

  Grade 3x2.

**Texturas:**
- **T4 `t4_chao_gelo.png`** (1024x1024): chão de gelo azul fosco com rachaduras e neve nas frestas.
- **T5 `t5_parede_gelo.png`** (2048x1024): parede de gelo azul translúcido e rocha escura, sem emenda nas laterais.
- **T6 `t6_topo_gelo.png`** (1024x512): o topo da parede de gelo visto de cima.

### FASE 3: O TRONO DO JARL (`Sprites/Trono_Jarl/`)
Salão de uma fortaleza nórdica antiga em ruínas, tomada pelo gelo. Pedras rúnicas gigantes, estátuas de reis mortos, cabeças de troféu, gêiseres de gelo saindo do chão e poças de água gelada mortal. Chamas azuis nos braseiros.

- **J1 `j1_pedras_runicas.png`**: a PEDRA RÚNICA DO JURAMENTO, objetivo da fase. Pedra alta com runas acesas em azul-gelo. Quadro 1: ATIVA (runas brilhando, gelo escorrendo). Quadro 2: QUEBRADA (rachada ao meio, runas apagadas). Proporção 1:2. Grade 2x1.
- **J2 `j2_gelo_rochas.png`**:
  - quadros 1–3: grupos de espinhos de gelo pontudos saindo do chão;
  - quadros 4–6: rochas cobertas de gelo.

  Proporção 1:1. Grade 3x2.
- **J3 `j3_geiser.png`**: buraco no chão soltando um jato de gelo e névoa fria. **3 quadros** (jato baixo, médio, alto; o buraco é igual). Proporção 1:2. Grade 3x1.
- **J4 `j4_decoracao.png`**:
  1. guerreiro congelado empalado numa estaca;
  2. totem de crânios com chifres;
  3. estátua de um rei nórdico antigo, quebrada;
  4. jaula de ferro coberta de gelo.

  Proporção 1:2. Grade 2x2.
- **J5 `j5_trono.png`**: o TRONO DO JARL: trono enorme de pedra e gelo, com chifres e escudos pendurados e peles. Proporção 1:1. Grade 1x1.
- **J6 `j6_decals.png`**: marcas planas vistas de cima (proporção 2:1):
  1. poça de água gelada mortal (azul-escuro com gelo em volta);
  2. rachadura no chão;
  3. círculo de runas no chão (onde fica a pedra rúnica);
  4. ossos;
  5. neve acumulada;
  6. sangue congelado.

  Grade 3x2.
- **J7 `j7_braseiro.png`**: braseiro de ferro nórdico com chama azul. **3 quadros de animação**. Proporção 1:2. Grade 3x1.

**Texturas:**
- **T7 `t7_chao_salao.png`** (1024x1024): piso de pedra de salão antigo com gelo e neve nas juntas.
- **T8 `t8_parede_fortaleza.png`** (2048x1024): muralha de pedra nórdica entalhada com gelo, sem emenda nas laterais.
- **T9 `t9_topo_fortaleza.png`** (1024x512): o topo dessa muralha visto de cima.
- **T10 `t10_agua_gelada.png`** (1024x1024): superfície de água gelada escura vista de cima, com placas de gelo boiando (para as poças mortais).

---

## PARTE 5: O CHEFE DO NORTE (`Sprites/Norte_Boss/`)

**Mesmo formato do Malakor e da Sylvara:**
- uma prancheta por AÇÃO, **4 linhas (frente, diagonal frente-esquerda, lado virado para a ESQUERDA, costas) x 4 colunas (quadros)**;
- células de **512x512** (imagem de 2048x2048);
- fundo **transparente**;
- mesma escala em todos os quadros, pés na mesma linha de chão;
- o chefe é **GIGANTE**.

**Fase 1: Jarl Hrimgar, o Rei Morto.** Rei nórdico morto-vivo gigante: esqueleto coberto de gelo, coroa de ferro com chifres, capa de pele de urso branco congelada, armadura de ferro enferrujada com runas azuis, olhos azuis brilhando. Carrega um machado-espada enorme de gelo.
- **K1 `k1_jarl_parado.png`**: respirando névoa fria (loop).
- **K2 `k2_jarl_andando.png`**: passos pesados (loop).
- **K3 `k3_jarl_ataque.png`**: preparo, golpe, impacto com gelo, recuperação.
- **K4 `k4_jarl_dano.png`**: recebendo golpe.
- **K5 `k5_jarl_morte.png`**: cai de joelhos e se despedaça em gelo.
- **K6 `k6_jarl_furia.png`**: transformação. O gelo cresce sobre o corpo e ele urra.

**Fase 2: o Draugr de Gelo.** O Jarl renasce como um colosso de gelo: corpo de gelo azul translúcido com o esqueleto visível dentro, espinhos de gelo nas costas, braços enormes com garras de gelo, névoa saindo do corpo.
- **D1 `d1_draugr_parado.png`**
- **D2 `d2_draugr_andando.png`**
- **D3 `d3_draugr_ataque.png`**: varrida com as garras.
- **D4 `d4_draugr_dano.png`**
- **D5 `d5_draugr_morte.png`**: desmorona em blocos de gelo.
- **D6 `d6_draugr_furia.png`**: urra com explosão de cristais de gelo.
