"""Exercise real menu input and capture localized screens, restoring player settings afterwards."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
SAVED_DIRS = [ROOT / "Saved", ROOT / "Builds/Windows/ShiftStore/Saved",
              Path(os.environ["LOCALAPPDATA"]) / "ShiftStore/Saved"]


def player_saves():
    return {str(path): hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None
            for folder in SAVED_DIRS for path in [folder / "SaveGames/ShiftStoreSlot.sav"]}


def run(code, editor=False, width=1920, height=1080):
    settings = [folder / f"Config/{platform}/GameUserSettings.ini"
                for folder in SAVED_DIRS for platform in ["Windows", "WindowsEditor"]]
    before_settings = {path: path.read_bytes() if path.exists() else None for path in settings}
    before_saves = player_saves()
    out = ROOT / "Saved/QA"
    out.mkdir(parents=True, exist_ok=True)
    log = out / f"localization_{code}_{width}.log"
    if editor:
        args = [r"D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe", str(ROOT / "ShiftStore.uproject"),
                "/Game/ShiftStore/Maps/Normal", "-game"]
    else:
        args = [str(ROOT / "Builds/Windows/ShiftStore.exe"), "/Game/ShiftStore/Maps/Normal"]
    args += ["-ShiftLocalizationTest", "-ShiftLanguage=" + code, "-windowed", "-RenderOffscreen",
             "-ForceRes", f"-ResX={width}", f"-ResY={height}", "-unattended", "-nosound",
             "-abslog=" + log.as_posix()]
    started = time.time()
    process = None
    try:
        with (out / f"localization_stdout_{code}.log").open("w", encoding="utf-8") as stdout:
            process = subprocess.Popen(args, cwd=ROOT, stdout=stdout, stderr=subprocess.STDOUT)
            exit_code = process.wait(timeout=120)
        reports = [folder / f"Localization/{code}/report.json" for folder in SAVED_DIRS]
        fresh = [path for path in reports if path.exists() and path.stat().st_mtime >= started]
        assert fresh, f"No runtime report for {code}; inspect {log}"
        source = max(fresh, key=lambda path: path.stat().st_mtime)
        report = json.loads(source.read_text(encoding="utf-8-sig"))
        print(json.dumps({"language": code, "exit": exit_code, "report": report}, ensure_ascii=False), flush=True)
        assert exit_code == 0 and report["passed"], f"Runtime checks failed for {code}"
        assert all(report["checks"].values()), report
        assert Path(report["settings_file"]).resolve() in {path.resolve() for path in settings}, "Unexpected settings file"
        dest = ROOT / "Previews/Localization"
        dest.mkdir(parents=True, exist_ok=True)
        screenshots = ["Language", "Title", "NightIntro", "HUD", "Customer", "GameOver", "Dawn", "Ending", "RestockTake", "RestockReturn", "TaskSwitch", "DepartureEarly", "DepartureDoorway", "CookOden"]
        for name in screenshots:
            image = source.parent / f"{name}.png"
            assert image.exists() and image.stat().st_mtime >= started, f"Missing screenshot {image}"
            if name in ["Language", "Title", "HUD", "Customer", "RestockTake", "RestockReturn", "TaskSwitch", "DepartureEarly", "DepartureDoorway", "CookOden"]:
                shutil.copy2(image, dest / f"{code}_{width}_{name}.png")
        assert player_saves() == before_saves, "Gameplay checkpoint changed"
        readback_args = [arg for arg in args if not arg.startswith(("-ShiftLanguage=", "-abslog="))]
        readback_args += ["-ShiftLocalizationReadback=" + code,
                          "-abslog=" + (out / f"localization_readback_engine_{code}.log").as_posix()]
        with (out / f"localization_readback_{code}.log").open("w", encoding="utf-8") as stdout:
            process = subprocess.Popen(readback_args, cwd=ROOT, stdout=stdout, stderr=subprocess.STDOUT)
            readback_exit = process.wait(timeout=90)
        readback = json.loads(source.read_text(encoding="utf-8-sig"))
        assert readback_exit == 0 and readback["checks"].get("saved_language_restored_after_relaunch"), readback
        report.update(process_exit_code=exit_code, player_saves_unchanged=True,
                      screenshot_count=len(screenshots), resolution=[width, height], editor=editor,
                      saved_language_restored_after_relaunch=True)
        (ROOT / f"Verification/localization_runtime_{code}_{width}.json").write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    finally:
        if process is not None and process.poll() is None:
            subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], capture_output=True)
        for path, content in before_settings.items():
            if content is None:
                path.unlink(missing_ok=True)
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(content)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--editor", action="store_true")
    parser.add_argument("--languages", nargs="+", choices=["en", "ja", "zh-Hant", "zh-Hans"], default=["en", "ja", "zh-Hant", "zh-Hans"])
    parser.add_argument("--width", type=int, default=1920)
    parser.add_argument("--height", type=int, default=1080)
    options = parser.parse_args()
    for language in options.languages:
        run(language, options.editor, options.width, options.height)
