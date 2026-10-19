# VEIL OF THE DAMNED
## Documento Mestre de Direção Criativa, Gameplay e Implementação — Nintendo 64 (v2: ambiente recuperável, Expansion Pak e 60 FPS)

**Papel:** você é o diretor de arte, game designer e programador responsável por transformar o projeto atual em uma demo jogável de um RPG de ação roguelike gótico para Nintendo 64.

**Objetivo:** construir uma experiência original inspirada na atmosfera, no combate, na exploração de masmorras e na progressão de personagens dos ARPGs clássicos, especialmente Diablo II, reinterpretada para o hardware real do Nintendo 64, **rodando a 60 FPS em um N64 com Expansion Pak (8 MB)**.

Este documento define o ambiente de trabalho, a identidade visual, o personagem principal, os inimigos, os poderes, os ambientes, os sistemas de jogo, as metas de desempenho e os critérios de qualidade. Trate essas especificações como a direção oficial do projeto.

---

# 0. ESTADO DO AMBIENTE E RECUPERAÇÃO (FAÇA PRIMEIRO)

## 0.1. O que existe hoje

Pasta do projeto: `C:\Desenvolvimento\Nintendo 64` (o espaço no nome é a razão do symlink em `build.sh`).

Ao ser inspecionada, a pasta continha:

- `build.sh`: roda **dentro do WSL**, define `N64_INST=/opt/libdragon`, cria o symlink `/home/georges/veil_n64` apontando para `/mnt/c/Desenvolvimento/Nintendo 64`, define `T3D_INST` como `deps/tiny3d` e chama `make`.
- `setup_deps.sh` e `setup_deps_unix.sh`: clonam e compilam o Tiny3D em `deps/tiny3d`.
- `deps/tiny3d` com `build/libt3d.a` já compilado, e `deps/ares/ares-v148/` com `ares.exe`, `SDL3.dll`, `librashader.dll`, `Systems/`, `Database/`, `Shaders/` e `settings.bml` (nele consta `ExpansionPak: true`).
- `Makefile` que compila **apenas** `src/main.c`.
- `src/main.c`: um esqueleto de cerca de 700 bytes que inicializa o vídeo, limpa a tela e fica em laço. Ou seja: **hoje o jogo só mostra uma tela vazia.**
- `src/models.c` e `include/models.h` (cerca de 24 KB): constroem os personagens e objetos com **caixas** (`builder_add_box`). Esse arquivo **não entra no Makefile** e **contradiz as regras de arte deste documento** (seção 3.1). Trate-o como protótipo descartável, não como base.
- `veil_of_the_damned.z64` (cerca de 160 KB) e `ares_output.txt`: o esqueleto abriu no Ares com o renderizador paraLLEl-RDP (Vulkan).

O usuário acredita que, depois que ele apagou a pasta de **outro projeto** (Raptor N64), o Antigravity desinstalou ferramentas que tinham sido baixadas antes (por exemplo o Ares). Não presuma nada: verifique.

## 0.2. Verificação e recuperação do ambiente (uma vez, de forma reproduzível)

Verifique, nesta ordem, e registre o resultado de cada item em `AMBIENTE.md`:

1. **WSL:** existe uma distro funcionando? Qual? O usuário `georges` existe? (`wsl -l -v`.)
2. **Toolchain e libdragon:** `/opt/libdragon/bin/mips64-elf-gcc --version` funciona dentro do WSL? Existe `/opt/libdragon/include/n64.mk`? Qual branch/commit do libdragon está instalado? O Tiny3D exige a branch **`preview`** do libdragon; confirme que a instalada é compatível com o Tiny3D de `deps/tiny3d`.
3. **Tiny3D:** `deps/tiny3d/build/libt3d.a` existe e corresponde à toolchain atual? Se a toolchain foi reinstalada, recompile o Tiny3D.
4. **Ferramentas do Tiny3D e do libdragon** necessárias para os assets (conversores de modelo glTF, de textura e de áudio, `mkasset`, `mkdfs`, `audioconv64` etc.): estão instaladas e no PATH da sessão de build?
5. **Ares:** `deps/ares/ares-v148/ares.exe` existe, abre e carrega a ROM? A configuração de **Expansion Pak** está ativa?
6. **Teste de ponta a ponta:** `build.sh` gera `veil_of_the_damned.z64` e o Ares a abre mostrando o log de inicialização.

**O que fazer com o que faltar (autorizado pelo usuário):**

- Pode **baixar e reinstalar o que estiver faltando**, inclusive o **Ares**: baixe o pacote oficial do Windows x64 da página de releases do projeto ares (`github.com/ares-emulator/ares`), versão v148 ou a estável mais recente, extraia em `deps/ares/<versão>/` e use o `ares.exe` de lá. Confira que o arquivo veio do repositório oficial.
- Pode reinstalar toolchain, libdragon (branch `preview`) e Tiny3D. A toolchain e o libdragon ficam **no WSL, em `/opt/libdragon`** (fora de qualquer pasta de projeto, para que apagar uma pasta nunca mais leve as ferramentas junto). O Tiny3D e o Ares ficam em `deps/` do projeto.
- Se precisar de pacotes do WSL (git, make, build-essential etc.), instale-os com o gerenciador da distro.
- **Pare e pergunte** antes de qualquer coisa que exija administrador do Windows, reinício, habilitar recursos do Windows (WSL, virtualização) ou alterar variáveis de ambiente globais. Não altere o PATH global do Windows.
- Não toque em outros projetos ou repositórios (Raptor N64, Kenji-NX, Ryujinx ou qualquer outra pasta de `C:\Desenvolvimento`). Não faça commit nem push para o GitHub.

**Torne a recuperação reproduzível:** deixe um único script idempotente (`tools/setup_env.sh`, chamado por um `setup_env.ps1` se necessário) que verifica cada item acima e instala somente o que estiver faltando, incluindo o download do Ares com versão fixada. Deixe também `run_ares.ps1` (ou `.bat`) que abre a ROM mais recente no Ares com o comando que já funcionou nesta máquina, e `AMBIENTE.md` com as versões e os comandos. Se o ambiente estiver íntegro, o custo desta seção é só a verificação: não reinstale por reflexo.

## 0.3. Prova de vida (antes de qualquer feature)

Compile e abra no Ares um `main.c` que imprima no log: `memoria total=<bytes>, expansion pak=<0|1>` (via `get_memory_size()` e `is_memory_expanded()`). Esperado no Ares com Expansion Pak ativo: `8388608` e `1`. Se aparecer `4194304`, a opção de Expansion Pak do Ares não está ativa: corrija a configuração (anote o valor original) e repita. Só depois comece a Fase 1.

---

# 1. REGRAS DE EXECUÇÃO

1. Continue a partir do projeto da pasta `C:\Desenvolvimento\Nintendo 64`. Preserve a estrutura, a configuração de compilação e as dependências **que estiverem funcionando**. A seção 0 vem antes de tudo e é a única parte em que reinstalar ferramentas é esperado.
2. Depois da seção 0, não perca tempo com relatórios genéricos sobre o que poderia ser feito. Examine apenas o necessário para entender os pontos de integração e implemente.
3. Não reescreva sistemas funcionais sem uma justificativa técnica concreta. (Exceção declarada: `src/models.c`, ver 0.1.)
4. Não substitua mecânicas solicitadas por texto, objetos estáticos ou efeitos puramente decorativos.
5. Não considere a tarefa concluída apenas porque a ROM compila ou porque o personagem aparece na tela.
6. Se um recurso for grande demais para a primeira versão, implemente uma versão funcional reduzida, mantendo a arquitetura preparada para expansão.
7. Não simule funcionalidades inexistentes. Se não houver animações, IA, geração de mapas ou sistema de combate funcional, implemente-os de verdade dentro dos limites do projeto.
8. Não use assets, personagens, nomes ou mapas extraídos de Diablo II ou de qualquer jogo comercial. A inspiração é estética e mecânica; a identidade, os personagens e o universo devem ser originais.
9. Não substitua o visual por formas geométricas genéricas apenas para terminar rapidamente. Use os recursos gráficos mais adequados à direção de arte, respeitando o desempenho do Nintendo 64 (ver seção 3.4 para a origem permitida da arte).
10. Compile, execute os testes disponíveis, corrija os problemas encontrados e entregue uma ROM reproduzível.
11. **Honestidade de medição:** não invente nem estime resultados. Número que você não mediu consta como "não medido". Medidas de emulador são rotuladas "emulador". **Nunca afirme que o hardware real foi testado:** o teste no N64 real (SummerCart64) é feito pelo usuário depois.

