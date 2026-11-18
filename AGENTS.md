# SHIFT / 24 开发交接

更新于 2026-11-18。本文件包含商品放回与散落清理规则、怪物追逐寻路修复、顾客挡路复测及三晚流程验证记录。此前提交基线为 `68d2dfe`。

## 工程与启动

- 仓库为 `https://github.com/xiecl06241-hash/gameoff`，工作分支为 `main`。
- 本机工程位于 `D:\Game Jam\Game Off 2026\ShiftStore`，入口为 `ShiftStore.uproject`。
- 使用 UE 5.8.2，本机引擎位于 `D:\UE\UE_5.8`。编译工具为 Visual Studio 2022、MSVC 14.44 和 Windows SDK。
- 本机试玩入口为 `Builds/Windows/ShiftStore.exe`，同目录的 `启动正常场景.cmd` 以 1080p 全屏启动。完整保留 `Windows` 文件夹才能运行。
- 选择语言后，Space／E／Enter 从第一晚开始，C 继续存档。WASD 移动，E 交互或长按做任务，Tab 切换目标，Esc 暂停。暂停菜单可调整鼠标灵敏度。
- 当前公开下载为 `v0.2`，对应提交 `e50bec2`。本机包包含此后的关东煮位置调整、挡路复测与本次商品放回／怪物寻路修复。`Builds/` 为本地交付目录，由 Git 忽略，源码提交和 Release 附件有各自的更新时间。

## 当前玩法

Windows 第一人称 3D 三晚原型，支持英语、日语、繁体中文和简体中文。七晚为后续内容目标。

1. 完成开店任务后顾客进店，店内同时接待一位顾客。
2. 顾客可能自行购物或请求帮助，正常顾客结账后实际走出店门。
3. 接待间派发整理货架或煮关东煮，累计三笔正常消费后安排补货。最后一位顾客离店后进入打扫。
4. 异常顾客显形时自动触发 Shift。玩家找到需求商品并交到收银台，使怪物离开、店铺恢复。
5. 手持商品对准任意货架、冷柜或展示位按 E 可放回（不扣钱）；手里已有物品时不能直接拿取新物品，需先放回或按 G 放下。关东煮热汤杯可放回关东煮台。
6. 夜末打扫除清理地面污渍外，还需将散落在地面或手中的普通商品全部放回货架，才会进入天亮并保存进度。
7. 清醒度归零或亏损过多会失败，支持当晚重开。

第一晚为三位正常顾客，第二晚四位顾客中有一位异常顾客，第三晚五位顾客中有两位异常顾客。任务时限分别为 240／180／120 秒。具体配置在 `Source/ShiftStore/ShiftTypes.cpp`。

## 本次完成的工作

### 商品放回与夜末散落清理

- 手持普通商品（非怪物需求的红光商品、非关东煮杯）对准任意货架、冷柜或展示商品按 E 可直接放回并销毁手持实例，不计亏损。手持关东煮热汤杯对准关东煮台按 E 也可放回。
- 手里持有任意物品时，对货架商品、地上商品、补货纸箱源、关东煮台或拖把交互会提示双手已满，不再自动丢弃手中物品造成满地狼藉；玩家需主动放回货架或按 G 放下。
- 夜末 `Cleanup` 阶段新增散落商品检查（`CountLooseMerchandise`）：拖地任务全部完成后，若地面或手中仍有未放回的普通商品，右上角目标面板与横幅会提示剩余件数，全部放回货架后才触发天亮存档。每晚开局清理上一晚残留的动态商品 Actor。
- 四套语言包（`en`、`ja`、`zh-Hant`、`zh-Hans`）已同步新增 5 条相关提示文本（共 180 条）。源码见 `ShiftPlayerCharacter.cpp`、`ShiftStoreGameMode.cpp`、`ShiftHUD.cpp`、`ShiftItem.h` 及 `Tools/author_localization.py`。

### 怪物追逐寻路与防卡死修复

