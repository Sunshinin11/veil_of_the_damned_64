"""Gera include/map_chunks.h: pedacos de mapa (6x6 tiles) que o jogo encaixa a cada partida,
no estilo Diablo II (src/mapgen.c monta o layout, os muros entre salas e o chao/trilha).

Regras de cada pedaco:
  - 6 linhas x 6 colunas (so o miolo; os muros entre pedacos sao do montador).
  - As "faixas" (linhas 2-3 e colunas 2-3) ficam livres: e por elas que o druida atravessa
    de uma saida a outra. Nelas so pode chao, decalques, marcadores, E, P, ~ e Y.
  - Categoria: N normal, D beco (tesouro), G guardiao (tem q), S inicio (tem E), X saida (tem P).
Rode depois de mexer:  python tools/gen_chunks.py
"""
from pathlib import Path

LANE_OK = set(".bvkosSZaqCEP~Ylgmdt")

# ------------------------------------------------------------ mapa 1: Caminho dos Condenados
# T lapide X cruz U estatua R arvore seca K arbusto F fogueira p santuario J cerca ~ corpo Y ossos
CEM = {
 "N": [
  ["TX..RK", "X.....", "..s...", "......", ".T..XT", "RK..T."],
  ["R....R", ".T..XT", "......", "...S..", ".X..T.", "K....R"],
  ["RR..KR", "R.....", "..Z...", "...a..", "T....X", "XT..TR"],
  ["U....U", ".o....", "..v...", "...s..", "....o.", "T.Y..X"],
  ["JJ..JJ", "TX..XT", "..S...", "......", "XT..TX", "JJ..JJ"],
  ["RF...R", "~.....", "..k...", "...Z..", "......", "RK..YR"],
  ["KR..RK", "RT....", "..a...", "..s...", "....XR", "RK..KR"],
  ["XU...T", ".o....", "......", "..S...", "....o.", "T...UX"],
  ["RK..RR", "R.....", "..b.s.", "......", "RT...K", "RT..RR"],
 ],
 "D": [   # beco: tesouro
  ["RC..CR", "R.....", "..s...", "......", "T....X", "RK..KR"],
  ["U.C..U", ".Y....", "..a...", "......", ".T..X.", "RR..RR"],
  ["JC..CJ", "J.o...", "......", "..S...", "T....X", "RR..RR"],
 ],
 "G": [   # guardiao + escolta
  ["U....U", ".T..X.", "..q...", "......", ".X..T.", "RF...R"],
  ["RX..TR", "R..o.R", "..v...", "...q..", "K....K", "RR..RR"],
  ["FU...F", "......", "...q..", "..o...", "T....X", "RK..KR"],
 ],
 "S": [   # inicio (fogueira do acampamento)
  ["R....R", ".F..F.", "......", "...E..", "..~...", "RY..KR"],
  ["KR..RK", "RF....", "......", "...E..", "Y.....", "RR..RR"],
 ],
 "X": [   # saida: o portal aparece no P quando os guardioes caem
  ["UF...U", "......", "......", "...P..", "......", "TU...X"],
  ["RF..FR", "......", "...P..", "......", "X....T", "RR..RR"],
 ],
}

