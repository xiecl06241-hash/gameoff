"""Exercise mouse sensitivity UI, camera scaling and relaunch persistence, restoring player settings."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
SAVED_DIRS = [ROOT / 'Saved', ROOT / 'Builds/Windows/ShiftStore/Saved',
              Path(os.environ['LOCALAPPDATA']) / 'ShiftStore/Saved']


def save_hashes():
    paths = [folder / 'SaveGames/ShiftStoreSlot.sav' for folder in SAVED_DIRS]
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None for p in paths}


def language_code(contents):
    if contents is None:
        return None
    config = configparser.ConfigParser(strict=False, interpolation=None)
    config.read_string(contents.decode('utf-8-sig'))
    return config.get('ShiftStore.Language', 'Code', fallback=None)


def run(language, editor, width, height):
    settings = [folder / f'Config/{platform}/GameUserSettings.ini' for folder in SAVED_DIRS
                for platform in ('Windows', 'WindowsEditor')]
    original = {p: p.read_bytes() if p.exists() else None for p in settings}
    saves_before = save_hashes()
    out = ROOT / 'Saved/QA'
    out.mkdir(parents=True, exist_ok=True)
    base = ([r'D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
             str(ROOT / 'ShiftStore.uproject'), '/Game/ShiftStore/Maps/Normal', '-game'] if editor else
            [str(ROOT / 'Builds/Windows/ShiftStore.exe'), '/Game/ShiftStore/Maps/Normal'])
    base += ['-ShiftSensitivityTest', '-ShiftLanguage=' + language, '-windowed', '-RenderOffscreen',
             '-ForceRes', f'-ResX={width}', f'-ResY={height}', '-unattended', '-nosound']
    process = None

    def launch(readback=False):
        nonlocal process
        started = time.time()
        suffix = '_readback' if readback else ''
        args = base + ['-abslog=' + (out / f'sensitivity_{language}_{width}{suffix}.log').as_posix()]
        if readback:
            args.append('-ShiftSensitivityReadback=1.5')
        process = subprocess.Popen(args, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        exit_code = process.wait(timeout=90)
        reports = [folder / f'Sensitivity/{language}/report.json' for folder in SAVED_DIRS]
        fresh = [p for p in reports if p.exists() and p.stat().st_mtime >= started]
        assert fresh, f'No fresh report for {language}; inspect Saved/QA/sensitivity logs'
        path = max(fresh, key=lambda p: p.stat().st_mtime)
        report = json.loads(path.read_text(encoding='utf-8-sig'))
        failures = [name for name, passed in report['checks'].items() if not passed]
        assert exit_code == 0 and report['passed'] and not failures, f'{language}: {failures}, exit {exit_code}'
        return path, report, started

    try:
        source, report, started = launch()
        screenshot = source.parent / 'Settings.png'
        assert screenshot.exists() and screenshot.stat().st_mtime >= started, 'No settings screenshot'
        assert save_hashes() == saves_before, 'Mouse settings changed a player checkpoint'
        for path, before in original.items():
            after = path.read_bytes() if path.exists() else None
            assert language_code(before) == language_code(after), f'Mouse settings changed language: {path}'
        _, readback, _ = launch(readback=True)
        assert readback['checks'].get('sensitivity_restored_after_relaunch')
        assert save_hashes() == saves_before, 'Relaunch changed a player checkpoint'
        previews = ROOT / 'Previews/Settings'
        previews.mkdir(parents=True, exist_ok=True)
        shutil.copy2(screenshot, previews / f'{language}_{width}.png')
        report['checks'].update(readback['checks'])
        report.update(process_exit_code=0, player_save_and_language_unchanged=True,
                      original_settings_restored=True, resolution=[width, height], editor=editor)
        (ROOT / f'Verification/sensitivity_{language}_{width}.json').write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({'language': language, 'resolution': [width, height],
                          'checks': len(report['checks']), 'passed': True}), flush=True)
    finally:
        if process is not None and process.poll() is None:
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True)
        for path, contents in original.items():
            if contents is None:
                path.unlink(missing_ok=True)
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(contents)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', action='store_true')
    parser.add_argument('--languages', nargs='+', choices=['en', 'ja', 'zh-Hant', 'zh-Hans'],
                        default=['en', 'ja', 'zh-Hant', 'zh-Hans'])
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=720)
    options = parser.parse_args()
    for language in options.languages:
        run(language, options.editor, options.width, options.height)
