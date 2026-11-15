from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1]
args=[str(r/'Builds/Windows/ShiftStore.exe'),'/Game/ShiftStore/Maps/Normal','-NullRHI','-unattended','-ExecCmds=quit','-abslog='+str(r/'Saved/NullRHIExit.log')]
p=subprocess.run(args,timeout=60,cwd=r/'Builds/Windows')
(r/'Saved/exit_diagnostic.json').write_text(json.dumps({'exit_code':p.returncode,'mode':'NullRHI; quit console command'},indent=2))
print(p.returncode)