# ------------------------------------------------------------ mapa 2: Cripta (caverna)
# L coluna Q sarcofago A altar F braseiro V W B pedras H cogumelos I M cristais U estatua ~ corpo Y ossos
CRYPT = {
 "N": [
  ["L....L", ".V....", "..s...", "......", "....H.", "L....L"],
  ["QF...Q", "......", "..S...", "......", ".I..M.", "H....V"],
  ["LL..LL", "L.....", "..Z...", "...a..", "......", "LV..HL"],
  ["I....M", ".k....", "..v...", "...s..", "....k.", "M.Y..I"],
  ["Q....Q", "Q.....", "..S...", "......", ".....Q", "Q....Q"],
  ["FL...F", "......", "..a...", "...Z..", "......", "HL...W"],
  ["VB..WV", "H.....", "..k...", "..s...", "M.....", "VH..BV"],
 ],
 "D": [
  ["LC..CL", "L.....", "..s...", "......", "H....V", "LL..LL"],
  ["QC..CQ", ".Y....", "..a...", "......", ".I..M.", "LL..LL"],
 ],
 "G": [
  ["LU...L", "......", "..q...", "......", "F....F", "L....L"],
  ["AF...L", "......", "...q..", "..o...", "L....H", "LV..VL"],
  ["U....U", ".L..L.", "..q...", "......", ".L..L.", "Q....Q"],
 ],
 "S": [   # E: a boca da caverna fica 1 tile ao sul (precisa de espaco)
  ["L....L", ".F..F.", "......", "...E..", "......", "H....V"],
  ["LL..LL", "L....L", "..k...", "...E..", "......", "VH..HV"],
 ],
 "X": [   # P: o trono do guardiao fica ao norte do portal
  ["LF...L", "......", "......", "...P..", "......", "FL...F"],
  ["LF..FL", "......", "......", "...P..", "I....M", "LL..LL"],
 ],
}

# ------------------------------------------------------------ campanha 2, fase 1: Floresta Morta
# R arvore seca K arbusto G arvore gigante H cogumelos V pedras Y ossos ~ corpo F fogueira U pedra antiga
FOREST = {
 "N": [
  ["RR..KR", "R.....", "..s...", "......", "K....R", "RH..RR"],
  ["G....R", "......", "......", "...S..", "H....K", "RK..RR"],
  ["RK..HR", "K.....", "..Z...", "...a..", "R....V", "RR..KR"],
  ["R.v..R", "H.....", "......", "..s...", "....HK", "RK..YR"],
  ["RG...R", "R.....", "..a...", "...S..", "V.....", "KR..RR"],
  ["HR..RH", "R....K", "..k...", "...Z..", "K....R", "RR..RR"],
  ["R....G", "KH....", "..o...", "..s...", "......", "RV..KR"],
  ["RK..RR", "R.....", "..b...", "...S..", "~....R", "RK..RR"],
 ],
 "D": [
  ["RC..CR", "R.....", "..s...", "......", "H....K", "RR..RR"],
  ["GC...R", "R....C", "..a...", "......", "K....V", "RR..RR"],
  ["RC..HR", "K.Y...", "..S...", "......", "R....K", "RR..RR"],
 ],
 "G": [
  ["U....U", ".R..K.", "..q...", "......", ".H..R.", "RF...R"],
  ["RG...R", "R..o.K", "..v...", "...q..", "K....H", "RR..RR"],
  ["FU...F", "......", "...q..", "..o...", "R....K", "RK..KR"],
 ],
 "S": [
  ["R....R", ".F..F.", "......", "...E..", "..~...", "RY..KR"],
  ["KR..RK", "RF....", "......", "...E..", "Y.....", "RR..RR"],
 ],
 "X": [
  ["UF...U", "......", "......", "...P..", "......", "RU...R"],
  ["RF..FR", "......", "...P..", "......", "K....R", "RR..RR"],
 ],
}

