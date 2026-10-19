# Prompt ChatGPT: Caçadora v2 (no estilo do Druida / Diablo II)

**Por que refazer:** o Druida parece um personagem de Diablo II e a Caçadora não. O Druida tem:
- figura compacta e "parruda", com poucas formas grandes;
- poucos detalhes;
- sombreado fosco, como um modelo 3D pré-renderizado da época;
- cores dessaturadas e terrosas;
- contorno limpo;
- tamanho pequeno dentro de cada quadro.

As pranchetas atuais da Caçadora são uma ilustração moderna, detalhada demais (capa de franjas, folhas soltas, muito fio).

**Regra de ouro:** gerar no **MESMO formato das pranchetas do Druida**. Assim o jogo processa as duas com o mesmo programa, na mesma escala e com o mesmo acabamento.

## Como pedir
1. **Anexe ao ChatGPT a imagem `Sprites/Druid Idle.png`** como referência de estilo. Isso é o mais importante.
2. Cole o bloco abaixo e peça **uma prancheta por vez**.
3. Salve em `Sprites/Cacadora_v2/` com o nome exato da tabela.
4. Mande primeiro só `Huntress Idle.png` para eu conferir antes de você gerar o resto.

## BLOCO PARA COLAR

> Anexei uma prancheta de sprites do DRUIDA do meu jogo (Nintendo 64, ARPG gótico estilo **Diablo II**). Preciso de uma nova heroína, a **CAÇADORA**, desenhada **exatamente no mesmo estilo, tamanho, enquadramento e formato** dessa referência. Ela deve parecer da mesma equipe de arte, como se fosse a Amazona do Diablo II ao lado do Druida.
>
> **Estilo (copiar da referência):**
> - Aparência de modelo 3D pré-renderizado dos anos 2000 (Diablo II).
> - Sombreado fosco e suave, luz vinda de cima à esquerda.
> - Cores terrosas e dessaturadas.
> - Contorno escuro limpo.
> - Formas grandes e simples, POUCOS detalhes.
> - Figura compacta e robusta, com cabeça, mãos e pés bem legíveis.
> - Mesma altura de figura dentro de cada célula que o Druida.
>
> **A Caçadora:**
> - Mulher atlética, capuz e capa CURTA verde-musgo, com borda lisa: **sem franjas, sem folhas soltas, sem fiapos**.
> - Rosto claro visível sob o capuz, trança ruiva saindo do capuz.
> - Corselete de couro marrom, cinto, calça escura e botas de couro.
> - Aljava nas costas com penas claras.
> - **Arco longo** de madeira escura na mão esquerda.
> - Silhueta bem diferente do Druida: ele tem cajado e capa longa; ela tem arco e capa curta.
>
> **Formato (igual à referência):**
> - Imagem de **1536x1024**, grade de **4 colunas x 4 linhas**, células de 384x256.
> - **Colunas** = os 4 quadros da animação, da esquerda para a direita.
> - **Linhas** = as 4 direções, **na MESMA ordem e virada para o MESMO lado que o Druida na referência**:
>   - linha 1: de frente;
>   - linha 2: diagonal;
>   - linha 3: de perfil;
>   - linha 4: de costas.
> - Fundo **TRANSPARENTE** (PNG com alfa real). Se não conseguir, use verde chapado igual ao da referência.
> - Figura inteira dentro da célula, pés na mesma linha de chão, mesma escala em todas as células.
> - Sem texto, sem sombra no chão, sem efeitos de brilho em volta.

## Pranchetas (uma por resposta)

| Arquivo | Animação (4 quadros) |
|---|---|
| `Huntress Idle.png` | Parada, respirando, arco abaixado (loop) |
| `Huntress Run.png` | Correndo, ciclo de passos (loop) |
| `Huntress Attack.png` | Pega a flecha, puxa a corda, mira com o arco todo puxado, solta |
| `Huntress Hurt.png` | Recebe um golpe e recua |
| `Huntress Death.png` | Cai de joelhos e tomba |
| `Huntress Escape.png` | Rolamento / esquiva rápida |

São 6 pranchetas. Quando chegarem, o build monta os quadros sozinho, no mesmo processo do Druida, e eu gero os quadros intermediários (12 por animação), igual aos dele.
