<p align="center">
  <img src="Sprites/Menu/logo_veil_of_the_damned.png" alt="Veil of the Damned" width="560">
</p>

<h3 align="center">RPG de ação roguelike gótico para Nintendo 64</h3>

<p align="center">
  Feito do zero para o hardware original do N64 · roda no console de verdade e em emuladores (ares, EverDrive, SummerCart64)
</p>

---

## Sobre o jogo

**Veil of the Damned** é um RPG de ação no estilo dos clássicos do fim dos anos 90: câmera isométrica, hordas de monstros, baús, cartas mágicas e chefes enormes. O universo, os personagens e o código são 100% originais. O jogo foi feito para um cartucho de **Nintendo 64**.

É um **roguelike**:
- os mapas são montados de forma procedural a cada partida;
- cada baú oferece uma carta que muda as habilidades do herói;
- morrer custa caro.

## Heróis

| Herói | Estilo | Habilidades |
|---|---|---|
| **Bárbaro** | Tanque de combate corpo a corpo, movido a **Fúria** | Combo de Machado, Salto Esmagador, Grito de Guerra, Redemoinho, Berserker e um salto de esquiva longo |
| **Caçadora** | Ágil, à distância, alto crítico | Tiro de Arco carregado, Rajada, Armadilha de Raízes, Gavião, Foco do Caçador, Camuflagem |
| **Druida** | Conjurador da natureza | Raio Espiritual, Erupção de Espinhos, Lobos Espectrais e Forma de Fera (lobisomem) |

Cada classe tem vida, dano, armadura, crítico e energia próprios, que crescem de forma diferente a cada nível.

O Bárbaro começa liberado. Os outros heróis são **segredos**: descobrir como libertá-los faz parte do jogo.

## Campanhas

Três campanhas, cada uma com três fases e um chefe próprio, e três dificuldades: **Normal**, **Pesadelo** e **Inferno**.

1. **O Norte Esquecido**: Passo Congelado (gelo escorregadio e nevasca), Cavernas de Gelo e o Trono do Jarl.
2. **Floresta Morta**: floresta sombria, pântano venenoso e o Coração Seco.
3. **Caminho dos Condenados**: cemitério em névoa, cripta e o Altar Negro.

Vencer uma campanha abre a próxima, e vencer uma dificuldade abre a seguinte.

## Recursos

- **Mapas procedurais:** blocos montados a cada partida, testados em milhares de sementes.
- **Cartas:** 72 cartas, 24 por herói, entre comuns, raras, lendárias e amaldiçoadas.
- **Baús selados:** cada um guarda um evento (emboscada, guardião ou vigília). Há também baús menores espalhados pelo mapa.
- **Mascate demoníaco:** troca essência demoníaca por poções, cartas e vidas extras.
- **Chefes:** combates difíceis, com fases e ataques que precisam ser lidos.
- **Animação:** sprites pré-renderizados no estilo dos ARPGs clássicos, com 12 quadros por animação nos heróis.
- **Idiomas:** inglês, português (BR) e espanhol.
- **Save:** salva na EEPROM do cartucho.
- **Expansion Pak:** opcional. Quando presente, aumenta o cache de sprites e melhora o desempenho.

## Controles

| Botão | Ação |
|---|---|
| Analógico | Mover |
| A | Ataque básico |
| B | Habilidade principal |
| C-esquerda / C-cima / C-direita | Habilidades |
| R | Esquiva (rolamento ou salto) |
| Z | Poção |
| D-pad esquerda | Minimapa |
| START | Pausa, status, habilidades e cartas |

## Como compilar

Requisitos: Windows com WSL, o toolchain do **libdragon** e o **Tiny3D**. Os scripts `setup_deps.sh` / `setup_deps_unix.sh` baixam as dependências. Para os conversores de sprites é preciso Python 3 com Pillow, NumPy e OpenCV.

```powershell
powershell -ExecutionPolicy Bypass -File .\build_with_sprites.ps1
```

O script converte os sprites (`Sprites/` → `assets/`), prepara o áudio e compila a ROM `veil_of_the_damned.z64`.

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `src/`, `include/` | Código do jogo em C (libdragon + Tiny3D) |
| `tools/` | Conversores de sprites, gerador de mapas, textos e cartas (Python) |
| `Sprites/` | Arte original em alta resolução |
| `assets/` | Quadros já convertidos para o N64 |
| `SFX/`, `Soundtrack/` | Efeitos sonoros e trilha |

## Tecnologia

[libdragon](https://github.com/DragonMinded/libdragon) · [Tiny3D](https://github.com/HailToDodongo/tiny3d) · C · Python

---

<p align="center"><i>Projeto independente de fã do Nintendo 64. Não é afiliado à Nintendo.</i></p>