**Prioridade máxima:** qualidade visual coerente, movimentação responsiva, combate funcional, inimigos que representem uma ameaça real, **60 FPS estáveis** e uma primeira experiência jogável completa.

# 2. IDENTIDADE DO JOGO

**Título:** VEIL OF THE DAMNED

**Gênero:** RPG de ação, fantasia sombria, roguelike.

**Plataforma-alvo:** Nintendo 64 original **com Expansion Pak (8 MB)**, com testes adicionais em emulador (Ares).

**Perspectiva:** câmera oblíqua, preferencialmente isométrica ou pseudo-isométrica, mostrando o personagem, os inimigos e uma área suficiente para planejar os combates.

**Fantasia central:** o jogador é um sobrevivente de uma ordem ancestral de guardiões naturais que descobre que uma força profana está corrompendo os mortos, as florestas e os antigos santuários.

O jogador desce por regiões amaldiçoadas, combate criaturas, encontra relíquias e desenvolve seu personagem. Cada incursão apresenta uma organização diferente de salas, encontros e recompensas.

A experiência deve transmitir:

- Solidão e perigo em lugares esquecidos.
- Uma natureza antiga que está sendo consumida por uma força sobrenatural.
- Magia poderosa, mas visualmente contida e legível.
- Combates rápidos e deliberados, com posicionamento importante.
- Recompensas que tornam o personagem progressivamente mais forte.
- A sensação de que cada nova incursão pode apresentar um caminho diferente.

Não transforme o jogo em uma aventura colorida, um RPG infantil ou um clone genérico de fantasia medieval.

# 3. DIREÇÃO DE ARTE

## 3.1. Filosofia visual

A referência visual é a fantasia sombria dos RPGs de PC do final dos anos 1990, reinterpretada com a linguagem gráfica do Nintendo 64.

O resultado deve parecer uma produção concebida para a geração do N64, e não um jogo moderno artificialmente reduzido.

Busque uma combinação de:

- Silhuetas fortes e facilmente reconhecíveis.
- Texturas pequenas, com detalhes pintados à mão.
- Ambientes escuros, mas não uniformemente pretos.
- Contraste entre pedra fria, madeira envelhecida, vegetação morta e magia sobrenatural.
- Animações simples, mas expressivas.
- Efeitos mágicos econômicos e visualmente marcantes.
- Uma quantidade limitada de cores por material e por personagem.
- Névoa, sombras e iluminação quando suportadas pela infraestrutura gráfica existente. **Preferência: luz assada nas cores dos vértices do cenário** (como no Doom 64), em vez de luzes calculadas em tempo de execução.

### Importante sobre os personagens

Não resolva o personagem principal e os monstros usando apenas bonecos poligonais genéricos, cápsulas ou blocos com cores diferentes.

Prefira sprites de baixa resolução, sprites com perspectiva adequada ou personagens billboard quando isso se integrar melhor à arquitetura gráfica existente. Se o projeto usar personagens 3D, aplique uma direção de arte deliberada, com modelos estilizados, texturas pintadas e silhuetas bem definidas.

É aceitável usar geometria 3D para pisos, paredes, pilares, portas e outros elementos do cenário. O que não queremos é um mundo visualmente genérico, sem identidade ou acabamento.

Se for necessário criar sprites, use folhas de sprites consistentes, com poses e animações coerentes. Não desenhe personagens como uma coleção de retângulos sem acabamento.

## 3.2. Paleta de cores oficial

Use a seguinte paleta como referência principal. Os valores HEX são referências de direção de arte; adapte-os às texturas, ao formato de cor e às limitações de renderização do N64.

| Uso | Cor | HEX |
|---|---|---|
| Fundo e áreas profundas | Preto abissal | `#101116` |
| Sombras dos ambientes | Azul-carvão | `#202631` |
| Pedra antiga | Cinza ardósia | `#424750` |
| Pedra iluminada | Cinza calcário | `#77766F` |
| Ossos e caveiras | Marfim envelhecido | `#C5B99C` |
| Couro e madeira | Marrom escuro | `#493126` |
| Terra e sangue seco | Ferrugem escura | `#70352F` |
| Tecido principal do druida | Verde-floresta profundo | `#263F34` |
| Vegetação sobrenatural | Verde espectral | `#70A77A` |
| Energia espiritual | Ciano pálido | `#8FD8D1` |
| Magia profana | Magenta-vinho | `#A43F68` |
| Fogo demoníaco | Laranja incandescente | `#D76B36` |
| Ouro envelhecido | Bronze antigo | `#A68A51` |

### Regras de utilização

- Aproximadamente 70% da imagem deve ser composta por tons escuros, neutros e terrosos.
- Verde e ciano devem identificar a magia do druida.
- Magenta-vinho, vermelho escuro e laranja devem identificar corrupção, demônios e ataques perigosos.
- O marfim deve destacar ossos, crânios e pontos de interesse.
- O bronze deve ser usado com moderação em itens especiais, inscrições e relíquias.
- Evite usar todas as cores simultaneamente na mesma cena.
- Efeitos importantes precisam se destacar do cenário, sem cobrir completamente os inimigos.
- Não use brilho excessivo para compensar sprites sem detalhes ou animações inadequadas.

O jogador deve reconhecer um ataque inimigo, uma magia aliada e um item importante mesmo em uma televisão CRT.

## 3.3. Texturas e materiais

**Pedra:** blocos irregulares, rachaduras, marcas de umidade e juntas escuras. Não use paredes lisas de uma única cor.

**Madeira:** vigas antigas, tábuas quebradas, portas reforçadas e estruturas apodrecidas.

**Metal:** ferro escurecido, rebites, ferrugem e bronze envelhecido. Evite superfícies cromadas.

**Tecido:** roupas em camadas, bordas gastas, remendos e diferenças de tonalidade.

**Carne demoníaca:** superfícies escuras, avermelhadas ou arroxeadas, com contrastes pontuais de energia sobrenatural.

**Vegetação:** raízes expostas, galhos secos, musgo e pequenas áreas de vegetação espectral.

As texturas precisam ter detalhes visíveis na resolução final. Não produza mapas enormes ou materiais excessivamente detalhados que serão reduzidos a borrões. Como o combiner textura × cor de vértice nunca passa de 100% da textura, pinte as texturas **claras** e deixe a luz assada escurecê-las.

## 3.4. Origem e pipeline da arte (decisão obrigatória antes da Fase 1)

O projeto não tem artista. A arte dos personagens precisa vir de um pipeline que produza resultado **bom**, e não de formas primitivas montadas no código. Escolha e justifique **uma** das rotas abaixo, nesta ordem de preferência, e registre a escolha em `ARTE.md`:

