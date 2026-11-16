"""Run packaged regression checks, including the deliberately failing test path."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'Builds/Windows/ShiftStore.exe'
OUT = ROOT / 'Saved/QA'
OUT.mkdir(parents=True, exist_ok=True)
LOCAL = Path(os.environ['LOCALAPPDATA']) / 'ShiftStore/Saved'
SAVED_DIRS = (ROOT / 'Saved', ROOT / 'Builds/Windows/ShiftStore/Saved', LOCAL)


def player_saves():
    result = {}
    for folder in SAVED_DIRS:
        path = folder / 'SaveGames/ShiftStoreSlot.sav'
        result[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None
    return result


def run_case(force_failure=False):
    case = 'expected_failure' if force_failure else 'pass'
    before = player_saves()
    log = OUT / f'gameplay_{case}.log'
    args = [str(EXE), '/Game/ShiftStore/Maps/Normal', '-NullRHI', '-unattended',
            '-benchmark', '-fps=60', '-seconds=60', '-ShiftSelfTest', '-ShiftSelfTestExit',
            '-abslog=' + log.as_posix()]
    if force_failure:
        args.append('-ShiftSelfTestForceFailure')
    started = time.time()
    process = subprocess.Popen(args, cwd=EXE.parent)
    try:
        code = process.wait(timeout=90)
    except subprocess.TimeoutExpired:
        subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True)
        raise RuntimeError(f'{case}: self-test timed out')

    reports = [folder / 'gameplay_selftest.json' for folder in SAVED_DIRS]
    fresh = [p for p in reports if p.exists() and p.stat().st_mtime >= started]
    assert fresh, f'{case}: no fresh self-test report; inspect {log}'
    report_path = max(fresh, key=lambda p: p.stat().st_mtime)
    report = json.loads(report_path.read_text(encoding='utf-8-sig'))
    assert player_saves() == before, f'{case}: player save changed'
    assert report['test_save_slot'].startswith('ShiftStoreSelfTest_')
    assert report['checks']['formal_save_unchanged'] and report['checks']['test_slot_cleaned']
    if force_failure:
        assert code == 1 and report['passed'] is False, f'Expected failure exit 1, got {code}'
        assert report['checks']['forced_failure'] is False
        assert all(ok for name, ok in report['checks'].items() if name != 'forced_failure')
    else:
        assert code == 0 and report['passed'] is True, f'Self-test failed (exit {code}): {report}'
        assert len(report['checks']) >= 18 and all(report['checks'].values())
    report['process_exit_code'] = code
    report['external_player_save_hashes_unchanged'] = True
    target = OUT / f'gameplay_{case}.json'
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'case': case, 'exit_code': code, 'checks': len(report['checks']),
                      'passed': report['passed'], 'player_save_unchanged': True}))
    return target


if __name__ == '__main__':
    assert EXE.exists(), 'Build the Windows package first'
    passed = run_case()
    negative = run_case(force_failure=True)
    verification = ROOT / 'Verification'
    verification.mkdir(exist_ok=True)
    shutil.copy2(passed, verification / 'gameplay_selftest.json')
    shutil.copy2(negative, verification / 'gameplay_selftest_failure.json')
