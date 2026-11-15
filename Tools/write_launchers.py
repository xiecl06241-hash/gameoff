from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Builds/Windows'
OUT.mkdir(exist_ok=True,parents=True)
for label,state in [('启动正常场景','Normal'),('启动异常场景','Shift')]:
    (OUT/(label+'.cmd')).write_text('@echo off\ncd /d "%~dp0"\nstart "" "%~dp0ShiftStore.exe" /Game/ShiftStore/Maps/'+state+' -fullscreen -ForceRes -ResX=1920 -ResY=1080\n',encoding='ascii')
(OUT/'操作说明.txt').write_text('SHIFT / 24 — 首批环境漫游预览\n\n双击“启动正常场景.cmd”或“启动异常场景.cmd”。\n也可以直接双击 ShiftStore.exe，默认进入正常场景。\n\nWASD：移动\n鼠标：观察\nAlt+F4：退出\n\n门在本批预览中保持打开，方便检查营业厅、仓库、办公室和卫生间。\n这是独立运行包，试玩不需要 UE 编辑器或 Python。请保留整个 Windows 文件夹，不要只复制 exe。\n本批为环境资产与漫游预览；顾客、怪物、交互任务与七晚流程属于后续制作范围。\n',encoding='utf-8')
print('LAUNCHERS_READY',OUT)
