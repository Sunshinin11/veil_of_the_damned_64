"""Copy the user's MP3s to stable ASCII asset names for the N64 build."""
from pathlib import Path
from shutil import copy2

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "audio"

SFX = {
    "druid_swing": "audio-Generate-one-isolated-SFX-asset*",
    "beast_swing": "audio-Garras-do-Lobisomem*",
    "player_hurt": "audio-Jogador-ferido*",
    "dodge": "audio-Esquiva-*",
    "shapeshift": "audio-ransforma-*",
    "potion": "audio-Po-o-*",
    "bolt_cast": "audio-Spirit-Bolt-lan-ado-*",
    "bolt_hit": "audio-Spirit-Bolt-atingindo-*",
    "thorns": "audio-Espinhos-*",
    "wolf_summon": "audio-Invoca-o-do-lobo-*",
    "wolf_howl": "audio-Uivo-espiritual-*",
    "wolf_bite": "audio-Mordida-espiritual-*",
    "skeleton_swing": "audio-Ataque-do-esqueleto-*",
    "skeleton_hurt": "audio-Esqueleto-ferido-*",
    "skeleton_die": "audio-Morte-do-esqueleto-*",
    "demon_swing": "audio-Ataque-do-dem-nio-*",
    "demon_hurt": "audio-Dem-nio-ferido-*",
    "demon_die": "audio-Morte-do-dem-nio-*",
    "cultist_cast": "audio-Ataque-do-cultista-*",
    "cultist_hurt": "audio-Cultista-ferido-*",
    "cultist_die": "audio-Morte-do-cultista-*",
    "boss_slam": "audio-Golpe-de-Malakor-*",
    "boss_enrage": "audio-F-ria-de-Malakor-*",
    "boss_hurt": "audio-Malakor-ferido-*",
    "boss_die": "audio-Morte-de-Malakor-*",
    "chest": "audio-Ba-Generate-*",
    "pickup": "audio-Item-coletado-*",
    "gate": "audio-Port-o-aberto-*",
    "ui_select": "audio-Sele-o-na-HUD-*",
    "ui_unavailable": "audio-A-o-indispon-vel-*",
    "campfire": "audio-Fogueira-*",
    "infernal_fire": "audio-Braseiro-infernal-*",
}

MUSIC = {
    "title": "01 *",
    "path": "02 *",
    "crypt": "03 *",
    "combat": "04 *",
    "altar": "05 *",
    "boss": "06 *",
    "victory": "07 *",
    "defeat": "08 *",
}


def copy_group(directory: Path, mapping: dict[str, str], prefix: str) -> None:
    for name, pattern in mapping.items():
        matches = list(directory.glob(pattern))
        if len(matches) != 1:
            raise ValueError(f"{directory.name}/{pattern}: expected one MP3, got {len(matches)}")
        copy2(matches[0], OUT / f"{prefix}_{name}.mp3")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    copy_group(ROOT / "SFX", SFX, "sfx")
    copy_group(ROOT / "Soundtrack", MUSIC, "bgm")
    print(f"Prepared {len(SFX)} SFX and {len(MUSIC)} music tracks in {OUT}")


if __name__ == "__main__":
    main()
