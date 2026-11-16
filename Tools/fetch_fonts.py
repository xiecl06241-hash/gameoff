"""Fetch the pinned, SIL OFL licensed fonts used by the localized HUD."""
import concurrent.futures
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "SourceArt/Fonts"


def main():
    DEST.mkdir(parents=True, exist_ok=True)
    manifest_path = DEST / "sources.json"
    if manifest_path.exists():
        commit = json.loads(manifest_path.read_text(encoding="utf-8"))["commit"]
    else:
        request = urllib.request.Request(
            "https://api.github.com/repos/notofonts/noto-cjk/commits/main",
            headers={"User-Agent": "ShiftStore-font-import"},
        )
        with urllib.request.urlopen(request, timeout=60) as response:
            commit = json.load(response)["sha"]
    base = f"https://raw.githubusercontent.com/notofonts/noto-cjk/{commit}/Sans/"
    files = {
        "NotoSansCJKsc-Regular.otf": "OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf",
        "NotoSansCJKjp-Regular.otf": "OTF/Japanese/NotoSansCJKjp-Regular.otf",
        "NotoSansCJKtc-Regular.otf": "OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf",
        "LICENSE": "LICENSE",
    }

    def fetch(entry):
        name, relative = entry
        data = urllib.request.urlopen(base + relative, timeout=120).read()
        if name.endswith(".otf") and data[:4] != b"OTTO":
            raise ValueError(f"Invalid OpenType font: {name}")
        path = DEST / name
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
        return name, {"url": base + relative, "sha256": hashlib.sha256(data).hexdigest()}

    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        results = dict(pool.map(fetch, files.items()))
    manifest_path.write_text(json.dumps({"commit": commit, "files": results}, indent=2) + "\n", encoding="utf-8")
    print(f"Fetched {len(results)} files at {commit}")


if __name__ == "__main__":
    main()
