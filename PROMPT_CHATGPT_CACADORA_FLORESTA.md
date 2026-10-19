# Prompts ChatGPT: Caçadora e Floresta Morta

O que **já funciona sem arte nova**:
- A Caçadora usa as pranchetas da pasta `Sprites/DLC`.
- A Floresta Morta reaproveita os cenários pintados de musgo.

Os itens abaixo trocam os **provisórios** por arte de verdade. Peça **uma prancheta por resposta**.

| Prancheta | Salvar em | Depois rodar |
|---|---|---|
| H1, H2, H3 (cartas) | `Sprites/Cartas_Cacadora/` | `python tools/import_cards.py` |
| S1–S6 e R1–R6 (chefe) | `Sprites/Floresta_Morta/` | `python tools/import_forest_boss.py` |

---

## 1) Cartas da Caçadora (24 ilustrações), PRIORIDADE ALTA

Hoje as cartas dela mostram recortes dos próprios sprites tingidos.

### BLOCO PARA COLAR (antes de H1, H2 e H3)

Você é ilustrador de um jogo de Nintendo 64 chamado VEIL OF THE DAMNED (ARPG roguelike gótico, estilo Diablo II). A heroína é uma CAÇADORA de capuz verde-musgo, capa de folhas rasgada, tranças ruivas, arco longo de madeira, aljava nas costas e um gavião. Os poderes dela são verdes, feitos de folhas, raízes e runas da floresta. Preciso das ilustrações das cartas mágicas que ela encontra nos baús.

**Estilo (vale para todas):**
- Pintura 2D gótica escura, contorno marcado, poucas cores fortes, como ícones de habilidade de Diablo II e Darkest Dungeon.
- Um elemento principal grande e centralizado, **legível em 64x80 pixels**, com fundo escuro simples (névoa ou brilho).
- **SEM texto, letras, números, molduras ou bordas.**
- Fundo preenchendo toda a célula, sem transparência.
- **Grade de 4 colunas x 2 linhas**, células de **512x640 px** (imagem de 2048x1280).
- Ordem esquerda→direita, cima→baixo. As células não invadem as vizinhas.

### H1 `h1_cartas.png`: comuns (fundo verde-escuro terroso)
1. **Ponta Afiada**: ponta de flecha de ferro brilhando, afiadíssima.
2. **Raízes Espinhosas**: raízes com espinhos saindo do chão em espiral.
3. **Laço Firme**: laço de cipó apertado prendendo um pé de esqueleto.
4. **Pena de Gavião**: uma pena de gavião listrada flutuando com brilho dourado.
5. **Botas de Couro**: botas de caçadora correndo com rastro de folhas.
6. **Couro Curtido**: peitoral de couro com costuras e presas de animal.
7. **Fôlego**: vapor verde saindo de um frasco de chifre.
8. **Bolsa de Ervas**: bolsinha de couro aberta com ervas e um frasco vermelho.

### H2 `h2_cartas.png`: raras (fundo azul-noite)
9. **Rajada Larga**: leque de 5 flechas voando juntas.
10. **Flecha Perfurante**: flecha atravessando três escudos de madeira.
11. **Armadilha Dupla**: dois círculos de runas verdes no chão, lado a lado.
12. **Bando de Gaviões**: dois gaviões mergulhando juntos.
13. **Gavião Veterano**: gavião com cicatrizes e garras douradas pousado num galho.
14. **Ponta Envenenada**: ponta de flecha pingando veneno verde.
15. **Olho de Águia**: olho dourado de águia com retículo de mira.
16. **Couraça de Casca**: braço coberto de casca de árvore como armadura.

### H3 `h3_cartas.png`
1ª linha: raras e lendárias. 2ª linha: lendária e amaldiçoadas.

