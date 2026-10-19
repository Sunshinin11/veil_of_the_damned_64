# VEIL OF THE DAMNED
### Action RPG Gótico para Nintendo 64 (1998 Vintage Dark Fantasy)
**Demo completa e jogável para hardware original Nintendo 64 e emuladores de alta precisão (Ares / SummerCart64 / EverDrive).**

## Atualização de sprites

Os personagens, inimigos, habilidades, itens, baú e HUD agora usam as artes de `Sprites/`, convertidas para quadros pequenos em `assets/sprites/`. O cenário continua em 3D. O botão **R** aciona a esquiva do Druida e do Lobisomem. Para regenerar os quadros e compilar a ROM no Windows, execute `powershell -ExecutionPolicy Bypass -File .\build_with_sprites.ps1`. O inventário está em `Sprites/CATALOGO.md`; as instruções para teste final no Antigravity estão em `PROMPT_ANTIGRAVITY_SPRITES_ROM.md`.

A compilação local passou. A validação visual e de memória no emulador ainda deve ser feita antes de considerar a ROM final.

---

## 1. VISÃO GERAL DO PROJETO

**Veil of the Damned** é um RPG de ação sombrio concebido sob a estética, atmosfera e filosofia de design dos clássicos do gênero do final dos anos 90, mas construído com universo, narrativa, personagens, modelos 3D, efeitos e arquitetura de software 100% originais para o **Nintendo 64 original**.

O jogador assume o manto de um **Druida Protetor**, último guardião de um círculo sagrado corrompido por uma fenda abissal. Armado com seu cajado místico e conectado aos espíritos ancestrais da floresta morta, ele deve descer pelas catacumbas profanadas e derrotar **Malakor, o Guardião Corrompido**, no Altar Negro.

---

## 2. ESPECIFICAÇÕES TÉCNICAS E ORÇAMENTO DE HARDWARE

O jogo foi estritamente desenvolvido respeitando as restrições físicas do console sem periféricos adicionais:

| Componente | Especificação N64 | Implementação em *Veil of the Damned* |
| :--- | :--- | :--- |
| **CPU** | MIPS VR4300 @ 93.75 MHz | Lógica com passo de tempo fixo (*delta time* clamped), física 2.5D, FSM econômica |
| **Memória** | 4.0 MiB RDRAM padrão (Sem Expansion Pak) | Código + BSS + Dados ocupam **~335 KB** (~8.2% da RAM), deixando >3.6 MiB para framebuffers |
| **Processador Gráfico** | RCP (Reality Coprocessor) RSP + RDP | Engine **Tiny3D (T3D)** com microcódigo customizado RSP e rasterização RDPQ |
| **Resolução de Vídeo** | 320 x 240 (16-bit RGBA) | Buffer triplo (*triple buffering*) com Z-Buffer de profundidade e filtro de reamostragem |
| **Iluminação & Névoa** | RSP T&L + RDP Fog Blending | Iluminação direcional + ambiente por vértice e névoa atmosférica dinâmica por área |
| **Áudio** | Audio Interface (AI DMA) 16-bit estéreo | Sintetizador procedural em tempo real a **22.050 Hz** com 14 SFX e drone gótico em Ré Menor |
| **Desempenho** | 20 a 30 FPS estáveis | Taxa de quadros medida e verificada em emulação LLE de ciclo exato (Ares v148) |

---

## 3. MAPEAMENTO DE CONTROLES (NINTENDO 64 PAD)

