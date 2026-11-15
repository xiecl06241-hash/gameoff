"""CI packaging entry point; requires Windows, UE 5.8 and VS 2022 C++ tools."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-only', action='store_true', help='Validate tools without building')
    args = parser.parse_args()
    if sys.platform != 'win32':
        raise RuntimeError('UE Windows packaging must run on a Windows runner')
    engine = Path(os.environ.get('UE_ROOT') or r'D:\UE\UE_5.8')
    ubt = engine / 'Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe'
    uat = engine / 'Engine/Build/BatchFiles/RunUAT.bat'
    version = engine / 'Engine/Build/Build.version'
    for tool in (ubt, uat, version):
        if not tool.is_file():
            raise FileNotFoundError(f'{tool}: configure the UE_ROOT repository variable')
    build_version = json.loads(version.read_text(encoding='utf-8-sig'))
    if (build_version['MajorVersion'], build_version['MinorVersion']) != (5, 8):
        raise RuntimeError('This project requires UE 5.8 (tested on 5.8.2)')
    print(f'Using Unreal Engine at {engine}', flush=True)
    if args.check_only:
        return
    logs = ROOT / 'Saved/QA'
    logs.mkdir(parents=True, exist_ok=True)

    def run(name, command):
        log = logs / (name + '.log')
        print(f'{name}: {log}', flush=True)
        with log.open('w', encoding='utf-8') as output:
            result = subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text(encoding='utf-8', errors='replace')[-16000:])
            raise subprocess.CalledProcessError(result.returncode, command)

    project = '-Project=' + str(ROOT / 'ShiftStore.uproject')
    run('EditorBuild', [str(ubt), 'ShiftStoreEditor', 'Win64', 'Development', project,
                        '-WaitMutex', '-NoHotReloadFromIDE'])
    run('WindowsPackage', [os.environ['COMSPEC'], '/d', '/c', str(uat), 'BuildCookRun',
                          project, '-noP4', '-platform=Win64', '-clientconfig=Development',
                          '-build', '-cook', '-stage', '-pak', '-iostore', '-archive',
                          '-archivedirectory=' + str(ROOT / 'Builds'),
                          '-AdditionalCookerOptions=-SkipZenStore', '-CrashReporter',
                          '-utf8output', '-unattended', '-prereqs'])
    run('Launchers', [sys.executable, str(ROOT / 'Tools/write_launchers.py')])
    exe = ROOT / 'Builds/Windows/ShiftStore.exe'
    if not exe.is_file():
        raise FileNotFoundError(f'Packaging did not produce {exe}')


if __name__ == '__main__':
    main()
