# SHIFT / 24 开发交接

更新于 2026-11-19。当前位于独立开发分支 `feat/shift-gameplay`（`main` 分支保持在 `2a01629`）。本文件记录了紧扣 **SHIFT** 主题的“去公式化 & 去无聊化”全套玩法升级、商品放回与散落清理规则、怪物追逐寻路修复及全套验证结果。

## 工程与启动

- 仓库为 `https://github.com/xiecl06241-hash/gameoff`，当前功能分支为 `feat/shift-gameplay`，主分支为 `main`。
- 本机工程位于 `D:\Game Jam\Game Off 2026\ShiftStore`，入口为 `ShiftStore.uproject`。
- 使用 UE 5.8.2，本机引擎位于 `D:\UE\UE_5.8`。编译工具为 Visual Studio 2022、MSVC 14.44 和 Windows SDK。
- 本机试玩入口为 `Builds/Windows/ShiftStore.exe`，同目录的 `启动正常场景.cmd` 以 1080p 全屏启动。完整保留 `Windows` 文件夹才能运行。
- 选择语言后，Space／E／Enter 从第一晚开始，C 继续存档。WASD 移动，**Left Shift／Right Shift 冲刺跑**，E 交互或长按做任务，Tab 切换目标，Esc 暂停。暂停菜单可调整鼠标灵敏度。

## 当前玩法（SHIFT 主题深化 & 去无聊化版）

Windows 第一人称 3D 三晚原型，支持英语、日语、繁体中文和简体中文。

1. **左下角常驻《夜班异常守则》+ 3 种肉眼视觉破绽推理（无准星剧透）**：
   - 求助顾客的准星提示仅显示其表面索要的商品，不再直白剧透答案。
   - 玩家在递交商品前必须用肉眼观察顾客体态，并核对屏幕左下角常驻的 3 条《夜班异常守则》：
     - **守则 ①【歪头死盯空墙（`HeadTilt`）】**（脖子歪斜 26° 且频繁扭头死盯空墙）→ 别给原商品，递交**【番茄浓汤罐头（`Can`）】**；
     - **守则 ②【红眼微光+顶灯闪烁（`RedGlare`）】**（眼部渗出脉冲红光且头顶灯光周期性闪烁）→ 别给原商品，递交**【每日全脂鲜奶（`Milk`）】**；
     - **守则 ③【脚不沾地悬空漂浮（`Floating`）】**（脚底离开地面约 22 cm 上下悬浮晃动）→ 别给原商品，递交**【晨光切片面包（`Bread`）】**。
   - **识破平静驱离**：递交守则对应的克制商品可在显形前直接平息并送走怪客，不触发全店熄灯 Shift，并奖励 **+$10.00 奖金与 +8 清醒度**。
   - **误交禁忌暴走**：若未看破伪装而把表面索要的商品递给怪客（或贴脸 `< 155 cm` 直视过久、或超耐性时限），怪客当场暴走触发惊吓音效、红屏脉冲与全面 Shift。
2. **第一晚即刻遭遇怪客 + 每晚动态随机洗牌（防背板）**：
   - 正常游玩下，**第一晚（Night 1）第 3 位顾客即登场伪装怪客**，首夜即有观察推理与生死抉择。
   - 每次开局或重开夜晚（`BuildNightCustomerRoster`），系统自动随机打乱后续顾客的登场顺序，并为每位伪装怪客随机抽取 3 种视觉破绽（`EDisguiseTell`）之一及对应的守则克制商品。
3. **`Left Shift` 冲刺跑 & 任务读条全面提速 & 距离动态心跳声**：
   - 基础步行速度提升至 `285 cm/s`，按住 `Left Shift` / `Right Shift` 冲刺速度达 **`435 cm/s`**（视野 FOV 动态拉宽至 `93°`），配备 `0~100` 体力条（左上角状态栏下方实时显示）。
   - 日常任务读条全面提速：补货 `0.8s`、整理/倒垃圾/拖地/复位异变 `1.0s`、清点 `1.2s`、煮关东煮从 `6.0s` 压缩至 **`2.2s`**。
   - 当怪物显形追逐（或伪装怪客近身潜伏）时，根据距离（`< 950 cm`）实时触发**双搏低频心跳声（Lub-Dub Heartbeat）**与屏幕四边暗红搏动边框，距离越近心跳越急促。
4. **日常局部异变（Micro-Shift）与顾客并行**：
   - 顾客在店期间会周期性发生局部异变（`FixAnomaly`：迎宾牌篡改、二号货架暗移、冷柜异响震落商品），不阻塞后续顾客进店，逾期未长按 `E` 复位会扣清醒度并加速伪装怪客暴走。
5. **全面 SHIFT 货架商品真错位 + 后勤走廊应急电闸（`BreakerBox`）**：
   - 全面 Shift 期间冷柜与干货架商品类别发生互换（`ShiftData::ShiftedSku`），角落需求物发出微弱呼吸光与 3D 方位脉冲音。
   - 后勤走廊 `(900, 1125, 130)` 常驻应急电闸，Shift 期间按 `E` 拉闸可强制恢复全店正常照明与货架位置 **10 秒**，但电弧噪音会将怪物吸引至后勤走廊。
6. **商品放回与夜末散落清理**：
   - 手持商品对准任意货架、冷柜按 `E` 可放回；满手时防误拿；夜末需将散落商品全部放回货架方可天亮。

## 验证结果与范围

| 项目 | 本次结果 | 证据 |
| --- | --- | --- |
| 编辑器及 Windows Development 构建 | 成功 | `Saved/QA/EditorBuild.log`、`WindowsPackage.log` |
| 核心玩法回归（含 SHIFT 主题与守则映射断言） | **64 项通过**，正式存档哈希一致 | `Verification/gameplay_selftest.json` |
| 故意失败检查 | **65 项**中只有强制失败项失败，退出码 1 | `Verification/gameplay_selftest_failure.json` |
| 简体中文 720p 试玩反馈专项 | 84 项通过，其中挡路相关 40 项 | `Verification/feedback_zh-Hans_1280.json` |
| 三晚自动流程与性能走测 | 13 项通过，走到第三晚结局，平均 **59.22 FPS** | `Verification/playthrough_1920.json` |
| 四语言词条与字体覆盖检查 | 各 **207 条**，简繁日字形覆盖通过 | `Verification/localization_catalogues.json` |
| 静态仓库检查 | 33 个 Python 脚本、40 个 JSON、476 个 LFS 资产通过 | `.github/scripts/check_repository.py` |

## 构建与复现

```bash
cd '/d/Game Jam/Game Off 2026/ShiftStore'
export PYTHONUTF8=1 PYTHONIOENCODING=utf-8
export UE_ROOT='D:/UE/UE_5.8'
python Tools/author_localization.py
python Tools/check_localization.py
python .github/scripts/check_repository.py --require-lfs
python .github/scripts/build_windows.py
python Tools/check_gameplay.py
python Tools/check_feedback.py --languages zh-Hans
python Tools/check_feedback.py --playthrough
```
