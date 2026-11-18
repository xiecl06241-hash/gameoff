# SHIFT / 24 开发交接

更新于 2026-11-18。当前位于独立开发分支 `feat/shift-gameplay`（`main` 分支保持在 `2a01629`）。本文件记录了紧扣 **SHIFT** 主题的五大去公式化玩法改造、商品放回与散落清理规则、怪物追逐寻路修复及全套验证结果。

## 工程与启动

- 仓库为 `https://github.com/xiecl06241-hash/gameoff`，当前功能分支为 `feat/shift-gameplay`，主分支为 `main`。
- 本机工程位于 `D:\Game Jam\Game Off 2026\ShiftStore`，入口为 `ShiftStore.uproject`。
- 使用 UE 5.8.2，本机引擎位于 `D:\UE\UE_5.8`。编译工具为 Visual Studio 2022、MSVC 14.44 和 Windows SDK。
- 本机试玩入口为 `Builds/Windows/ShiftStore.exe`，同目录的 `启动正常场景.cmd` 以 1080p 全屏启动。完整保留 `Windows` 文件夹才能运行。
- 选择语言后，Space／E／Enter 从第一晚开始，C 继续存档。WASD 移动，E 交互或长按做任务，Tab 切换目标，Esc 暂停。暂停菜单可调整鼠标灵敏度。

## 当前玩法（SHIFT 主题深化版）

Windows 第一人称 3D 三晚原型，支持英语、日语、繁体中文和简体中文。

1. **并行夜班值守与日常局部异变（Micro-Shift）**：
   - 完成开店任务后顾客进店；顾客在店浏览或等待期间，店内会周期性发生**局部异变（`FixAnomaly`）**：迎宾牌被篡改为异常告示、二号货架偷偷横移、冷柜异响并震落商品。
   - 局部异变不阻塞下一位顾客进店，迫使玩家在“盯紧可疑顾客”和“长按 E 复位店铺异变”之间做实时取舍；异变逾期未处理会扣除清醒度并加速伪装顾客暴走。
2. **伪装破绽观察与“识破平静驱离”**：
   - 伪装成顾客的怪物不再固定 4.2 秒机械显形（正常游玩下潜伏 12~15.5 秒）。求助期间会露出破绽：头顶灯光偶发电磁微闪、视线偶发抽动盯向空墙、微弱红眼闪现，且求助提示夹带暗示其真正执念商品（`MonsterSku`）的低语。
   - **识破驱离**：若玩家看穿伪装，在显形前从货架拿来其真正执念的 `MonsterSku` 递交给它，怪客会低语离店，不触发全店 Shift，并奖励 **+$10.00 奖金与 +8 清醒度**。
   - **触发禁忌**：若玩家盲目递交其表面索要的 `WantedSku`，或贴脸（`< 155 cm`）直视超过 2.2 秒，怪物会当场暴走显形并触发全面 Shift。
3. **全面 SHIFT：空间/货架商品真错位与声光寻物**：
   - 怪物显形时店铺进入 Shift 状态：中排货架横移、主照明熄灭。
   - **货架商品错位互换（`ShiftData::ShiftedSku`）**：Shift 期间冷柜与干货架的商品类别发生对调（如矿泉水 ↔ 番茄汤罐头、鲜奶 ↔ 酱油、汽水 ↔ 面包、泡面 ↔ 便当），玩家既可凭店铺记忆直接从错位后的货架/冷柜取得怪物所需商品，也可跟随微弱呼吸红光（半径 75 cm）与 3D 方位脉冲音寻找散落物交到收银台。
4. **后勤走廊应急电闸（`BreakerBox`）主动博弈**：
   - 全面 Shift 熄灯期间，玩家可前往后勤走廊 `(900, 1125, 130)` 按 `E` 拉下应急电闸，强制恢复正常照明与货架布局 **10 秒**（冷却 20 秒），但电闸噪音会短暂震慑怪物并**将怪物吸引到后勤走廊**。
5. **商品放回与夜末散落清理**：
   - 手持普通商品对准任意货架、冷柜或展示位按 E 可放回（不扣钱）；手里已有物品时不能直接拿取新物品，需先放回或按 G 放下。
   - 夜末打扫除清理地面污渍外，还需将散落在地面或手中的普通商品全部放回货架，才会进入天亮并保存进度。

## 验证结果与范围

| 项目 | 本次结果 | 证据 |
| --- | --- | --- |
| 编辑器及 Windows Development 构建 | 成功 | `Saved/QA/EditorBuild.log`、`WindowsPackage.log` |
| 核心玩法回归（含 4 项新 SHIFT 机制断言） | **63 项通过**，正式存档哈希一致 | `Verification/gameplay_selftest.json` |
| 故意失败检查 | **64 项**中只有强制失败项失败，退出码 1 | `Verification/gameplay_selftest_failure.json` |
| 简体中文 720p 试玩反馈专项 | 84 项通过，其中挡路相关 40 项 | `Verification/feedback_zh-Hans_1280.json` |
| 三晚自动流程与性能走测 | 13 项通过，走到第三晚结局，平均 **60.00 FPS** | `Verification/playthrough_1920.json` |
| 四语言词条与字体覆盖检查 | 各 **200 条**，简繁日字形覆盖通过 | `Verification/localization_catalogues.json` |
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
