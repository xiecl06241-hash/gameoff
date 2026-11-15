from pathlib import Path
import subprocess
import json
import os
ROOT=Path(__file__).resolve().parents[1]
state=os.environ.get('SHIFT_QA_STATE','Normal')
args=[str(ROOT/'Builds/Windows/ShiftStore.exe'),'/Game/ShiftStore/Maps/'+state,'-windowed','-ResX=1280','-ResY=720','-NoSplash','-abslog='+(ROOT/'Saved'/f'{state}_GUI.log').as_posix()]
p=subprocess.Popen(args,cwd=ROOT/'Builds/Windows')
print('GUI_STARTED',p.pid,flush=True)
code=p.wait(timeout=300)
(ROOT/'Saved'/f'{state}_GUI_result.json').write_text(json.dumps({'state':state,'exit_code':code},indent=2),encoding='utf-8')
print('GUI_EXIT',code,flush=True)
