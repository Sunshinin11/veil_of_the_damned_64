# Tarefas do projeto

## Lote de vídeos em andamento

- [x] Finalizar os idles em quatro direções do bárbaro e do personagem de capuz/cajado.
- [x] Substituir a corrida de frente do bárbaro e a corrida de costas do personagem de capuz/cajado.
- [x] Salvar PNGs transparentes, GIFs, cartelas e versões 64×64; integrar no jogo e gerar a ROM.

## Próxima etapa — criar Mago e separar o visual do Druida

Decisão do usuário em 22/10/2026: o personagem atual de capuz e cajado será o visual do **Mago**. O **Druida** receberá novos sprites com aparência mais animalesca, que o usuário ainda vai fornecer. O jogo também terá a classe **Ladino**.

- [x] Criar a classe jogável **Mago** como classe própria, mantendo o Druida separado.
- [x] Transferir **todos os sprites atuais do Druida** para o Mago, incluindo os idles e corridas de vídeo deste lote, ataques, conjurações, dano, morte, retratos e demais animações disponíveis.
- [x] Organizar as pastas e os nomes dos recursos do Mago e atualizar importadores e referências no jogo para que as artes não fiquem divididas entre as duas classes.
- [x] Atualizar seleção de personagem, nomes nos idiomas, retratos, desbloqueios e salvamento para reconhecer a nova classe sem confundir partidas existentes.
- [ ] Definir as habilidades e os atributos próprios do Mago; a transferência solicitada é dos sprites, não uma transferência automática das habilidades de natureza do Druida.
- [ ] Receber os novos sprites animalescos do Druida e substituir o visual dele em todas as animações.
- [ ] Conferir no jogo a separação visual entre Mago e Druida e gerar uma ROM com a migração completa.

## Nova classe — Ladino

- [x] Criar a classe **Ladino**.
- [ ] Definir visual, sprites, habilidades, atributos e progressão do Ladino com o usuário.
- [x] Integrar a classe à seleção de personagem, idiomas, desbloqueios e salvamento.

## Escopo atual do bárbaro

O restante do bárbaro está fechado. Neste lote, alterar somente o idle de quatro direções e a corrida de frente enviados pelo usuário. Preservar ataques, habilidades e demais corridas.

## Entrega de 22/10/2026

Mago criado e jogável, usando provisoriamente os atributos, habilidades e cartas do personagem de cajado. Druida mantém o visual compartilhado até receber a nova arte animalesca. Ladino criado e visível na seleção, mas bloqueado até ter sprites próprios, inclusive com o atalho de desbloqueio geral. Os IDs antigos e o formato dos arquivos de salvamento foram preservados; os cinco IDs de classe são reconhecidos.

- [x] Revisar a corrida de frente do bárbaro, escolhendo somente o trecho de machados baixos após 2 segundos.
- [x] Substituir as diagonais de frente do Mago pelo novo vídeo e seu espelho; salvar os pacotes e gerar a ROM.
