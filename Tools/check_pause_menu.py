"""Exercise pause/resume, frozen gameplay, mouse/keyboard menus and Quit in the Windows build."""
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


def persistent_files():
    return [folder / 'SaveGames/ShiftStoreSlot.sav' for folder in SAVED_DIRS] + [
        folder / f'Config/{platform}/GameUserSettings.ini' for folder in SAVED_DIRS
        for platform in ('Windows', 'WindowsEditor')]


def hashes():
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
            for p in persistent_files()}


def run(language, editor, width, height):
    before = hashes()
    output = ROOT / 'Saved/QA'
    output.mkdir(parents=True, exist_ok=True)
    log = output / f'pause_{language}_{width}.log'
    if editor:
        args = [r'D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
                str(ROOT / 'ShiftStore.uproject'), '/Game/ShiftStore/Maps/Normal', '-game']
    else:
        args = [str(ROOT / 'Builds/Windows/ShiftStore.exe'), '/Game/ShiftStore/Maps/Normal']
    args += ['-ShiftPauseTest', '-ShiftLanguage=' + language, '-windowed', '-RenderOffscreen',
             '-ForceRes', f'-ResX={width}', f'-ResY={height}', '-unattended', '-nosound',
             '-abslog=' + log.as_posix()]
    started = time.time()
    process = subprocess.Popen(args, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    try:
        exit_code = process.wait(timeout=90)
    except subprocess.TimeoutExpired:
        subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True)
        raise RuntimeError(f'Pause menu / Quit timed out; inspect {log}')
    assert hashes() == before, 'Pause test changed a player save or language setting'
    reports = [folder / f'PauseMenu/{language}/report.json' for folder in SAVED_DIRS]
    fresh = [p for p in reports if p.exists() and p.stat().st_mtime >= started]
    assert fresh, f'No fresh report; inspect {log}'
    source = max(fresh, key=lambda p: p.stat().st_mtime)
    report = json.loads(source.read_text(encoding='utf-8-sig'))
    assert exit_code == 0 and report['passed'] and all(report['checks'].values()), report
    screenshot = source.parent / 'Pause.png'
    assert screenshot.exists() and screenshot.stat().st_mtime >= started, 'No pause screenshot'
    preview = ROOT / 'Previews/PauseMenu'
    preview.mkdir(parents=True, exist_ok=True)
    shutil.copy2(screenshot, preview / f'{language}_{width}.png')
    report.update(process_exit_code=exit_code, quit_menu_exited_process=True,
                  player_save_and_settings_unchanged=True, resolution=[width, height], editor=editor)
    target = ROOT / f'Verification/pause_menu_{language}_{width}.json'
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'language': language, 'checks': len(report['checks']),
                      'exit_code': exit_code, 'passed': True}), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', action='store_true')
    parser.add_argument('--languages', nargs='+', choices=['en', 'ja', 'zh-Hant', 'zh-Hans'],
                        default=['en', 'ja', 'zh-Hant', 'zh-Hans'])
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=720)
    options = parser.parse_args()
    for code in options.languages:
        run(code, options.editor, options.width, options.height)
