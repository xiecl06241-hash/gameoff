# SHIFT / 24 开发交接

更新于 2026-11-18。当前交接版本为 [SHIFT / 24 v0.3](https://github.com/xiecl06241-hash/gameoff/releases/tag/v0.3)。发布审查基线为 `2a01629`。查本版相对上一版的完整改动，运行 `git diff v0.2..v0.3`。

## 工程与启动

- 仓库为 `https://github.com/xiecl06241-hash/gameoff`，工作分支为 `main`。
- 本机工程位于 `D:\Game Jam\Game Off 2026\ShiftStore`，入口为 `ShiftStore.uproject`。
- 使用 UE 5.8.2，本机引擎位于 `D:\UE\UE_5.8`。编译工具为 Visual Studio 2022、MSVC 14.44 和 Windows SDK。
- 本机试玩入口为 `Builds/Windows/ShiftStore.exe`，同目录的 `启动正常场景.cmd` 以 1080p 全屏启动。完整保留 `Windows` 文件夹才能运行。
- 选择语言后，Space／E／Enter 从第一晚开始，C 继续存档。WASD 移动，E 交互或长按做任务，Tab 切换目标，Esc 暂停。暂停菜单可调整鼠标灵敏度。
- 下载 Release 的 `ShiftStore-v0.3-Windows-x64.zip`，完整解压后运行 `Windows/ShiftStore.exe`。同页 `SHA256SUMS.txt` 可用 Git Bash 的 `sha256sum -c SHA256SUMS.txt` 校验。复现源码版本使用 `git switch --detach v0.3`，再执行 `git lfs pull`。

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

## v0.3 发布审查

- 普通商品成为打烊条件后，原先固定在镜头前方 65 厘米放下物品的做法可能把商品送到薄墙另一侧。放下与投掷商品现共用一次按模型尺寸进行的碰撞检测。新增回归在薄货架前放下普通商品，检查物品仍在可拾取侧、拖地结束后地面或手中的商品会阻止天亮，以及对准货架按 E 归还最后一件后可以下班。
- 怪物绕行按一段时间内的实际位移判断卡住，走完仍受阻的路径后可重新规划。收银区拖住怪物的人工走位体验列入下一轮实测。

## 发布验证

Windows Development 包来自独立干净检出。以下测试使用最终 ZIP 解压出的程序。报告中特定机器的绝对路径只用于定位本次测试文件。

| 测试 | 结果 | 证据 |
| --- | --- | --- |
| 核心玩法与存档 | 58 项通过，正式存档一致 | `Verification/gameplay_selftest.json` |
| 测试失败能否正确上报 | 59 项中仅强制失败项失败，退出码 1 | `Verification/gameplay_selftest_failure.json` |
| 四语言 720p 试玩反馈 | 每种语言 84 项，含指引、挡路和抓取 | `Verification/feedback_*_1280.json` |
| 四语言 720p 暂停及灵敏度 | 暂停每种 19 项，灵敏度每种 20 项，含重启读取 | `Verification/pause_menu_*_1280.json`、`Verification/sensitivity_*_1280.json` |
| 三晚自动流程 | 13 项通过，完成第三晚并写入隔离存档 | `Verification/playthrough_1920.json` |
| 文本与字体 | 四套语言各 180 条，简繁日字形覆盖通过 | `Verification/localization_catalogues.json` |

1920×1080 全屏 High、100% 渲染比例，RTX 5060 Laptop GPU。第二晚显形走测共 741 帧，平均 60.00 FPS，P95 17.13 ms，显形帧 16.99 ms，最慢帧 21.58 ms。该固定路线在怪物贴近时会短暂眩晕怪物，不能代表持续追逐的最低帧率。三晚测试脚本代办部分任务、递交和结账，人工操作体验需另验。

## 构建与复现

用 Git Bash 从仓库根目录运行。构建脚本需要 UE 5.8.2、VS 2022 C++ 工具及 Windows SDK。语言包字体检查需要 Python 的 fontTools。

```bash
export PYTHONUTF8=1 PYTHONIOENCODING=utf-8
export UE_ROOT='D:/UE/UE_5.8'
git lfs pull
python .github/scripts/check_repository.py --require-lfs
python Tools/check_localization.py
python .github/scripts/build_windows.py
python Tools/check_gameplay.py
python Tools/check_feedback.py --playthrough
python Tools/check_feedback.py
python Tools/check_pause_menu.py
python Tools/check_sensitivity.py
```

发布使用干净检出，避免把开发机设置带入安装目录。压缩包保留完整 Windows 目录、运行库及许可，排除 Saved、日志和调试符号。上传后核对 GitHub 附件大小及 SHA-256。仓库中保留各版标签和下载说明。

## 下一阶段验收目标

1. **首次玩家通关。** 请一名没读过策划的玩家从第一晚开始，看能否独立找到补货点、弄懂关东煮交互、放下拖把并归还最后一件商品。记录首次卡住的位置和耗时，再实际用键鼠走完三晚，覆盖死亡重开和跨晚续档。
2. **追逐体验与性能。** 在第二晚按相同路线分别绕收银台、垃圾桶和货架端头，测量怪物能否继续追击以及玩家能否挣脱。实测全程不启用测试眩晕，记录 1080p High 的平均帧率及 P95 帧耗时，目标为平均至少 60 FPS。
3. **外部交付。** 用一台未装 UE 的 Windows x64 电脑从 Release 下载、解压、启动、存档并重启续玩。人工检查四语言在 720p 和 1080p 的任务区与字幕。三项通过后再排第四晚的内容。

本机主工作区里的 `Config/DefaultEngine.ini` 有开发机专用 AndroidFileServer 设置。发布从 Git 中的已提交配置构建。