# ------------------------------------------------------------ campanha 2, fase 2: Pantano Podre
# R arvore de mangue K juncos H cogumelos V tronco caido W poca grande (deixa lento) w poca pequena
# U totem de bruxa F lanterna de fogo-fatuo B barco afundado h cabana Y ossos l vitorias-regias g limo m lama
SWAMP = {
 "N": [
  ["RK..WR", "K....W", "..s...", "......", "w....K", "RH..KR"],
  ["R....R", ".W..l.", "......", "...S..", "K.m..w", "RK..RR"],
  ["WW..RK", "W.....", "..Z...", "...a..", "R....H", "KR..WW"],
  ["R.g..R", "H....w", "......", "..s...", "....HK", "RK..YR"],
  ["RW...R", "K....B", "..a...", "...S..", "V.....", "KR..RW"],
  ["HR..RH", "R....K", "..k...", "...Z..", "w....R", "RR..WR"],
  ["R....W", "KH....", "..l...", "..s...", "......", "RV..KR"],
  ["RK..RR", "R....h", "..g...", "...S..", "~....R", "RK..WR"],
 ],
 "D": [
  ["RC..CR", "R.....", "..s...", "......", "H....w", "RR..RR"],
  ["WC...R", "W....C", "..a...", "......", "K....V", "RR..RR"],
  ["RC..HR", "K.Y...", "..S...", "......", "R....W", "RR..WW"],
 ],
 "G": [
  ["UW...U", ".R..K.", "..q...", "......", ".H..R.", "RF...R"],
  ["RW...R", "R..m.K", "..g...", "...q..", "K....H", "RR..RR"],
  ["FU...F", "......", "...q..", "..m...", "R....W", "RK..KR"],
 ],
 "S": [
  ["R....R", ".F..F.", "......", "...E..", "..~...", "RY..KR"],
  ["KR..RK", "RF....", "......", "...E..", "Y....W", "RR..RR"],
 ],
 "X": [
  ["UF...U", "......", "......", "...P..", "......", "RU...R"],
  ["RF..FR", "......", "...P..", "......", "W....K", "RR..RR"],
 ],
}

# ------------------------------------------------------------ campanha 3 (Norte), fase 1: Passo Congelado
# R pinheiro K pedrinhas V rocha grande U pedra runica B carroca ~ guerreiro congelado N estandarte
# J palicada Y ossos de mamute F fogueira I placa de gelo (escorrega) d neve acumulada t pegadas
SNOW = {
 "N": [
  ["RK..VR", "R....I", "..s...", "......", "K....d", "RV..KR"],
  ["R....R", ".U..t.", "......", "...S..", "KI...V", "RK..RR"],
  ["RR..RK", "R....I", "..Z...", "...a..", "V....K", "KR..RR"],
  ["R.d..R", "K....~", "......", "..s...", "....KV", "RK..YR"],
  ["JJ..JJ", "RV....", "..a...", "...S..", "B....K", "JJ..JJ"],
  ["KR..RK", "R....K", "..t...", "...Z..", "I....R", "RR..VR"],
  ["R....V", "KI....", "..d...", "..s...", "......", "RV..KR"],
  ["RK..RR", "R....N", "..b...", "...S..", "~....R", "RK..IR"],
 ],
 "D": [
  ["RC..CR", "R.....", "..s...", "......", "K....V", "RR..RR"],
  ["VC...R", "I....C", "..a...", "......", "K....B", "RR..RR"],
  ["RC..KR", "K.Y...", "..S...", "......", "R....I", "RR..RR"],
 ],
 "G": [
  ["UN...U", ".R..K.", "..q...", "......", ".V..R.", "RF...R"],
  ["RI...R", "R..d.K", "..t...", "...q..", "K....V", "RR..RR"],
  ["FU...F", "......", "...q..", "..d...", "R....I", "RK..KR"],
 ],
 "S": [
  ["R....R", ".F..F.", "......", "...E..", "..~...", "RY..KR"],
  ["KR..RK", "RF....", "......", "...E..", "B....I", "RR..RR"],
 ],
 "X": [
  ["UF...U", "......", "......", "...P..", "......", "RU...R"],
  ["RF..FR", "......", "...P..", "......", "I....K", "RR..RR"],
 ],
}