- 旧版怪物直线追逐仅用离地 40 cm、半径 26 cm 的球体探路，且遇阻只做固定左右 90° 偏移，容易被收银台旁的垃圾桶（高约 65 cm）和灭火器（高约 52 cm）卡住。
- 新版 `AShiftCustomer::MonsterMoveToward` 改用与角色等高的完整胶囊体（`MonsterPathClear`）检测前方路径：
  1. 直线无阻挡时直接追击玩家；
  2. 直线受阻时接入店内通道导航网（`ShiftGuidance::FindRoute`），最多每 0.5 秒重算一次路点；
  3. 路点不可用时按 35°／70°／105°／140° 扇形探测可通行方向绕障；
  4. 新增卡住自救检测（0.45 秒位移不足则切换绕行侧并强制重算路线）。场景内的垃圾桶和灭火器位置保持不变。
- 第二晚性能走测（`RunBenchmarkRoute`）专门测量 Shift 过渡与移动帧耗时，现会在怪物贴近测试角色时短暂眩晕怪物，避免追逐增强后打断固定采样路线。源码见 `ShiftCustomer.cpp`、`.h` 与 `ShiftFeedbackTest.cpp`。

### 顾客挡路专项复测与三晚自动流程

- 挡路用例按进店、购物、去收银台、离店分别设置起点和目的地，确认顾客正在行走后再将玩家放到前方可通行路段，检查停止、目标保持、计时冻结、让路台词、防重复及 0.2 秒恢复延迟。
- 显式测试参数 `-ShiftPlaythroughTest` 由 `ShiftStoreGameMode::BeginPlay` 创建测试 Actor，使用独立的 `ShiftStorePlaythrough-*` 用户目录跑完三晚流程并核对正式存档哈希。
- `README.md`、`Docs/ONBOARDING.md`、`Tools/write_launchers.py` 及 `Builds/Windows/操作说明.txt` 已统一说明右上角任务区、黄色箭头和到达高亮。

## 验证结果与范围

| 项目 | 本次结果 | 证据 |
| --- | --- | --- |
| 编辑器及 Windows Development 构建 | 成功 | `Saved/QA/EditorBuild.log`、`WindowsPackage.log` |
| 核心玩法回归 | 54 项通过，正式存档哈希一致 | `Verification/gameplay_selftest.json` |
| 故意失败检查 | 55 项中只有强制失败项失败，退出码 1 | `Verification/gameplay_selftest_failure.json` |
| 简体中文 720p 试玩反馈专项 | 84 项通过，其中挡路相关 40 项 | `Verification/feedback_zh-Hans_1280.json` |
| 三晚自动流程 | 13 项通过，走到第三晚结局，退出码 0 | `Verification/playthrough_1920.json` |
| 四语言词条与字体覆盖检查 | 各 180 条，简繁日字形覆盖通过 | `Verification/localization_catalogues.json` |
| 静态仓库检查 | 33 个 Python 脚本、37 个 JSON、476 个 LFS 资产通过 | `.github/scripts/check_repository.py` |

性能样本来自 RTX 5060 Laptop GPU，1920×1080 全屏 High，渲染比例 100%。第二晚收银区至仓库路线共 741 帧，平均 **60.00 FPS**，P95 **17.11 ms**，显形帧 **17.12 ms**，最慢帧 **25.34 ms**（见 `Verification/playthrough_1920.json`）。

## 构建与复现

在 Git Bash 中进入仓库目录。源码检出先完成 `git lfs pull`。构建时关闭占用本工程模块的 UE 编辑器。

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

## 后续优先项

1. 实机试玩验证本次新增的两项改动：检查各类货架／冷柜按 E 放回商品与夜末散落清理的交互手感，以及收银台旁垃圾桶／灭火器附近拉扯怪物的追逐体验。
2. 安排没看过策划的玩家从第一晚开始盲测，完成完整键鼠三晚人工通关验收。
3. 补充环境音效与怪物显形视觉表现，视剩余时间决定是否整理并发布新版下载包（如 `v0.3`）。
4. 扩展反馈专项到另外三种语言和 1080p，并在一台没有 UE／Python 的干净 Windows 电脑上验证完整下载包。

## 交接时的工作区状态

`Config/DefaultEngine.ini` 在此前工作前已有 15 行 `AndroidFileServer` 本地改动，留在工作区未提交。其余改动为本次商品放回、夜末清理、怪物寻路修复、四语言词条及最新生成的验证证据。
