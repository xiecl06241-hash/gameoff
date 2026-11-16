"""Run the five-feedback regression in isolated rendered processes and protect player data."""
import argparse
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


def run(language, editor, width, height):
    settings = [folder / f'Config/{platform}/GameUserSettings.ini' for folder in SAVED_DIRS
                for platform in ('Windows', 'WindowsEditor')]
    original_settings = {p: p.read_bytes() if p.exists() else None for p in settings}
    before = save_hashes()
    out = ROOT / 'Saved/QA'
    out.mkdir(parents=True, exist_ok=True)
    log = out / f'feedback_{language}_{width}.log'
    args = ([r'D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
             str(ROOT / 'ShiftStore.uproject'), '/Game/ShiftStore/Maps/Normal', '-game'] if editor else
            [str(ROOT / 'Builds/Windows/ShiftStore.exe'), '/Game/ShiftStore/Maps/Normal'])
    args += ['-ShiftFeedbackTest', '-ShiftLanguage=' + language, '-windowed', '-RenderOffscreen',
             '-ForceRes', f'-ResX={width}', f'-ResY={height}', '-unattended', '-nosound',
             '-abslog=' + log.as_posix()]
    started = time.time()
    process = None
    try:
        process = subprocess.Popen(args, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        code = process.wait(timeout=120)
        assert save_hashes() == before, 'Feedback probe changed a player checkpoint'
        reports = [folder / f'Feedback/{language}/report.json' for folder in SAVED_DIRS]
        fresh = [p for p in reports if p.exists() and p.stat().st_mtime >= started]
        assert fresh, f'No fresh report; inspect {log}'
        source = max(fresh, key=lambda p: p.stat().st_mtime)
        report = json.loads(source.read_text(encoding='utf-8-sig'))
        failures = [name for name, passed in report['checks'].items() if not passed]
        assert code == 0 and report['passed'] and not failures, f'{language}: {failures}; exit={code}'
        screenshots = ['Work', 'Behind', 'Target', 'Deliver', 'BlockedRoute', 'ShiftRoute', 'Courtesy', 'Grab']
        for name in screenshots:
            picture = source.parent / f'{name}.png'
            assert picture.exists() and picture.stat().st_mtime >= started, f'Missing screenshot: {name}'
        previews = ROOT / 'Previews/Feedback'
        previews.mkdir(parents=True, exist_ok=True)
        selected = ['Work', 'Grab'] + (['Target', 'Behind', 'Courtesy', 'BlockedRoute'] if language == 'zh-Hans' else [])
        for name in selected:
            shutil.copy2(source.parent / f'{name}.png', previews / f'{language}_{width}_{name}.png')
        report.update(process_exit_code=code, player_saves_unchanged=True, settings_restored=True,
                      resolution=[width, height], editor=editor, screenshot_count=len(screenshots))
        (ROOT / f'Verification/feedback_{language}_{width}.json').write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({'language': language, 'resolution': [width, height],
                          'checks': len(report['checks']), 'passed': True}), flush=True)
    finally:
        if process is not None and process.poll() is None:
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True)
        for path, contents in original_settings.items():
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