# ------------------------------------------------------------ campanha 3 (Norte), fase 2: Cavernas de Gelo
# L estalagmite de gelo I M cristais Q guerreiro no gelo w lobo no gelo A mamute F braseiro azul
# V cristal pequeno H neve B W ossos k rachadura o agua congelada g brilho
ICE = {
 "N": [
  ["L....L", ".V....", "..s...", "......", "....H.", "L....L"],
  ["QF...Q", "......", "..S...", "......", ".I..M.", "H....V"],
  ["LL..LL", "L.....", "..Z...", "...a..", "......", "LV..HL"],
  ["I....M", ".k....", "..o...", "...s..", "....k.", "MB...I"],
  ["w....Q", "w.....", "..S...", "......", ".....w", "Q....w"],
  ["FL...F", "......", "..a...", "...Z..", "......", "HL...W"],
  ["VB..WV", "H.....", "..g...", "..s...", "M.....", "VH..BV"],
 ],
 "D": [
  ["LC..CL", "L.....", "..s...", "......", "H....V", "LL..LL"],
  ["QC..CQ", ".B....", "..a...", "......", ".I..M.", "LL..LL"],
 ],
 "G": [
  ["LQ...L", "......", "..q...", "......", "F....F", "L....L"],
  ["AF...L", "......", "...q..", "..o...", "L....H", "LV..VL"],
  ["Q....Q", ".L..L.", "..q...", "......", ".L..L.", "w....w"],
 ],
 "S": [
  ["L....L", ".F..F.", "......", "...E..", "......", "H....V"],
  ["LL..LL", "L....L", "..k...", "...E..", "......", "VH..HV"],
 ],
 "X": [
  ["LF...L", "......", "......", "...P..", "......", "FL...F"],
  ["LF..FL", "......", "......", "...P..", "I....M", "LL..LL"],
 ],
}

def check(name, sets):
    for cat, lst in sets.items():
        for i, t in enumerate(lst):
            assert len(t) == 6 and all(len(r) == 6 for r in t), (name, cat, i, "tamanho")
            for r in range(6):
                for c in range(6):
                    ch = t[r][c]
                    if (r in (2, 3) or c in (2, 3)) and ch not in LANE_OK:
                        raise SystemExit(f"{name} {cat}{i}: '{ch}' bloqueia a faixa em linha {r} coluna {c}")
            flat = "".join(t)
            if cat == "S": assert flat.count("E") == 1, (name, cat, i)
            if cat == "X": assert flat.count("P") == 1, (name, cat, i)
            if cat == "G": assert flat.count("q") == 1, (name, cat, i)


def emit(f, prefix, sets):
    for cat in "NDGSX":
        lst = sets[cat]
        f.write(f"static const char {prefix}_{cat}[{len(lst)}][37] = {{\n")
        for t in lst:
            f.write('    "' + "".join(t) + '",\n')
        f.write("};\n")
    f.write(f"static const ChunkSet {prefix}_SET = {{ {{ " +
            ", ".join(f"{prefix}_{c}" for c in "NDGSX") + " }, { " +
            ", ".join(str(len(sets[c])) for c in "NDGSX") + " } };\n\n")


check("cemiterio", CEM)
check("cripta", CRYPT)
check("floresta", FOREST)
check("pantano", SWAMP)
check("neve", SNOW)
check("gelo", ICE)
out = Path(__file__).resolve().parents[1] / "include" / "map_chunks.h"
with open(out, "w", encoding="utf-8") as f:
    f.write("// GERADO por tools/gen_chunks.py - edite la e rode de novo.\n#pragma once\n")
    f.write("// Pedaco = 36 caracteres (6x6, linha a linha). Categorias: N normal, D beco, G guardiao, S inicio, X saida.\n")
    f.write("typedef struct { const char (*list[5])[37]; int count[5]; } ChunkSet;\n\n")
    emit(f, "CHK_CEM", CEM)
    emit(f, "CHK_CRYPT", CRYPT)
    emit(f, "CHK_FOREST", FOREST)
    emit(f, "CHK_SWAMP", SWAMP)
    emit(f, "CHK_SNOW", SNOW)
    emit(f, "CHK_ICE", ICE)
print("ok ->", out, {k: len(v) for k, v in CEM.items()}, {k: len(v) for k, v in CRYPT.items()})