**Rota A — modelos 3D baixos com animação por esqueleto no Tiny3D (recomendada).** Modelos low-poly de personagens (cerca de 150 a 500 triângulos cada), com textura pintada pequena e animações (idle, andar, atacar, dano, morte), convertidos de glTF para o formato do Tiny3D com as ferramentas do próprio Tiny3D/libdragon. A câmera inclinada dispensa sprites em 8 direções e deixa todas as animações disponíveis em qualquer ângulo.

**Rota B — sprites pré-renderizados.** Renderizar modelos 3D com Blender (módulo `bpy` via pip, sem interface) em 8 direções × poses, reduzir para CI4 (16 cores, índice 0 transparente) com paleta própria, como no Doom 64. Custa muitos quadros por personagem (direções × animações) e cada sprite com transparência gasta preenchimento de tela: use poucos sprites grandes na tela ao mesmo tempo.

**Origem permitida da arte:**

- Modelos, texturas e sons **livres**, apenas sob licença **CC0 ou CC-BY** (pacotes de personagens low-poly animados de autores que publicam em CC0, por exemplo Quaternius e Kenney, **se** tiverem o que precisamos; confira a licença na página de cada pacote, não presuma). Cada item usado entra em `CREDITS.md` com autor, link e licença; CC-BY exige o crédito.
- Arte original produzida por você com ferramentas de script (Blender/`bpy`, Python, código), desde que cumpra a seção 3.1. **Teste a qualidade visual em jogo e em captura antes de produzir o resto.**
- **Proibido:** qualquer asset extraído de Diablo II, Turok, Prodeus ou outro jogo comercial; modelos de licença não verificada; arte "gerada" que seja só caixas, cápsulas ou cones.

