"""Launch the packaged program; capture real runtime images and frame profiles."""
from pathlib import Path
import subprocess
import os
import shutil
import json
import time
import csv
import statistics

ROOT=Path(__file__).resolve().parents[1]
QA=Path(os.environ['LOCALAPPDATA'])/'Temp/ShiftStoreQA'
QA.mkdir(parents=True,exist_ok=True)
state=os.environ.get('SHIFT_QA_STATE','Normal')
shot=QA/f'{state}_Runtime.png'
log=QA/f'{state}_Runtime.log'
exe=ROOT/'Builds/Windows/ShiftStore.exe'
args=[str(exe), '/Game/ShiftStore/Maps/'+state, '-windowed','-ResX=1920','-ResY=1080','-unattended','-NoSplash','-NoVSync',
      '-csvCaptureFrames=850','-ExitAfterCsvProfiling',
      '-abslog='+log.as_posix(),
      '-ExecCmds=r.DefaultFeature.AutoExposure 1,r.VSync 0,t.MaxFPS 0,sg.ViewDistanceQuality 2,sg.ShadowQuality 2,sg.GlobalIlluminationQuality 2,sg.ReflectionQuality 2,sg.PostProcessQuality 2,sg.TextureQuality 2,sg.EffectsQuality 2,sg.ShadingQuality 2,r.ScreenPercentage 100',
      '-csvExecCmds=300:HighResShot 1920x1080 filename='+shot.as_posix()]
started=time.time()
if os.environ.get('SHIFT_QA_RHI'):
    args.append('-'+os.environ['SHIFT_QA_RHI'])
if os.environ.get('SHIFT_QA_DISABLE_PLUGINS')=='1':
    args.append('-DisablePlugins=RenderDocPlugin,NFORDenoise,NNEDenoiser')
if os.environ.get('SHIFT_QA_NO_CSV_EXIT')=='1':
    command_index=next(i for i,arg in enumerate(args) if arg.startswith('-csvExecCmds='))
    args[command_index]='-csvExecCmds=300:HighResShot 1920x1080 filename='+shot.as_posix()+',800:quit'
capture_requested=os.environ.get('SHIFT_QA_NO_SCREENSHOT')!='1'
if not capture_requested:
    args=[arg for arg in args if not arg.startswith('-csvExecCmds=')]
if shot.exists():
    shot.unlink()
if os.environ.get('SHIFT_QA_OFFSCREEN')=='1':
    args.append('-RenderOffscreen')
if os.environ.get('SHIFT_QA_FULLSCREEN')=='1':
    args[args.index('-windowed')]='-fullscreen'
    args.append('-ForceRes')
proc=subprocess.Popen(args,cwd=ROOT/'Builds/Windows')
try:
    code=proc.wait(timeout=120)
except subprocess.TimeoutExpired:
    subprocess.run(['taskkill','/PID',str(proc.pid),'/T','/F'],capture_output=True)
    raise RuntimeError('Packaged preview did not finish the timed run')
out=ROOT/'Previews'; out.mkdir(exist_ok=True)
if shot.exists() and shot.stat().st_mtime>=started:
    shutil.copy2(shot,out/shot.name)
if log.exists():
    shutil.copy2(log,ROOT/'Saved'/log.name)
result={'state':state,'exit_code':code,'elapsed_seconds':time.time()-started,'screenshot_requested':capture_requested,'screenshot':shot.exists() and shot.stat().st_mtime>=started,'render_mode':'offscreen' if '-RenderOffscreen' in args else ('fullscreen' if '-fullscreen' in args else 'windowed'),'command':args}
profiles=list((ROOT/'Builds/Windows/Engine/Saved/Profiling/CSV').glob('*.csv'))
fresh=[p for p in profiles if p.stat().st_mtime>=started]
if fresh:
    profile=max(fresh,key=lambda p:p.stat().st_mtime)
    qa=ROOT/'Saved/QA'; qa.mkdir(exist_ok=True)
    shutil.copy2(profile,qa/f'{state}_FrameProfile.csv')
    csv.field_size_limit(10_000_000)
    with profile.open(encoding='utf-8-sig') as f:
        reader=csv.reader(f); headers=next(reader); rows=list(reader)
    def values(key):
        if key not in headers: return []
        i=headers.index(key)
        return [float(r[i]) for r in rows[400:800] if len(r)>i]
    frame=values('FrameTime')
    gpu=values('GPUTime')
    result['performance']={'resolution':'1920x1080','quality':'High (sg.*=2), 100% screen percentage','steady_frames':'400–799; startup and screenshot frames excluded','sample_count':len(frame),'mean_frame_ms':statistics.mean(frame),'average_fps':1000/statistics.mean(frame),'p95_frame_ms':sorted(frame)[int(len(frame)*.95)],'mean_gpu_ms':statistics.mean(gpu) if gpu else None,'peak_gpu_local_used_mb':max(values('GPUMem/LocalUsedMB'),default=None)}
(ROOT/'Saved'/f'{state}_runtime_result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False))
if code!=0 or (capture_requested and not result['screenshot']):
    raise RuntimeError('Runtime smoke failed; see Saved runtime logs')
