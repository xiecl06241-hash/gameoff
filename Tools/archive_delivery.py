"""Archive the standalone preview and editable project; omit local caches and logs."""
from pathlib import Path
import zipfile,json,hashlib
r=Path(__file__).resolve().parents[1]
out=r/'Builds'
out.mkdir(exist_ok=True)
def archive(path,files):
    count=0
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as z:
        for source,name in sorted(files,key=lambda x:x[1]):
            z.write(source,name);count+=1
    with zipfile.ZipFile(path) as z:
        bad=z.testzip()
        if bad: raise RuntimeError('Corrupt archive member '+bad)
    digest=hashlib.file_digest(path.open('rb'),'sha256').hexdigest()
    print(path.name,count,round(path.stat().st_size/1048576,1),'MiB')
    return {'file':path.name,'entries':count,'size_bytes':path.stat().st_size,'sha256':digest,'crc_check':'passed'}
runtime=[]
for p in (out/'Windows').rglob('*'):
    rel=p.relative_to(out/'Windows')
    if p.is_file() and 'Saved' not in rel.parts and p.suffix.lower()!='.pdb' and not p.name.startswith('Manifest_'):
        runtime.append((p,'Windows/'+rel.as_posix()))
required=['Windows/ShiftStore.exe','Windows/启动正常场景.cmd','Windows/启动异常场景.cmd','Windows/Engine/Binaries/Win64/UnrealGame.exe']
assert all(n in {name for _,name in runtime} for n in required)
results=[archive(out/'ShiftStore-Windows.zip',runtime)]
source=[]
for folder in ('Config','Content','Tools','SourceArt','Previews','Verification'):
    for p in (r/folder).rglob('*'):
        rel=p.relative_to(r)
        if p.is_file() and '__pycache__' not in rel.parts:
            source.append((p,'ShiftStore/'+rel.as_posix()))
for name in ('ShiftStore.uproject','README.md','VERIFICATION.md','.gitignore'):
    p=r/name
    if p.exists(): source.append((p,'ShiftStore/'+name))
results.append(archive(out/'ShiftStore-UE-Source.zip',source))
(out/'delivery_manifest.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