17. **Seiva**: gota de seiva dourada escorrendo de um tronco (fundo azul-noite).
18. **Manto de Folhas**: capa de folhas girando como redemoinho, figura sumindo (fundo azul-noite).
19. **Caçadora Lendária**: arco dourado ornamentado com runas verdes acesas (LENDÁRIA, fundo dourado escuro).
20. **Senhora dos Gaviões**: três gaviões em volta de um arco erguido (LENDÁRIA, fundo dourado escuro).
21. **Floresta Viva**: árvore com rosto, raízes agarrando o chão como mãos (LENDÁRIA, fundo dourado escuro).
22. **Mãos Trêmulas**: mãos tremendo segurando arco rachado, flechas tortas (AMALDIÇOADA, fundo vermelho-sangue).
23. **Sede de Sangue**: flecha com sangue escorrendo e dentes de lobo (AMALDIÇOADA, fundo vermelho-sangue).
24. **Olho Frio**: olho gelado azul-branco com retículo, rachaduras de gelo (AMALDIÇOADA, fundo vermelho-sangue).

---

## 2) Chefe da Floresta Morta, PRIORIDADE MÉDIA

Hoje o chefe aparece como o Malakor/Varkhul pintado de verde.

### BLOCO PARA COLAR (antes de cada prancheta S e R)

Você é ilustrador de sprites de um jogo de Nintendo 64 (ARPG gótico estilo Diablo II, visão 3/4 de cima).

**Estilo:**
- Pintura 2D escura, contorno grosso, 3 a 4 tons por material.
- Fundo **TRANSPARENTE** (PNG com alfa real). Se não conseguir, use magenta chapado #FF00FF.
- Sem texto e sem sombra no chão.

**Formato:**
- Uma prancheta por AÇÃO, com **4 linhas (direções) x 4 colunas (quadros)**.
- Células de **512x512** (imagem de 2048x2048).
- **Linhas:** 1 = de FRENTE, 2 = DIAGONAL frente-esquerda, 3 = de LADO virado para a ESQUERDA, 4 = de COSTAS.
- Mesma escala em todos os quadros, pés na mesma linha de chão, figura inteira dentro da célula.
- O chefe é **GRANDE**, muito maior que um humano.

### Fase 1: Sylvara, a Druida Caída
Druida gigante corrompida pela podridão. Pele cinza-esverdeada rachada como casca, cabelo de cipós e musgo, coroa de galhos secos com espinhos, olhos amarelos brilhando, manto de folhas mortas. Carrega um cajado-tronco retorcido com um coração verde pulsando na ponta. Fungos e cogumelos crescem nos ombros.

- **S1 `s1_sylvara_parada.png`**: respirando, cajado apoiado (loop).
- **S2 `s2_sylvara_andando.png`**: passos lentos, raízes arrastando (loop).
- **S3 `s3_sylvara_ataque.png`**: ergue o cajado, golpe no chão, impacto com raízes saindo, recuperação.
- **S4 `s4_sylvara_dano.png`**: recebendo golpe, lascas de casca voando.
- **S5 `s5_sylvara_morte.png`**: cai de joelhos e se desfaz em folhas secas.
- **S6 `s6_sylvara_furia.png`**: transformação. O corpo se abre e raízes enormes saem das costas.

### Fase 2: A Mãe-Raiz
Monstro enorme feito de raízes e troncos podres entrelaçados. O torso de Sylvara fica preso no centro, como uma figura de proa. Tem vários braços de raiz terminando em garras de madeira, boca vertical cheia de espinhos no peito, cogumelos brilhando em verde-doentio e uma névoa de esporos.

- **R1 `r1_maeraiz_parada.png`**: raízes se mexendo, esporos caindo (loop).
- **R2 `r2_maeraiz_andando.png`**: se arrastando sobre as raízes (loop).
- **R3 `r3_maeraiz_ataque.png`**: preparo, varredura com os braços de raiz, impacto, recuperação.
- **R4 `r4_maeraiz_dano.png`**: recebendo golpe, seiva verde espirrando.
- **R5 `r5_maeraiz_morte.png`**: apodrece e desmorona num monte de galhos secos.
- **R6 `r6_maeraiz_furia.png`**: urra abrindo todos os braços, explosão de esporos.

---

## 3) Opcional: deixa a floresta mais "própria"

Hoje esses itens são reaproveitados e pintados de musgo. Se quiser, eu escrevo os prompts de:
- textura de chão de folhas mortas;
- paredão de troncos e raízes;
- totem podre (no lugar do obelisco dos selos);
- árvores mortas com raízes expostas;
- cogumelos gigantes.
