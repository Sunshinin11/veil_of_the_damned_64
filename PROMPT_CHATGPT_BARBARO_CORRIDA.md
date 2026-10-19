# Prompt ChatGPT: corrida do Bárbaro (refazer)

**Problema:** na `Barbarian Run.png` atual os 4 quadros de cada direção são quase iguais. As pernas não se mexem, então de lado e na diagonal ele "desliza" sem passada.

## Como pedir
1. Anexe `Sprites/Barbaro/Barbarian Idle.png` (o mesmo Bárbaro, mesma escala e mesmas direções) e `Sprites/Cacadora_v2/Huntress Run Alt.png` (exemplo de passada que funciona no jogo).
2. Cole o bloco abaixo.
3. Salve como `Sprites/Barbaro/Barbarian Run.png`, substituindo o atual. Se quiser guardar o antigo, renomeie para `Barbarian Run old.png`.

## BLOCO PARA COLAR

> Anexei duas pranchetas do meu jogo de N64 (estilo Diablo II): o **BÁRBARO parado** e a **corrida da CAÇADORA**. Preciso da prancheta de **CORRIDA do Bárbaro**, com o mesmo personagem, estilo, escala e enquadramento da prancheta do Bárbaro parado.
>
> **O mais importante é a passada.** Os 4 quadros de cada linha precisam ser **4 poses de perna claramente diferentes**, um ciclo completo de corrida pesada:
> 1. **Contato A:** perna esquerda esticada na frente tocando o chão, perna direita atrás.
> 2. **Passagem A:** pernas se cruzando embaixo do corpo, joelho direito subindo, corpo um pouco mais alto.
> 3. **Contato B:** perna direita esticada na frente, perna esquerda atrás (o espelho do quadro 1).
> 4. **Passagem B:** pernas se cruzando, joelho esquerdo subindo.
>
> - Os braços balançam ao contrário das pernas. O machado vai numa mão só, balançando junto.
> - A diferença entre os quadros tem que ser bem visível mesmo reduzida a 64x64.
> - Proibido repetir a mesma pose com só o machado mudando.
>
> **Formato (igual à prancheta do Bárbaro parado):**
> - 1536x1024, grade 4 colunas x 4 linhas, células de 384x256.
> - Colunas = os 4 quadros acima, na ordem.
> - Linhas = as 4 direções, **viradas exatamente para o mesmo lado que na prancheta do Bárbaro parado**:
>   - linha 1: de frente;
>   - linha 2: diagonal;
>   - linha 3: de perfil;
>   - linha 4: de costas.
> - Na **linha 3 (perfil)** a passada é a mais importante: o vão entre as pernas abre e fecha a cada quadro.
> - Mesma altura de figura do Bárbaro parado em todas as células: cabeça no mesmo tamanho, sem zoom.
> - Fundo transparente (PNG com alfa real), figuras separadas sem encostar umas nas outras, pés na mesma linha de chão, sem texto e sem sombra no chão.
