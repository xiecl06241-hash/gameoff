from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Builds/Windows'
OUT.mkdir(exist_ok=True, parents=True)

for label, state in [('启动正常场景', 'Normal'), ('启动异常场景', 'Shift')]:
    (OUT / (label + '.cmd')).write_text(
        '@echo off\ncd /d "%~dp0"\nstart "" "%~dp0ShiftStore.exe" /Game/ShiftStore/Maps/' + state + ' -fullscreen -ForceRes -ResX=1920 -ResY=1080\n',
        encoding='ascii',
    )

(OUT / '操作说明.txt').write_text(
    'SHIFT / 24 — 便利店夜班恐怖游戏 (GitHub Game Off 2026)\n\n'
    '【启动方式】\n'
    '双击“启动正常场景.cmd”以 1920x1080 全屏启动完整三晚夜班游戏，或直接双击 ShiftStore.exe。\n'
    '（“启动异常场景.cmd”用于直接预览 Shift 异常灯光与布局。）\n\n'
    '【操作按键】\n'
    'WASD：移动\n'
    '鼠标（或方向键）：观察视角\n'
    'E / 鼠标左键：交互 / 拾取商品或工具 / 长按执行店员任务 / 收银结账 / 被抓时挣脱\n'
    'F / 鼠标右键：投掷手中物品（砸中怪物可使其眩晕，损耗计入店铺成本亏损）\n'
    'G：放下手中物品\n'
    'Space / Enter：跳过标题、每晚交班说明、天亮结算页\n'
    'R：淘汰后回溯重玩当前夜晚\n'
    'Alt+F4：退出游戏\n\n'
    '【核心规则】\n'
    '1. 按时完成右上角清单中的补货（去仓库拿纸箱 Stock Carton）、清点、整理、倒垃圾任务；逾期会持续扣除清醒度。\n'
    '2. 当前任务全部完成后顾客进店：帮求助顾客拿取指定商品（按 E 递交），并在收银台按 E 结账。\n'
    '3. 若顾客显形成怪物，便利店会自动 Shift 进入异常状态；找到怪物索要的红光商品并送到收银台即可驱离怪物、恢复正常。\n'
    '4. 服务完当晚全部顾客后，去后勤走廊拿拖把（Mop）清理地面污渍即可迎来天亮并自动存档。\n',
    encoding='utf-8',
)
print('LAUNCHERS_READY', OUT)
