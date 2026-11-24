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
    'Left Shift / Right Shift：冲刺跑（消耗体力）\n'
    '鼠标（或方向键）：观察视角\n'
    'E / 鼠标左键：交互 / 拾取商品或工具 / 长按执行店员任务 / 收银结账 / 供奉神龛 / 被抓时挣脱\n'
    'Tab：从口袋掏出／放回《夜班守则纸条》（遮挡大部分视野，请快速核对）\n'
    'R：面对违反守则的异常顾客主动拒单驱离（切勿误逐正常顾客，淘汰页按 R 重玩本夜）\n'
    'Q：窥视这家店残留的旧影（便利店 ↔ 原址中式老宅灵堂）；异界会消耗清醒度且久看伪装者会触发反噬。怪物追逐时按 Q 可闪回现实 2.5 秒并短暂眩晕怪物。\n'
    'F / 鼠标右键：投掷手中物品（砸中怪物可使其眩晕，普通商品损耗计入店铺成本亏损）\n'
    'G：放下手中物品\n'
    'V：在多个日常任务之间切换当前指引目标\n'
    'Space / Enter：跳过标题、每晚交班说明、天亮结算页\n'
    'Esc：暂停／继续；菜单可用鼠标点击，或上下方向键／W、S 选择并按 Enter／Space 确认\n'
    'Esc → 鼠标灵敏度：拖动滑条或用左右方向键调整，范围 0.10–3.00 倍，默认 1.00 倍；自动保存，可恢复默认。\n'
    '暂停时任务、清醒度扣除和怪物行动停止。返回主菜单会丢失本晚未保存的进度，保留最近天亮时的存档。\n'
    'Alt+F4：退出游戏\n\n'
    '【核心规则】\n'
    '1. 每晚开局会自动从口袋掏出《夜班守则纸条》（第 1/2/3 晚渐进生效 1/2/3 条守则），随时可按 Tab 再次掏出核对。\n'
    '2. 看右上角的当前目标和操作步骤，沿黄色箭头前往工作点，长按 E 完成理货、清点、补货或煮关东煮。逾期会持续扣清醒度。\n'
    '3. 顾客进店后仔细观察其体态与异象：正常顾客帮其取货并在收银台按 E 结账；若发现对方违反纸条守则，立刻对准其按 R 拒单驱离（识破奖励奖金与清醒度；误拒正常顾客扣 15 清醒度）。\n'
    '4. 若未识破或误递商品，怪物会显形并将全店 Shift 为阴森中式老宅灵堂；必须找到现形后在店里刷出的红光遗失物并供奉到神龛（收银台原位）平息怨念，提前从货架囤的普通商品无效。\n'
    '5. 服务完当晚全部顾客后，去仓库拿拖把（Mop）清理地面污渍（拖地实时溶解），把散落商品放回货架，全部收好才会天亮并自动存档。\n',
    encoding='utf-8',
)
print('LAUNCHERS_READY', OUT)