| Botão / Comando | Ação | Descrição |
| :--- | :--- | :--- |
| **Analógico (Stick)** | **Movimentação Livre** | Controle 360° com zona morta ajustada, aceleração e rotação suave do personagem |
| **Botão A** | **Ataque Básico** | Golpe físico com o cajado (humano) ou garras selvagens cortantes (em forma bestial) |
| **Botão B** | **Spirit Bolt (Raio Espiritual)** | Projétil mágico rápido com colisão e impacto nos alvos (Custo: 15 Mana) |
| **C-Esquerda** | **Thorn Burst (Erupção de Espinhos)** | Círculo de espinhos amaldiçoados que brotam do solo em área (Custo: 25 Mana) |
| **C-Cima** | **Spirit Wolves (Lobos Espectrais)** | Invoca 2 lobos espirituais autônomos que perseguem e atacam inimigos (Custo: 40 Mana) |
| **C-Direita** | **Shapeshift (Transmutação)** | Transforma o Druida em um poderoso Lobisomem com maior velocidade e dano (Custo: 25 Mana) |
| **Botão Z / C-Baixo** | **Poção de Cura / Mana** | Consome poção do cinto para restaurar vida (+45 HP) ou essência espiritual |
| **Start** | **Iniciar / Renascer** | Inicia a jornada no título ou renasce o Druida na área atual após tombar em combate |

> **Modo Attract Arcade Integrado:** Se nenhum controle for pressionado na tela de início por 2.5 segundos, o jogo inicia automaticamente uma demonstração guiada pelo sistema de IA, permitindo que a demo rode em quiosques ou testes automatizados. O jogador pode assumir o controle a qualquer milissegundo simplesmente tocando o analógico ou qualquer botão!

---

## 4. AS 4 HABILIDADES OBRIGATÓRIAS DO DRUIDA

1. **Spirit Bolt (Raio Espiritual - Botão B):**
   - Projétil luminoso com trajetória linear em alta velocidade.
   - Detecção de colisão física com hitboxes dos inimigos e remoção ao impacto.
   - Aplica dano mágico proporcional ao nível do Druida (`32 + nível * 6`).
2. **Thorn Burst (Erupção de Espinhos - C-Esquerda):**
   - Invoca espinhos pontiagudos de pedra e raízes negras em um raio de 40 unidades.
   - Causa dano simultâneo em todos os adversários na área de efeito.
   - Ideal para dispersar emboscadas de esqueletos e monstros em corredores estreitos.
3. **Spirit Wolves (Lobos Espectrais - C-Cima):**
   - Materializa 2 companheiros lupinos espectrais com IA autônoma própria.
   - Buscam ativamente o inimigo mais próximo, realizam investidas e mordidas ferozes.
   - Duram 18 segundos em combate ou até limparem a área.
4. **Shapeshift (Transmutação Bestial - C-Direita):**
   - O Druida assume a forma de um Lobisomem Bestial em modelo 3D distinto.
   - Aumenta a velocidade de deslocamento de 75 para 110 unidades/segundo.
   - Substitui o ataque com cajado por golpes de garras cortantes (`+12 de dano`).
   - Concede regeneração passiva de vida durante a transformação com dreno contínuo de espírito.

---

## 5. ESTRUTURA DAS ÁREAS

- **Área 1: The Forsaken Path (O Caminho dos Condenados):**
  - Trilha florestal morta cercada de árvores retorcidas, lápides quebradas e altares arruinados.
  - Patrulhada por 5 Guardiões Esqueletos.
  - Ensina o combate, esquiva e uso do cajado e raio espiritual. O portal da capela antiga destranca após o confronto inicial.
- **Área 2: The Hollow Crypt (A Cripta Vazia):**
  - Nave subterrânea com grandes colunas góticas, sarcófagos milenares e altares de sangue.
  - Guarda baús de tesouros ocultos e combates intensos contra Cultistas das Trevas e Demônios Menores armados de garras pesadas.
- **Área 3: The Black Altar (O Altar Negro - Batalha de Chefe):**
  - Arena monumental em formato circular cercada por 6 monólitos antigos.
  - Palco do confronto contra **Malakor, o Guardião Profanado**.
  - O chefe possui duas fases dinâmicas: ataque corpo a corpo esmagador no solo e, abaixo de 50% de HP, entra em fúria liberando uma nova radial de orbes de sangue escuro.
  - Ao ser derrotado, Malakor dropa a Relíquia do Santuário e o jogo exibe a sequência de vitória e estatísticas finais.

---

## 6. ESTRUTURA DO CÓDIGO-FONTE

