"""Validate catalogues, dynamic arguments, source coverage and bundled font glyphs."""
import hashlib
import json
from pathlib import Path
import re
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]


def main():
    packs = {code: json.loads((ROOT / f"Content/Localization/Strings/{code}.json").read_text(encoding="utf-8"))
             for code in ["en", "ja", "zh-Hant", "zh-Hans"]}
    english = packs["en"]
    for code, pack in packs.items():
        assert pack.keys() == english.keys(), f"{code}: catalogue keys differ"
        for source, value in pack.items():
            assert value.strip(), f"{code}: empty {source}"
            assert sorted(re.findall(r"\{\d+\}", value)) == sorted(re.findall(r"\{\d+\}", english[source])), f"{code}: argument mismatch: {source}"

    excluded = {"S H I F T  /  2 4", "Language / 言語 / 語言 / 语言"}
    expressions = [
        r'(?:ShiftLoc::Format|DrawCenteredText|DrawLocalizedText|SetStatusBanner|TriggerGameOver)\(TEXT\("((?:\\.|[^"\\])*)"\)',
        r'CrosshairPrompt\s*=\s*TEXT\("((?:\\.|[^"\\])*)"\)',
        r'Speech->SetText\(FText::FromString\(TEXT\("((?:\\.|[^"\\])*)"\)',
        r'N[123]\.(?:Title|Intro)\s*=\s*TEXT\("((?:\\.|[^"\\])*)"\)',
        r'SubtitleText\s*=\s*TEXT\("((?:\\.|[^"\\])*)"\)',
        r'(?<![.\w])Add\(TEXT\("((?:\\.|[^"\\])*)"\)',
    ]
    checked = set()
    for path in (ROOT / "Source/ShiftStore").glob("*.cpp"):
        if "SelfTest" in path.name or "LocalizationTest" in path.name:
            continue
        text = path.read_text(encoding="utf-8")
        for expression in expressions:
            for literal in re.findall(expression, text):
                source = json.loads('"' + literal + '"')
                if source not in excluded:
                    assert source in english, f"Untranslated UI text in {path.name}: {source}"
                    checked.add(source)

    manifest = json.loads((ROOT / "SourceArt/Fonts/sources.json").read_text(encoding="utf-8"))
    glyph_counts = {}
    for code, suffix in [("ja", "jp"), ("zh-Hant", "tc"), ("zh-Hans", "sc")]:
        path = ROOT / f"SourceArt/Fonts/NotoSansCJK{suffix}-Regular.otf"
        assert hashlib.sha256(path.read_bytes()).hexdigest() == manifest["files"][path.name]["sha256"]
        font = TTFont(path)
        glyphs = font.getBestCmap()
        text = "".join(packs[code].values()) + "English日本語繁體中文简体中文Language / 言語 / 語言 / 语言"
        missing = sorted({char for char in text if not char.isspace() and ord(char) not in glyphs})
        assert not missing, f"{code}: font missing {missing}"
        glyph_counts[code] = len(set(text))
    report = {"passed": True, "languages": list(packs), "entries_per_pack": len(english),
              "source_strings_checked": len(checked), "font_characters_checked": glyph_counts}
    dest = ROOT / "Verification/localization_catalogues.json"
    dest.parent.mkdir(exist_ok=True)
    dest.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
