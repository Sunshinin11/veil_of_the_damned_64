# Altar Negro — arena infernal

As imagens desta pasta são as artes fonte. `tools/build_scenery_assets.py` cria as versões reduzidas em `assets/scenery/`.

| Arquivo | Uso |
| --- | --- |
| `Piso obsidiana.png` | Piso repetido de pedra negra e fissuras vermelhas |
| `Parede obsidiana.png` | Painéis ao fundo da arena |
| `Trono Malakor frontal.png` | Trono frontal atrás da batalha |
| `Pentagrama.png` | Selo ritual no centro da arena |
| `Monolito.png` | Quatro marcos nas laterais |
| `Braseiro ritual.png`, `Braseiro ritual 1.png`, `Braseiro ritual 2.png` | Três quadros dos fogos infernais próximos ao trono |
| `Portal 0.png`, `Portal 1.png`, `Portal 2.png` | Três quadros do portal de entrada |
| `Altar sacrificial.png` | Altar central em sprite |

O mapa em `src/world.c` posiciona os elementos e mantém colisões nos monólitos, braseiros, altar e trono. Os objetos de cenário usam sprites; o piso continua um plano texturizado para manter a perspectiva. Conferir no emulador a ordem das camadas, a leitura do pentagrama sobre o piso e o espaço livre para lutar contra Malakor.
