from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Builds/Windows'
OUT.mkdir(exist_ok=True, parents=True)
licenses = OUT / 'Licenses'
licenses.mkdir(exist_ok=True)
shutil.copy2(ROOT / 'SourceArt/Fonts/LICENSE', licenses / 'Noto-SIL-OFL-1.1.txt')
shutil.copy2(ROOT / 'THIRD_PARTY_NOTICES.md', licenses / 'THIRD_PARTY_NOTICES.md')

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
    '【语言】\n'
    '进入游戏后先选择语言：1 English / 2 日本語 / 3 繁體中文 / 4 简体中文。\n'
    '鼠标点击，或方向键、数字 1/2/3/4 选择后按 Enter 确认。\n'
    '会记住上次选择；标题页按 L 可重新选择语言，语言设置不影响游戏存档。\n\n'
    '【新游戏与继续】\n'
    '标题页 Space / E：新游戏，从第一晚开始。C：继续已有存档。\n'
    '每晚说明页不扣任务时间，进入实际操作后才开始计时。\n'
    '当前三晚每项任务分别为 240 / 180 / 120 秒，先熟悉店铺，再逐步收紧。\n\n'
    '【操作按键】\n'
    'WASD：移动\n'
    '鼠标（或方向键）：观察视角\n'
    'E / 鼠标左键：交互 / 拾取商品或工具 / 长按执行店员任务 / 收银结账 / 被抓时挣脱\n'
    'F / 鼠标右键：投掷手中物品（砸中怪物可使其眩晕，损耗计入店铺成本亏损）\n'
    'G：放下手中物品\n'
    'Space / Enter：跳过标题、每晚交班说明、天亮结算页\n'
    'R：淘汰后回溯重玩当前夜晚\n'
    'Tab：切换当前指引的日常任务；跟随黄色通道和距离标记。\n'
    'Alt+F4：退出游戏\n\n'
    '【核心规则】\n'
    '1. 跟随左下角步骤和黄色通道标记完成任务。补货：先去仓库按 E 领取补货纸箱，再按更新的标记返回指定货架长按 E。清点、整理、倒垃圾也在标记处长按 E；逾期会持续扣清醒度。\n'
    '2. 当前任务全部完成后顾客进店：帮求助顾客拿取指定商品（按 E 递交），并在收银台按 E 结账。\n'
    '   顾客离店后通常只安排一项整理或煮关东煮任务；累计三笔正常消费才安排一次补货。最后一位顾客离店后直接打扫。\n'
    '3. 若顾客显形成怪物，便利店会自动 Shift 进入异常状态；找到怪物索要的红光商品并送到收银台即可驱离怪物、恢复正常。\n'
    '4. 服务完当晚全部顾客后，跟随标记去仓库拿拖把（Mop），清理地面污渍即可迎来天亮并自动存档。\n',
    encoding='utf-8',
)
print('LAUNCHERS_READY', OUT)