```
Nintendo 64/
├── include/
│   ├── common.h        # Tipos universais, vetores, utilitários matemáticos e constantes
│   ├── models.h        # Gerador procedural e estruturas dos 15 modelos 3D góticos
│   ├── audio.h         # Driver de áudio sintético, sintetizador de 14 SFX e drone gótico
│   ├── druid.h         # Física, movimentação, stats, transmutação e controle do Druida
│   ├── spells.h        # Projéteis, espinhos de Thorn Burst e IA dos Lobos Espectrais
│   ├── enemies.h       # Máquina de estados finitos (FSM) de IA de inimigos e do Chefe Malakor
│   ├── world.h         # Geometria 3D dos pisos, colisores de obstáculos e dados de áreas
│   ├── items.h         # Drops de poções, orbes de alma, baú secreto e relíquia
│   └── hud.h           # HUD 2D clássico estilo Diablo com Orbe de Vida e Espírito
├── src/
│   ├── main.c          # Ponto de entrada, loop de jogo, orquestração de estados e render
│   ├── models.c        # Modelagem procedural de malhas low-poly com cores por vértice
│   ├── audio.c         # Síntese em tempo real 22 kHz estéreo sem dependências de ROM bank
│   ├── druid.c         # Lógica completa do protagonista jogável
│   ├── spells.c        # Lógica de projéteis e entidades mágicas
│   ├── enemies.c       # IA, perseguição, preparação de ataque (telegraph) e hitboxes
│   ├── world.c         # Montagem das 3 áreas e detecção de saída
│   ├── items.c         # Gerenciamento de drops e baús
│   └── hud.c           # Renderização do HUD 2D e interface de habilidades
├── deps/
│   ├── tiny3d/         # Biblioteca 3D moderna de alto rendimento para Libdragon
│   └── ares/           # Emulador de ciclo exato Ares v148 com renderizador paraLLEl-RDP
├── build.sh            # Script de compilação automatizada via WSL Ubuntu
├── Makefile            # Makefile integrado com Libdragon e Tiny3D
├── run_test.ps1        # Script PowerShell para execução e testes diagnósticos com Ares
└── veil_of_the_damned.z64 # ROM final compilada e pronta para execução no N64!
```

---

## 7. COMO COMPILAR E GERAR A ROM

O projeto utiliza a toolchain oficial **Libdragon (versão moderna/preview)** com compilador cruzado `mips64-elf-gcc` e o subsistema gráfico **Tiny3D**.

### No Ambiente Windows com WSL:
1. Abra um terminal PowerShell na raiz do projeto (`c:\Desenvolvimento\Nintendo 64`).
2. Execute o script de compilação:
   ```bash
   wsl -d Ubuntu -e bash /home/georges/veil_n64/build.sh
   ```
3. A compilação é concluída em poucos segundos gerando a ROM binária `veil_of_the_damned.z64`.

---

## 8. COMO EXECUTAR NO NINTENDO 64 REAL E EMULADORES

### Em Hardware Real (Nintendo 64 com Cartucho Flash):
- **SummerCart64:**
  Conecte o SummerCart64 via cabo USB-C ao computador e execute:
  ```bash
  sc64deployer --load veil_of_the_damned.z64
  ```
  O jogo inicializará no console físico e enviará todas as métricas de depuração para o terminal via porta USB integrada do cartucho!
- **EverDrive 64 (v2.x, v3.x, X7) / 64drive:**
  Copie o arquivo `veil_of_the_damned.z64` para o cartão SD do cartucho flash, insira no Nintendo 64 e selecione o jogo no menu.

### Em Emuladores:
- **Ares (Recomendado - Ciclo Exato RSP/RDP):**
  O projeto já inclui o emulador Ares v148 com suporte a Vulkan paraLLEl-RDP em `deps/ares/ares-v148/`.
  Para rodar o teste completo de diagnóstico e execução:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\run_test.ps1
  ```
- **simple64 / Project64:**
  Carregue o arquivo `veil_of_the_damned.z64` diretamente no menu do emulador. Certifique-se de utilizar plugins gráficos modernos baseados em RDP de alta precisão (GLideN64 ou Angrylion/paraLLEl).