**Teste de arte (obrigatório, antes de seguir):** coloque na tela, no jogo, o Druida, um esqueleto e um cão infernal com suas animações de idle e andar, num trecho de cenário com luz assada, e salve capturas em `capturas\`. O usuário julgará o visual. Se o resultado parecer "bonecos de bloco", pare e troque de rota antes de continuar.

# 4. PERSONAGEM PRINCIPAL — O DRUIDA

O Druida será a única classe obrigatória da primeira demo. Ele precisa ser desenvolvido com mais cuidado que os demais personagens, pois servirá como referência de qualidade para todo o projeto.

## 4.1. Conceito e personalidade

O Druida é um guardião de uma antiga tradição que estudava o equilíbrio entre vida, morte e forças naturais.

Ele não deve parecer um mago convencional com chapéu pontudo, nem um guerreiro bárbaro coberto de músculos.

Sua aparência deve transmitir experiência, resistência, mistério e uma conexão antiga com a floresta.

Ele usa magia espiritual e vegetal, invoca criaturas e pode assumir temporariamente uma forma bestial.

## 4.2. Silhueta e proporções

- Corpo adulto, esguio e resistente.
- Postura ligeiramente curvada para a frente, como alguém habituado a caminhar por terrenos hostis.
- Capuz ou gola alta que enquadre a cabeça.
- Manto curto ou camadas de tecido que permitam reconhecer o movimento do corpo.
- Um ombro protegido por couro endurecido ou placas antigas.
- Cinto largo com bolsas, amuletos e pequenos recipientes.
- Bastão de madeira retorcida, com uma raiz ou cristal espiritual na extremidade.
- Botas de couro e proteções simples nos antebraços.
- Cabelos grisalhos, castanhos ou parcialmente escondidos pelo capuz; barba curta ou média, se a resolução permitir.

A cabeça, o bastão e o manto devem ajudar a identificar o personagem de cima, não apenas de frente.

## 4.3. Roupa e materiais

**Camada externa:** manto verde-floresta escuro, com bordas gastas e pequenas marcas de folhas ou raízes.

**Camada interna:** túnica cinza-amarronzada, marcada pelo uso.

**Capuz:** verde quase preto, com interior em marrom profundo.

**Ombro:** couro envelhecido, preso com tiras e fivelas de bronze oxidado.

**Antebraços:** faixas de tecido, pequenas proteções de couro e marcas rituais discretas.

**Cinto:** bolsas, cordões, dentes de animais e um pequeno amuleto esculpido em madeira.

**Botas:** couro escuro, robusto e sem ornamentos exagerados.

**Bastão:** madeira retorcida, com raízes aparentes e um pequeno cristal verde-pálido ou ciano na ponta.

A roupa deve ser reconhecível mesmo quando o personagem ocupa poucos pixels na tela. Use blocos de cor separados e detalhes apenas onde forem legíveis.

Não cubra o Druida inteiro com partículas, folhas ou efeitos luminosos. A silhueta dele precisa permanecer visível durante o combate.

## 4.4. Animações obrigatórias

Implemente, conforme a técnica de renderização escolhida na seção 3.4:

1. Idle: respiração discreta e movimento leve do manto.
2. Caminhada: alternância clara dos passos, movimento do bastão e balanço da roupa.
3. Ataque básico: golpe curto com o bastão ou ataque espiritual de alcance reduzido.
4. Lançamento de magia: preparação, emissão do poder e recuperação.
5. Dano recebido: reação curta, sem travar excessivamente o controle.
6. Morte: queda, dissolução espiritual ou colapso gradual.
7. Invocação: gesto ritual com o bastão.
8. Transformação: transição visual breve para a forma bestial.
9. Retorno à forma normal.
10. Vitória ou descoberta: animação curta, caso exista tempo para implementá-la.

Se a primeira versão usar sprites, crie quadros coerentes, evitando mudanças abruptas de proporção entre poses. Se usar modelos 3D, mantenha as animações simples e econômicas.

Não é necessário produzir dezenas de quadros para cada ação. É necessário que as ações sejam distintas, legíveis e sincronizadas com os efeitos de gameplay.

# 5. PODERES DO DRUIDA

O conjunto inicial deve possuir quatro habilidades diferentes, com funções distintas. Não implemente quatro versões de um mesmo projétil.

Cada habilidade deve ter custo, tempo de recarga ou outra limitação adequada, efeitos visuais, comportamento de gameplay e interação real com os inimigos.

Os valores abaixo são pontos de partida para o balanceamento, não números imutáveis.

## 5.1. Spirit Bolt — Projétil Espiritual

**Função:** ataque principal à distância.

O Druida canaliza energia espiritual na extremidade do bastão e lança um projétil verde-ciano contra um inimigo.

### Visual

- Pequeno núcleo luminoso ciano.
- Halo verde espectral.
- Rastro curto que desaparece rapidamente.
- Animação de lançamento com movimento do braço ou do bastão.
- Impacto composto por um breve clarão e algumas partículas.

Não use uma esfera branca enorme que esconda o personagem ou cubra a tela.

### Comportamento

- Viaja em direção ao alvo escolhido ou à direção do ataque.
- Causa dano ao primeiro inimigo válido atingido, respeitando a geometria e as regras de colisão.
- Desaparece ao atingir um obstáculo ou completar seu alcance.
- Pode aplicar um pequeno efeito de interrupção em inimigos leves.
- Deve permitir ataques sucessivos com resposta rápida.

### Implementação e balanceamento

- Baixo custo de recurso.
- Recarga curta.
- Dano moderado.
- Velocidade suficiente para tornar o disparo responsivo.
- Colisão consistente com o inimigo, sem depender exclusivamente do efeito visual.

A habilidade precisa funcionar tanto contra um inimigo isolado quanto durante a aproximação de um grupo.

## 5.2. Thorn Burst — Explosão de Espinhos

**Função:** controle de multidões e defesa em curta distância.

O Druida faz raízes e espinhos irromperem do solo em uma pequena área ao seu redor ou em uma área indicada pelo jogador.

### Visual

- Pequenas rachaduras ou marcas verdes no chão antes do ataque.
- Raízes marrons emergindo rapidamente.
- Espinhos verdes escuros com pontas mais claras.
- Breve brilho espectral no momento do impacto.
- Desaparecimento rápido, deixando o chão visível.

A aparência deve remeter a uma planta agressiva, antiga e sobrenatural, não a uma explosão genérica.

### Comportamento

- Atinge múltiplos inimigos em uma área limitada.
- Causa dano em área e uma breve interrupção nos inimigos vulneráveis.
- Não atravessa paredes.
- Precisa considerar a posição real dos alvos.
- Deve funcionar mesmo quando vários inimigos se aproximam simultaneamente.

### Balanceamento

- Recarga intermediária.
- Dano individual inferior ao de um acerto concentrado de uma habilidade de alvo único, compensado pela área de efeito.
- Área suficientemente grande para ser útil, mas não para eliminar todos os inimigos sem posicionamento.

Se a engine não suportar mira no chão, use uma área centrada no jogador como primeira implementação.

## 5.3. Spirit Wolves — Lobos Espirituais

**Função:** invocação e combate indireto.

O Druida invoca dois lobos espectrais para atacar os inimigos próximos.

### Aparência dos lobos

- Silhueta claramente canina, com focinho, orelhas, quatro patas e cauda.
- Corpo formado por tons de verde-escuro e verde espectral.
- Olhos ciano discretos.
- Bordas parcialmente translúcidas, caso a renderização suporte isso sem custo excessivo (**no N64 a transparência custa preenchimento de tela; prefira cor opaca com paleta espectral**).
- Movimento de corrida baixo e agressivo.
- Mordida curta, com avanço do corpo e recuo.
- Pequenas partículas durante a invocação e o desaparecimento.

Não represente os lobos como duas bolas luminosas ou criaturas sem anatomia identificável. Mesmo com poucos polígonos ou quadros de sprite, a silhueta precisa ser imediatamente reconhecida.

### Comportamento

- Os lobos surgem perto do Druida.
- Procuram automaticamente um inimigo válido próximo.
- Aproximam-se do alvo, atacam e voltam a persegui-lo quando necessário.
- Não devem ficar permanentemente presos em paredes.
- Devem trocar de alvo quando o atual morre ou se torna inacessível.
- Têm vida própria e podem ser derrotados.
- Desaparecem quando o tempo de invocação termina ou quando são eliminados.
- Não podem atacar indefinidamente sem intervalo.
- Não devem bloquear a movimentação do jogador.

### Balanceamento

- Duração inicial aproximada de 15 a 25 segundos.
- Recarga relativamente longa.
- Dano moderado, distribuído entre os lobos.
- Limite de duas invocações simultâneas na primeira versão.

A IA pode ser simples, desde que seja funcional: seguir o Druida, procurar um alvo, perseguir, atacar, reagir à morte do alvo e retornar à procura.

Se for necessário reduzir o custo técnico, mantenha dois lobos com comportamento consistente em vez de criar uma multidão de invocações com IA defeituosa.

## 5.4. Shapeshift — Forma Bestial

**Função:** transformação temporária para combate próximo e sobrevivência.

O Druida assume uma forma bestial inspirada em um grande lobo ancestral. A transformação deve mudar a maneira de lutar, e não apenas trocar a aparência do personagem.

### Visual da forma bestial

- Corpo mais baixo e robusto.
- Ombros e patas dianteiras fortes.
- Focinho alongado.
- Pelagem em tons de carvão, marrom e verde-escuro.
- Olhos verde-espectrais.
- Pequenas marcas rituais luminosas nas patas ou no peito.
- Movimento agressivo, com peso e impulso.

A forma não deve parecer um lobo comum ampliado sem personalidade. Ela representa uma entidade espiritual ancestral.

### Transformação

1. O Druida interrompe brevemente sua postura.
2. Uma aura verde envolve o corpo.
3. A silhueta se comprime e muda para a forma bestial.
4. O efeito desaparece, revelando o novo personagem.

A animação deve ser curta, sem uma sequência cinematográfica longa.

### Comportamento

Enquanto transformado:

- O ataque básico passa a ser uma mordida ou golpe com as garras.
- O dano corpo a corpo aumenta.
- A movimentação fica um pouco mais rápida.
- O alcance dos ataques diminui.
- O personagem precisa se aproximar dos inimigos.
- As habilidades mágicas podem ficar temporariamente indisponíveis, exceto se houver uma justificativa de design para permitir alguma delas.
- O jogador pode retornar à forma normal ao término da duração.

### Balanceamento

- Duração inicial aproximada de 10 a 15 segundos.
- Recarga longa.
- Ataques corpo a corpo mais fortes, mas com risco de exposição.
- Não conceder invulnerabilidade.
- Não permitir que a transformação elimine a necessidade de utilizar as outras habilidades.

A transformação deve oferecer uma decisão tática: continuar atacando de longe, invocar os lobos ou entrar no combate corpo a corpo.

# 6. CONTROLES E RESPOSTA DO COMBATE

Os controles devem respeitar as possibilidades do controle original do Nintendo 64 e a arquitetura de entrada existente.

Use, como referência, uma configuração com:

- Analógico ou direcional: movimentação.
- Botão principal: ataque básico.
- Botões secundários: habilidades.
- Botão de gatilho ou botão adicional: habilidade alternativa, esquiva ou transformação.
- Start: pausa.

Adapte os botões exatos ao mapeamento e às limitações do projeto. Não invente suporte a periféricos ou sistemas de entrada que não estejam implementados. Como não há mouse, **use seleção automática do alvo mais próximo na direção do personagem** (com indicador visível) para Spirit Bolt, em vez de "clicar" no inimigo.

Requisitos:

- Movimentação responsiva, sem atraso artificial.
- Colisão consistente com paredes, obstáculos e inimigos.
- Ataques sem animações excessivamente longas que retirem o controle do jogador.
- Feedback claro ao acertar um inimigo.
- Tempo de recuperação perceptível, mas curto.
- Regras claras para invulnerabilidade temporária após dano, caso utilizada.
- O personagem não pode atravessar paredes para perseguir inimigos ou executar ataques.
- O combate deve continuar legível quando três ou mais inimigos estiverem na tela.

Se houver suporte a mira direcional, implemente-a. Se não houver, adote um sistema consistente de direção ou seleção de alvo e deixe isso claro na interface.

# 7. FAMÍLIAS DE INIMIGOS

Os inimigos precisam possuir identidades visuais, funções de combate e padrões de comportamento diferentes. Não basta mudar a cor do mesmo modelo e aumentar seu dano.

A primeira demo deve priorizar três famílias de inimigos, com pelo menos uma variação especial e um chefe.

## 7.1. Skeleton — Esqueleto Errante

**Função:** inimigo básico de combate corpo a corpo.

### Design visual

- Ossos em marfim envelhecido, nunca branco puro.
- Cavidades dos olhos escuras, com pequenos pontos de energia profana.
- Costelas, crânio, braços e pernas claramente identificáveis.
- Restos de tecido cinza-amarronzado.
- Cinto de couro apodrecido.
- Uma espada enferrujada, faca ou arma improvisada.
- Postura ligeiramente inclinada, com movimentos irregulares.

O esqueleto deve parecer um cadáver antigo reanimado, e não um guerreiro humano vivo com uma textura branca.

### Animação

- Caminhada lenta e instável.
- Preparação curta antes do golpe.
- Ataque com espada ou arma improvisada.
- Reação ao impacto.
- Desmontagem ou colapso quando morre.

### IA

- Detecta o jogador dentro de uma distância adequada.
- Aproxima-se evitando obstáculos simples.
- Para ao entrar no alcance de ataque.
- Executa o golpe após uma breve preparação.
- Aguarda a recuperação antes de atacar novamente.
- Persegue o jogador quando ele sai do alcance.
- Retorna à área inicial ou fica em alerta quando perde o alvo, se isso for viável.

### Combate

- Vida baixa ou moderada.
- Dano previsível.
- Movimento relativamente lento.
- Fácil de compreender, mas perigoso em grupo.

## 7.2. Hell Hound — Cão Infernal

**Função:** inimigo rápido que pressiona o posicionamento do jogador.

### Design visual

- Corpo quadrúpede e musculoso.
- Pele em carvão e marrom-avermelhado.
- Costelas ou placas ósseas discretamente expostas.
- Boca aberta com dentes irregulares.
- Olhos e fissuras com brilho laranja.
- Silhueta baixa, alongada e agressiva.

Não use chamas gigantes cobrindo o corpo. O brilho deve ser um detalhe visual que identifica a criatura, não seu único elemento de design.

### Animação

- Corrida rápida e baixa.
- Breve preparação antes de avançar.
- Mordida ou investida.
- Recuo após o ataque.
- Morte com queda e desaparecimento gradual do brilho.

### IA

- Detecta o jogador e se aproxima rapidamente.
- Pode executar uma investida em linha relativamente reta.
- A investida precisa ter uma preparação perceptível.
- Se o jogador desviar, o ataque deve poder errar.
- Após a investida, o cão precisa de um breve intervalo antes de atacar novamente.

### Combate

- Vida baixa a moderada.
- Velocidade elevada.
- Dano significativo se o jogador ignorar a preparação do ataque.
- Vulnerável a habilidades de área, como Thorn Burst.

O jogador deve conseguir aprender a combater essa criatura observando seu comportamento.

## 7.3. Profaned Cultist — Cultista Profanado

**Função:** inimigo de suporte e ameaça à distância.

### Design visual

- Manto vinho muito escuro ou marrom quase preto.
- Capuz profundo, ocultando parcialmente o rosto.
- Máscara de madeira ou metal envelhecido.
- Símbolos rituais discretos no tecido.
- Bastão, lâmina cerimonial ou pequeno foco mágico.
- Mãos com marcas de energia magenta.

A silhueta deve ser estreita e vertical, contrastando com os esqueletos e os cães infernais.

### Animação

- Caminhada lenta.
- Preparação ritual antes do disparo.
- Projétil profano com núcleo magenta e bordas vermelho-escuras.
- Reação ao dano.
- Queda ou dissolução do corpo.

### IA

- Tenta manter distância do jogador.
- Procura uma linha de ataque sem atravessar paredes.
- Prepara um projétil antes de disparar.
- O projétil viaja em direção ao jogador.
- Reposiciona-se quando o jogador se aproxima.
- Não dispara continuamente sem intervalo.

### Combate

- Vida baixa.
- Ataque à distância.
- Dano moderado.
- Pouca resistência ao combate corpo a corpo.
- Obriga o jogador a decidir entre eliminar a ameaça rapidamente ou lidar primeiro com os inimigos que bloqueiam seu caminho.

## 7.4. Graveguard — Guardião Sepulcral

**Função:** inimigo especial e mais resistente.

Pode ser uma variante do esqueleto com uma silhueta maior, armadura funerária e arma pesada.

### Design visual

- Ossos mais escuros, envelhecidos e parcialmente cobertos por placas.
- Elmo funerário quebrado.
- Ombreiras assimétricas.
- Arma pesada de ferro oxidado.
- Pequenos sinais de corrupção magenta em juntas ou cavidades.

### Comportamento

- Aproxima-se mais lentamente.
- Possui um golpe pesado com preparação visível.
- Pode empurrar o jogador ou causar dano em pequena área.
- Recupera-se por um intervalo maior após o ataque.
- Possui mais vida e resistência que um esqueleto comum.

Sua função é criar momentos em que o jogador precisa respeitar a animação de ataque, posicionar-se e escolher quando contra-atacar.

## 7.5. Chefe — The Hollow Warden

**Função:** encerrar a primeira incursão e testar todas as habilidades do Druida.

### Conceito

Uma antiga sentinela funerária que tentou proteger um santuário e acabou corrompida pela entidade que deveria combater.

Ela combina armadura funerária, ossos expostos e energia espiritual corrompida.

### Aparência

- Silhueta maior que a dos inimigos comuns.
- Elmo ou máscara funerária sem rosto humano visível.
- Armadura de placas antigas, com grandes áreas danificadas.
- Uma arma pesada, como uma lâmina de execução ou um machado funerário.
- Núcleo magenta-vinho visível no peito ou na cabeça.
- Pequenos fragmentos de pedra e osso ao redor da silhueta, se o orçamento de renderização permitir.

O chefe deve parecer imponente mesmo com poucos polígonos ou quadros de sprite. Não dependa apenas de aumentar a escala do esqueleto básico.

### Ataques

**1. Golpe pesado:** preparação longa, ataque frontal e recuperação evidente.

**2. Impacto no solo:** golpe que produz uma onda de choque ou uma área de perigo curta ao redor do chefe.

**3. Investida profana:** avanço em linha reta, com indicação visual antes do movimento.

**4. Reforços opcionais:** invocação de poucos esqueletos em um momento específico da luta, apenas se isso funcionar sem comprometer o desempenho.

### Fases

- Primeira fase: golpes pesados e ataques previsíveis.
- Segunda fase, quando a vida estiver baixa: aumento moderado da pressão, com ataques mais frequentes ou uma variante do impacto no solo.

Não torne a segunda fase apenas um aumento arbitrário de velocidade. O jogador deve perceber uma mudança de comportamento.

### Recompensa

A derrota precisa produzir um resultado concreto: um item raro, uma relíquia, uma chave para a próxima região ou uma tela de conclusão da incursão.

O chefe precisa ser derrotável por meio das habilidades implementadas, sem depender de uma mecânica não explicada.

# 8. ARQUITETURA DE IA

Implemente as IAs como máquinas de estados simples, adequadas ao hardware.

Estados possíveis:

- Idle.
- Alert.
- Chase.
- AttackWindup.
- Attack.
- Recovery.
- Hurt.
- Dead.

Acrescente estados específicos apenas quando realmente necessários.

Regras obrigatórias:

- Inimigos mortos não continuam atacando.
- Projéteis não perseguem o jogador indefinidamente, a menos que essa seja uma característica deliberada.
- Inimigos não devem ficar presos permanentemente em paredes.
- A distância de ataque deve ser verificada antes da aplicação de dano.
- Ataques com preparação devem respeitar o tempo de preparação.
- A morte de um inimigo deve atualizar os sistemas de combate e recompensa.
- O número de verificações de IA deve ser controlado para evitar desperdício de processamento.

Prefira colisões simples, distâncias calculadas com operações econômicas e atualizações distribuídas quando necessário. **Para 60 FPS:** inimigos fora da sala atual (ou a mais de um raio definido) ficam em Idle sem atualização; a linha de visão e a busca de alvo dos inimigos e dos lobos são distribuídas entre os quadros (por exemplo, cada inimigo refaz sua busca a cada 4 a 8 quadros, em fases diferentes).

# 9. AMBIENTES E DUNGEONS

O mundo deve ser construído com módulos reutilizáveis, não com geometria aleatória criada sem controle.

A geração procedural precisa produzir mapas estruturalmente diferentes, mas sempre conectados e jogáveis.

**Técnica sugerida (comprovada no projeto Raptor N64 do mesmo usuário):** cada módulo de sala é uma grade de células com altura de teto por célula; piso, teto e paredes viram quadrados com **cor por vértice** (luz assada com oclusão por linha de visão na grade, escurecimento de cantos e luzes de tochas), agrupados em malhas pequenas (até 17 quadrados, 70 vértices) e desenhados sem iluminação em tempo de execução. Isso dá o visual do Doom 64 e é barato para a RSP.

## 9.1. Região inicial — Forsaken Path

Uma passagem subterrânea que conecta uma floresta morta a um complexo funerário.

### Paleta

- Verde quase preto.
- Marrom-terra.
- Cinza ardósia.
- Pequenos pontos de verde espectral.

### Elementos

- Raízes invadindo as paredes.
- Árvores mortas nas áreas externas.
- Pedras quebradas.
- Túmulos abandonados.
- Pequenas estruturas de madeira.
- Entrada para as catacumbas.

A região deve estabelecer a identidade natural corrompida do jogo.

## 9.2. Catacumbas — Hollow Crypt

Esta será a região principal da primeira demo.

### Paleta

- Azul-carvão.
- Cinza ardósia.
- Marfim envelhecido.
- Marrom escuro.
- Magenta pontual em áreas corrompidas.

### Módulos de sala

1. Corredor estreito.
2. Câmara funerária.
3. Sala de combate.
4. Sala de tesouro.
5. Capela abandonada.
6. Câmara com armadilhas.
7. Sala de elite.
8. Câmara do chefe.

Cada módulo deve ter dimensões e portas definidas de antemão. As conexões precisam encaixar corretamente.

### Decoração

- Sarcófagos.
- Ossadas.
- Pilares rachados.
- Velas.
- Grades enferrujadas.
- Símbolos rituais.
- Pequenos altares.
- Fragmentos de estátuas.

A decoração não pode bloquear passagens importantes nem confundir o jogador sobre onde é possível caminhar.

## 9.3. Santuário profanado — Black Altar

Região destinada ao confronto final da primeira incursão.

### Paleta

- Preto abissal.
- Cinza calcário.
- Vinho escuro.
- Laranja discreto.
- Bronze envelhecido.

### Elementos

- Altar central.
- Pilares altos.
- Marcas rituais.
- Correntes.
- Ossadas.
- Pequenas fontes de luz profana.

O centro da arena precisa permanecer legível. Não encha o chão de objetos que dificultem enxergar o chefe ou seus ataques.

## 9.4. Geração procedural

Implemente um sistema com:

- Semente aleatória para cada incursão.
- Seleção de salas modulares.
- Conexões por portas ou corredores.
- Caminho principal até a sala do chefe.
- Ramificações opcionais.
- Salas de combate e tesouro.
- Posicionamento controlado de inimigos e recompensas.
- Verificação de conectividade.
- Verificação de que a entrada e a saída podem ser alcançadas.
- Limite de tamanho para cada mapa.

Use uma semente reproduzível durante os testes para que um problema de geração possa ser repetido.

A primeira versão não precisa criar paredes, texturas ou geometria totalmente novas em tempo de execução. O objetivo é combinar módulos existentes de formas diferentes.

**Regra essencial:** uma masmorra procedural só está funcionando se produzir caminhos diferentes sem criar salas inacessíveis, portas desconectadas, corredores bloqueados ou chefes impossíveis de alcançar.

**Teste no PC (obrigatório):** o gerador deve poder rodar fora do N64 (um programa de teste em C compilado no PC, ou o mesmo código com stubs) para validar, com centenas de sementes, conectividade, ausência de salas isoladas, alcance da sala do chefe e limite de tamanho, e imprimir um mapa em ASCII por semente. Registre quantas sementes foram testadas e o resultado.

Se o sistema procedural ainda não estiver pronto, primeiro implemente algumas salas modulares com conexões verificáveis e depois adicione aleatoriedade.

# 10. ITENS, RECOMPENSAS E PROGRESSÃO

Os itens precisam mudar alguma coisa no jogo. Não implemente apenas objetos com nomes diferentes.

Na primeira versão, concentre-se em três categorias.

## 10.1. Equipamentos

- Bastão ancestral: aumenta o dano de Spirit Bolt.
- Amuleto de raízes: reduz a recarga de Thorn Burst.
- Presas espirituais: aumenta o dano dos lobos.
- Pele do guardião: aumenta a duração de Shapeshift.

## 10.2. Consumíveis

- Poção de cura.
- Essência espiritual para recuperar recurso, se o sistema de habilidades usar um recurso consumível.
- Um consumível raro que possa ajudar durante uma luta difícil.

## 10.3. Relíquias

- **Rootheart:** aumenta o alcance de Thorn Burst.
- **Twin Fang:** melhora o dano dos lobos.
- **Piercing Wisp:** permite que Spirit Bolt atravesse um inimigo, se isso puder ser implementado sem criar problemas de colisão.
- **Ancient Hide:** aumenta a duração da transformação, com balanceamento adequado.

Cada item precisa possuir uma descrição curta e um efeito real no gameplay.

Não crie dezenas de itens antes de implementar um sistema de equipamento funcional. Quatro ou cinco itens bem implementados são melhores que cinquenta itens que não fazem nada.

# 11. INTERFACE E FEEDBACK VISUAL

A interface deve parecer parte do mesmo universo visual.

## Estilo

- Fundo escuro e opaco.
- Bordas inspiradas em ferro antigo, madeira escurecida ou pedra.
- Tipografia legível.
- Ícones simples e contrastantes.
- Verde para as habilidades naturais do Druida.
- Vermelho escuro ou magenta para perigo e corrupção.
- Bronze para itens raros.

Evite excesso de ornamentação. A interface deve ser gótica sem sacrificar a leitura.

## Elementos obrigatórios

- Barra de vida.
- Indicador de recurso mágico, caso exista.
- Indicadores das habilidades e seus tempos de recarga.
- Vida dos inimigos quando útil.
- Nome e barra de vida do chefe.
- Feedback de dano recebido.
- Indicação clara de morte e conclusão da incursão.
- Tela de pausa.
- Tela de início ou seleção inicial da classe, conforme o estado do projeto.
- Contador de FPS e de diagnóstico, ligável e desligável por botão (ver seção 13.5).

A interface deve funcionar na resolução final do jogo e continuar legível em uma televisão CRT.

Não use textos minúsculos nem dependa de transparências sutis que desapareçam em hardware real.

# 12. ÁUDIO E ATMOSFERA

Use áudio econômico, mas expressivo.

Prioridades:

- Ambiente subterrâneo com vento distante, gotejamento ou ruídos de estruturas antigas.
- Passos distintos em pedra ou terra, quando viável.
- Golpes de armas com impacto perceptível.
- Som curto e sobrenatural para Spirit Bolt.
- Som vegetal agressivo para Thorn Burst.
- Uivo ou chamado espiritual na invocação dos lobos.
- Som grave e orgânico durante Shapeshift.
- Ruídos ósseos na movimentação e morte dos esqueletos.
- Sons de corrupção para os cultistas.
- Música tensa e discreta nas áreas de exploração.
- Música ou camada sonora mais intensa na luta contra o chefe.

Não reproduza faixas de Diablo II. A trilha deve ter identidade própria. Sons e músicas de terceiros só se forem CC0 ou CC-BY e constarem em `CREDITS.md`.

Se o sistema de áudio atual for limitado, priorize efeitos curtos e reconhecíveis, sem comprometer a estabilidade nem os 60 FPS. O áudio entra depois que a vertical slice estiver estável a 60 FPS (ver fases).

# 13. NINTENDO 64: EXPANSION PAK, 60 FPS E MEMÓRIA

O alvo não é apenas um emulador. A ROM deve ser desenvolvida pensando no Nintendo 64 original **com Expansion Pak**.

## 13.1. Expansion Pak (8 MB)

- O jogo é projetado para **8 MB**. No início, registre em log `memoria total=<bytes>, expansion pak=<0|1>`. Se `is_memory_expanded()` for falso, mostre uma tela clara "REQUER EXPANSION PAK" em vez de falhar de modo imprevisível (a menos que você demonstre, medindo, que roda sem perdas em 4 MB; nesse caso registre).
- **O Expansion Pak adiciona memória, não velocidade.** Use os 8 MB para: manter os módulos de sala e as texturas do nível atual carregados sem streaming, conjuntos de animação completos, áudio em memória e folga para o heap. Não use o Expansion Pak como desculpa para subir a resolução: **a base é 320×240, 16 bits, 3 framebuffers + z-buffer.** Resolução maior (por exemplo 640×480) é proibida, pois derruba o desempenho.
- Reserve orçamentos de memória por categoria (framebuffers e z-buffer, texturas, geometria de nível, modelos e animações, áudio, heap dinâmico) e imprima o uso real em log. Não aloque memória durante o combate: use pools estáticos (inimigos, projéteis, partículas, lobos).
- No Ares, confirme que a opção de **Expansion Pak** está ativa (o `settings.bml` do Ares tem a chave `ExpansionPak`) e que o log mostra 8 MB. Anote o valor original se mudar a configuração.

## 13.2. Meta de 60 FPS

**Meta:** o laço do jogo roda a **60 quadros por segundo estáveis** (NTSC, 16,7 ms por quadro) no N64 real com Expansion Pak, em **todas** as situações normais de jogo: exploração, combate com vários inimigos, lobos e magias na tela, e luta contra o chefe. Nenhum quadro de combate deve ultrapassar 16,7 ms; quadros de 33 ms são falhas e devem ser contados e corrigidos.

Observações honestas que orientam o projeto:

- No projeto Raptor N64 do mesmo usuário, a versão só de geometria rodou a 60 FPS travados no hardware real; a versão com muitos sprites transparentes e cenário feito de sprites caiu para cerca de 30 FPS lá, embora o emulador mostrasse 60. **Conclusão: emulador não prova 60 FPS; preenchimento de tela (transparência, sobreposição) e excesso de triângulos visíveis são os maiores riscos.**
- Com vsync a 60 Hz, o FPS cai em degraus (60 → 30 → 20). Por isso importam o **maior tempo de quadro** e a **contagem de quadros lentos**, não só a média.
- Você não tem acesso ao hardware real. Seu trabalho é **projetar e medir para dar a máxima margem**, e entregar o instrumental para o usuário medir no N64 (13.5).

## 13.3. Orçamento e técnicas obrigatórias

Os números abaixo são **pontos de partida para medir e ajustar**, não garantias. Registre os valores reais medidos.

- **Geometria:** cenário em módulos de sala pequenos; câmera inclinada com campo de visão estreito e **distância de visão curta**; **culling por sala** (desenhe só a sala atual e as vizinhas visíveis pelas portas) além do culling por esfera e cone. Meta inicial: no máximo cerca de 1500 triângulos visíveis por quadro somando cenário e personagens.
- **Personagens:** poucos triângulos (Rota A: cerca de 150 a 500 por personagem), no máximo cerca de 8 a 10 personagens animados ativos ao mesmo tempo; fora disso, não atualize nem desenhe. Esqueleto de poucos ossos; animações curtas; atualize animações de inimigos distantes com menos frequência.
- **Sprites e efeitos:** poucos, pequenos e preferencialmente **opacos ou com recorte por alfa** (sem mistura). Orçamento inicial: no máximo cerca de 32 partículas simultâneas; evite quads grandes translúcidos cobrindo a tela. Contabilize os pixels desenhados por frame.
- **Preenchimento (RDP):** prefira modos de **um ciclo**; evite combinar neblina, múltiplas texturas e mistura no mesmo objeto quando não for necessário; mantenha o z-buffer em 16 bits; ordene o desenho por textura para evitar trocas de estado e uploads de textura; agrupe malhas pequenas em blocos RSPQ pré-gravados (`rspq_block`).
- **CPU:** passo de simulação fixo em 1/60 s; IA com orçamento (seção 8); colisão em grade com células pequenas e consultas O(1); sem `malloc` no laço do jogo; sem `sqrtf` desnecessário (compare distâncias ao quadrado); sem `printf`/`debugf` por quadro (use contadores e imprima a cada 2 s).
- **Câmera e vídeo:** `display_init` em 320×240, 16 bpp, 3 framebuffers; nunca bloqueie esperando o vsync sem necessidade; o tempo de submissão do quadro (CPU) deve ficar bem abaixo de 16 ms para deixar folga à RSP/RDP.
- **Degradação planejada:** implemente limites dinâmicos (máximo de inimigos ativos, de partículas e distância de visão) que você possa baixar para recuperar o 60 FPS **antes** de aceitar 30. Se uma cena específica (por exemplo, o chefe com reforços) não alcançar a meta nas suas medições, corte conteúdo ou efeitos daquela cena e registre o corte.

## 13.4. Como medir (sem inventar números)

- Meça com `get_ticks_us()` as seções do quadro: lógica de jogo e IA, preparação de desenho (CPU) e espera pelo vsync, e conte triângulos, malhas, sprites, partículas e personagens ativos. Se o libdragon instalado oferecer o perfilador do RSPQ (`rspq_profile`), use-o e registre o resultado; se não, diga que não foi usado.
- Crie um **modo de benchmark automático** (`make AUTOPLAY=1`): uma semente fixa, o Druida percorre um trajeto roteirizado por salas com combate real (inimigos, os quatro poderes e o chefe), por um tempo definido, e o jogo imprime estatísticas a cada 2 s e um resumo no fim (FPS médio, maior tempo de quadro, quadros acima de 16,7 ms e de 33 ms, médias e picos de triângulos, personagens, partículas, uso de memória). Esse mesmo modo é o que o usuário rodará no N64 real.
- **Emulador:** o Ares (RSP em LLE, paraLLEl-RDP) serve para verificar funcionamento e para uma estimativa relativa; seus tempos **não** representam o console. Rotule sempre "emulador". Registre versão do Ares, renderizador, resolução interna, CPU e GPU do PC.
- **Hardware real:** deixe pronto, sem executar, um procedimento para o usuário (copiar a ROM para o cartão do SummerCart64, carregar, ler o log pela USB, o que anotar) e uma tabela vazia "N64 real" ao lado da coluna "emulador". Nunca preencha essa coluna.

## 13.5. Instrumentos no jogo

- HUD de diagnóstico ligável por botão: FPS, ms de lógica, ms de submissão, ms de espera, triângulos, personagens ativos, partículas, memória usada.
- Linhas de log periódicas e resumos no formato dos testes anteriores do usuário (`[VEIL] janela ...` e `[VEIL] BENCH ...`), para comparar execuções.
- Flags para ligar e desligar neblina, texturas, sprites e efeitos, para isolar gargalos.

Não confunda compilar com funcionar. A ROM precisa ser executada e testada.

Registre separadamente o que foi verificado em emulador e o que foi efetivamente testado em hardware original. Nunca afirme que o hardware real foi testado se isso não aconteceu.

# 14. ESCOPO DE IMPLEMENTAÇÃO

Trabalhe em etapas para evitar uma demo visualmente ambiciosa, mas incompleta.

## Fase 0 — Ambiente e prova de vida

Seção 0 inteira: verificar e recuperar ambiente, `setup_env.sh`, `run_ares.ps1`, `AMBIENTE.md`, log de memória com Expansion Pak e decisão de arte (`ARTE.md`) com o **teste de arte** da seção 3.4 aprovado pelo usuário.

## Fase 1 — Vertical slice do Druida

Entregue, nesta ordem (cada passo compilado, executado no Ares e capturado antes do próximo):

1. Câmera inclinada, uma sala modular com luz assada, colisão em grade e o Druida andando com a animação de caminhada, **a 60 FPS medidos no emulador e com o HUD ligado**.
2. Ataque básico e Spirit Bolt com seleção automática de alvo.
3. Esqueleto e cão infernal com IA, preparação de ataque, dano, morte e feedback visual.
4. Thorn Burst.
5. Spirit Wolves.
6. Shapeshift funcional.
7. Vida, dano, morte do jogador e reinício.
8. O modo `AUTOPLAY` com estatísticas.

A primeira fase precisa ser jogável de verdade antes de expandir o conteúdo. Se o prazo apertar, entregue os passos 1 a 5 funcionando bem, em vez dos oito pela metade.

## Fase 2 — Primeira masmorra

- Conjunto de salas modulares.
- Portas e conexões funcionais.
- Geração com sementes e o teste no PC da seção 9.4.
- Inimigos posicionados corretamente.
- Sala de tesouro.
- Sala de elite.
- Validação de conectividade.

## Fase 3 — Incursão completa

- Cultista Profanado.
- Guardião Sepulcral.
- Chefe The Hollow Warden.
- Recompensas.
- Morte e reinício.
- Tela de vitória.
- Fluxo completo de entrada, exploração, combate, chefe e conclusão.

## Fase 4 — Roguelike e progressão

- Itens com efeitos reais.
- Relíquias.
- Variação de encontros.
- Diferentes combinações de salas.
- Ajustes de dificuldade.
- Recompensas por exploração.

## Fase 5 — Polimento, áudio e desempenho

- Áudio (seção 12), sem romper o 60 FPS.
- Corrigir problemas visuais.
- Melhorar animações.
- Ajustar colisões e resposta dos controles.
- Reduzir custos de processamento até cumprir 13.3 nas piores cenas (chefe, vários inimigos, os quatro poderes).
- Verificar uso de memória em 8 MB.
- Testar diferentes sementes.
- Preparar o pacote de teste para o hardware original.

Não inicie a expansão de classes enquanto a experiência básica do Druida não estiver satisfatória.

# 15. CLASSES FUTURAS — NÃO IMPLEMENTAR AGORA

O jogo poderá crescer para outras classes, mas elas não fazem parte dos requisitos obrigatórios da primeira demo.

**Cavaleiro Negro:** combatente corpo a corpo, com espada pesada, resistência, golpes amplos e ataques de execução.

**Caçador de Bruxas:** combatente à distância, com besta, armadilhas e movimentação tática.

**Necromante:** especialista em magia profana e invocações, com esqueletos e criaturas temporárias. Deve ser considerado apenas depois que o sistema de IA e invocação estiver estável.

Não crie interfaces, personagens incompletos ou habilidades falsas para essas classes na primeira versão. Preserve uma arquitetura que permita adicioná-las posteriormente.

# 16. CRITÉRIOS DE ACEITAÇÃO

A primeira versão completa de VEIL OF THE DAMNED só deve ser considerada entregue quando:

1. A ROM compilar com a toolchain do WSL e o ambiente puder ser recriado pelo `setup_env.sh`.
2. O Druida tiver identidade visual própria e animações reconhecíveis, e o **teste de arte** tiver sido aprovado pelo usuário.
3. A movimentação e as colisões funcionarem.
4. As quatro habilidades causarem efeitos reais no gameplay.
5. Os lobos seguirem, atacarem e trocarem de alvo corretamente.
6. Os inimigos possuírem comportamentos diferentes.
7. Ataques perigosos apresentarem preparação visual ou comportamental.
8. O jogador puder morrer e iniciar uma nova incursão.
9. A geração procedural produzir mapas conectados e diferentes conforme a semente, validada no PC com centenas de sementes.
10. A entrada, os caminhos principais e a sala do chefe forem acessíveis.
11. A luta contra o chefe puder ser vencida.
12. As recompensas modificarem o comportamento ou os atributos do personagem.
13. A interface permanecer legível.
14. **O jogo detectar e usar o Expansion Pak (8 MB), com orçamento de memória por categoria registrado e sem alocação dinâmica no combate.**
15. **O desempenho for medido no modo `AUTOPLAY` no emulador, com resultados reais (FPS médio, maior quadro, quadros acima de 16,7 ms e de 33 ms) em exploração, combate e chefe, e o procedimento e a tabela para o teste no N64 real estiverem prontos. A meta é 60 FPS; o que não for alcançado deve constar como falha conhecida, com o que foi tentado.**
16. Os testes realizados e suas limitações estiverem documentados, separando emulador de hardware real.
17. `CREDITS.md` listar todo recurso de terceiros usado, com licença.

# 17. O QUE NÃO FAZER

- Não entregar apenas uma sala vazia com um personagem andando.
- Não usar quadrados coloridos ou caixas montadas no código como substitutos permanentes de personagens (inclui o `models.c` atual).
- Não transformar todos os inimigos em cópias com cores diferentes.
- Não fazer as habilidades apenas trocarem a cor do projétil.
- Não permitir que inimigos atravessem paredes.
- Não criar mapas aleatórios sem verificar sua conectividade.
- Não implementar itens que não produzem efeito.
- Não esconder problemas de colisão com partículas.
- Não criar uma interface cheia de elementos minúsculos.
- Não sacrificar a resposta dos controles para adicionar efeitos visuais.
- Não começar a criar várias classes antes de terminar o Druida.
- Não declarar a demo concluída só porque a compilação terminou.
- Não gastar tempo com sistemas complexos que não contribuem para a experiência jogável inicial.
- Não subir a resolução nem usar transparência em larga escala para "compensar" o Expansion Pak.
- Não afirmar 60 FPS no N64 real com base em medições de emulador.
- Não instalar ferramentas fora do que a seção 0 autoriza, nem mexer em outros projetos.

# 18. ENTREGA ESPERADA

Trabalhe diretamente no projeto da pasta `C:\Desenvolvimento\Nintendo 64`.

Ao finalizar cada etapa, forneça:

- ROM compilada (`veil_of_the_damned.z64`, e `veil_of_the_damned_autoplay.z64` com `AUTOPLAY=1`).
- Capturas de tela do Ares em `capturas\`.
- Lista objetiva das funcionalidades implementadas.
- Resumo dos arquivos e sistemas alterados.
- Resultado dos testes efetivamente executados (emulador) e o que não foi testado.
- Tabela de desempenho com as colunas "emulador" (medida) e "N64 real" (vazia, para o usuário).
- Problemas conhecidos e limitações.
- Próxima prioridade técnica.

Mantenha `RESULTADOS.md`, `AMBIENTE.md`, `ARTE.md` e `CREDITS.md` atualizados. Não apresente uma lista de funcionalidades como concluídas se elas ainda forem apenas planejadas. Se algo bloquear de verdade (download, permissão, erro que exija decisão), pare nesse ponto, descreva o impedimento com a mensagem de erro real e o que já foi tentado, e aguarde o usuário.

**Comece pela Fase 0 e depois pela Fase 1.** Primeiro estabeleça o ambiente e a prova de vida, depois a decisão de arte com o teste de arte, e então movimentação, combate, identidade visual e um inimigo funcional, tudo a 60 FPS medidos. Depois integre as demais habilidades e expanda o cenário. Preserve a qualidade e a coerência do projeto antes de aumentar sua escala.

O objetivo não é demonstrar quantos recursos podem ser colocados na tela. É construir um jogo pequeno que tenha personalidade, combate satisfatório, 60 FPS e uma base técnica sólida para se transformar em um roguelike gótico completo para Nintendo 64.
