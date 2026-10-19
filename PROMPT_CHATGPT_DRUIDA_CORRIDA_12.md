# Prompt para o ChatGPT — corrida do Druida com 12 quadros desenhados

Use **uma prancheta por resposta**, nesta ordem: **1) costas (a mais urgente)**, 2) frente, 3) lado esquerdo.
Anexe junto, como referência de personagem e de estilo, a prancheta atual de corrida do lado (`Druid Run Left 4 Frames.png`) e a de parado (`Druid Idle.png`).
Salve como `Druid Run Back 12 Frames.png`, `Druid Run Front 12 Frames.png` e `Druid Run Left 12 Frames.png` em `Sprites/`. O exportador já lê essas folhas sozinho.

---

## BLOCO PARA COLAR

Você é artista de pixel art / pintura 2D de personagem para o jogo de Nintendo 64 VEIL OF THE DAMNED (ARPG gótico, estilo Diablo II, câmera isométrica de cima). Preciso de uma **prancheta de ciclo de CORRIDA com 12 quadros desenhados** do Druida, para a vista pedida abaixo. Use os anexos como referência exata do personagem e do estilo de pintura (contorno escuro, sombreado em poucos tons, paleta escura).

### Personagem (não mudar)
Druida encapuzado, capa verde-oliva escura e suja, cinto e tira de couro marrom cruzando o peito, ombreira de cobre em UM ombro, cajado de madeira com cristal verde brilhante na ponta (o cajado fica sempre na mão direita do personagem, apoiado de forma estável), botas de couro marrom. Mesma proporção, cores e nível de detalhe dos anexos.

### O ponto mais importante: AS PERNAS PRECISAM APARECER
No problema atual a capa cobre as pernas e o corpo "pula" sem passada. Neste ciclo:
- A barra da capa deve ser **mais curta ou aberta/ondulando para trás**, deixando **as duas pernas e as botas visíveis** abaixo dela em todos os quadros.
- As pernas **alternam de verdade**: esquerda à frente, depois direita à frente, com o pé de trás levantado mostrando a sola. Nada de pernas juntas e paradas.
- O corpo (cabeça e tronco) **quase não sobe e desce**: no máximo 2 a 3 px de variação na escala final (cerca de 5% da altura). A sensação de corrida vem das pernas e dos braços, não de pulos.
- A capa e o cajado balançam de leve, no ritmo da passada.

### Ciclo de 12 quadros (uma volta completa = dois passos, o quadro 12 emenda no quadro 1)
1 contato (perna esquerda à frente, pé tocando o chão) · 2 descida · 3 apoio · 4 passagem (perna de trás avançando, no ar) · 5 impulso (perna direita se estendendo) · 6 pré-contato direito · 7 contato (perna direita à frente) · 8 descida · 9 apoio · 10 passagem (perna esquerda avançando) · 11 impulso · 12 pré-contato esquerdo. Poses vizinhas devem diferir pouco, para a animação ficar suave ao rodar em sequência.

### Vistas
- **COSTAS** (`Druid Run Back 12 Frames`): visto de trás, ligeiramente de cima (3/4 de cima, como Diablo II), correndo para longe da câmera. Mostrar capuz, costas da capa, cajado na mão direita (lado direito do personagem), e as pernas alternando por baixo da barra da capa.
- **FRENTE** (`Druid Run Front 12 Frames`): visto de frente, ligeiramente de cima, correndo na direção da câmera.
- **LADO ESQUERDO** (`Druid Run Left 12 Frames`): de perfil, correndo para a ESQUERDA da tela, corpo levemente inclinado para a frente.

### Formato da prancheta (muito importante, o programa lê automaticamente)
- **Grade de 6 colunas x 2 linhas = 12 figuras**, em ordem de leitura: linha de cima do quadro 1 ao 6, linha de baixo do 7 ao 12.
- Imagem em torno de **2400 x 1000 px** (ou 1536 x 1024). Cada figura num espaço de células iguais.
- **As figuras NÃO podem encostar nem se sobrepor** (nem capa, nem cajado de uma figura tocando a vizinha). Deixe margem clara entre elas.
- **Mesma escala em todos os quadros**: a mesma altura de cabeça em todos (variando só os 2–3 px da passada), **pés encostando na mesma linha de chão em todos os quadros da mesma linha**. Não ajuste o tamanho quadro a quadro para "caber".
- Corpo centralizado na célula, com o centro do tronco mais ou menos no mesmo x em todos os quadros (sem o personagem andar para o lado dentro da prancheta).
- **Fundo transparente de verdade (PNG com alfa).** Se não conseguir, use **magenta chapado #FF00FF**, sem degradê, sem sombra no chão, sem moldura. Nunca desenhe o quadriculado de transparência.
- Sem texto, sem números, sem legenda, sem grade desenhada, sem sombra projetada.
- Contorno e estilo idênticos em todos os quadros (não misture estilos).

### Entrega
Uma única imagem PNG com os 12 quadros. Se algum quadro sair com perna escondida, corpo pulando, escala diferente ou figuras encostando, **refaça a prancheta inteira** em vez de entregar com defeito. Antes de entregar, confira que o quadro 12 emenda no quadro 1.

---

## Observações minhas (não colar)
- Ao receber, eu só rodo `python tools/export_druid_run.py`: ele corta, alinha os pés, calibra o tamanho e coloca no jogo, e o `anim_frames.h` já aceita 12 quadros. Se a folha tiver 12 quadros desenhados, ele não gera quadros intermediários.
- Se vierem 8 quadros (4x2), também funciona: o jogo usa os 8 como vierem.
- Para o lobisomem de lado, a mesma ideia vale depois, se você quiser mais fluidez nele.
