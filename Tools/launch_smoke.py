"""Windowed standalone launch and orderly shutdown, without the CSV profiler."""
from pathlib import Path
import subprocess,json,time,os
r=Path(__file__).resolve().parents[1]
results=[]
mode=os.environ.get('SHIFT_LAUNCH_MODE','windowed')
for state in ('Normal','Shift'):
    log=r/'Saved'/f'{state}_{mode}_Launch.log'
    args=[str(r/'Builds/Windows/ShiftStore.exe'),'/Game/ShiftStore/Maps/'+state,'-'+mode,'-ForceRes','-ResX=1920','-ResY=1080','-unattended','-benchmark','-seconds=8','-fps=60','-abslog='+log.as_posix()]
    start=time.time()
    p=subprocess.Popen(args,cwd=r/'Builds/Windows')
    try: code=p.wait(timeout=60)
    except subprocess.TimeoutExpired:
        subprocess.run(['taskkill','/PID',str(p.pid),'/T','/F'],capture_output=True)
        code=-1
    text=log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
    results.append({'state':state,'resolution':'1920x1080','render_mode':mode,'csv_profiler':False,'exit_code':code,'elapsed_seconds':round(time.time()-start,2),'orderly_shutdown':'LogExit: Exiting.' in text,'missing_material_usage_warning':'Missing bUsedWithInstancedStaticMeshes' in text})
    print(json.dumps(results[-1]))
(r/'Saved'/('launch_fullscreen_checks.json' if mode=='fullscreen' else 'launch_checks.json')).write_text(json.dumps(results,indent=2),encoding='utf-8')
assert all(x['exit_code']==0 and x['orderly_shutdown'] and not x['missing_material_usage_warning'] for x in results)
