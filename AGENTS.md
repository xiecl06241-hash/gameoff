# SHIFT / 24 开发交接

更新于 2026-11-18。本文件与顾客挡路复测、三晚流程验证和操作说明更新一同提交。提交前基线为 `1658ba8`，本次完整改动可用 `git show HEAD --stat` 查看。

## 工程与启动

- 仓库为 `https://github.com/xiecl06241-hash/gameoff`，工作分支为 `main`。
- 本机工程位于 `D:\Game Jam\Game Off 2026\ShiftStore`，入口为 `ShiftStore.uproject`。
- 使用 UE 5.8.2，本机引擎位于 `D:\UE\UE_5.8`。编译工具为 Visual Studio 2022、MSVC 14.44 和 Windows SDK。
- 本机试玩入口为 `Builds/Windows/ShiftStore.exe`，同目录的 `启动正常场景.cmd` 以 1080p 全屏启动。完整保留 `Windows` 文件夹才能运行。
- 选择语言后，Space／E／Enter 从第一晚开始，C 继续存档。WASD 移动，E 交互或长按做任务，Tab 切换目标，Esc 暂停。暂停菜单可调整鼠标灵敏度。
- 当前公开下载为 `v0.2`，对应提交 `e50bec2`。本机包包含此后的关东煮位置调整与本次验证入口。`Builds/` 为本地交付目录，由 Git 忽略，源码提交和 Release 附件有各自的更新时间。

## 当前玩法

Windows 第一人称 3D 三晚原型，支持英语、日语、繁体中文和简体中文。七晚为后续内容目标。

1. 完成开店任务后顾客进店，店内同时接待一位顾客。
2. 顾客可能自行购物或请求帮助，正常顾客结账后实际走出店门。
3. 接待间派发整理货架或煮关东煮，累计三笔正常消费后安排补货。最后一位顾客离店后进入打扫。
4. 异常顾客显形时自动触发 Shift。玩家找到需求商品并交到收银台，使怪物离开、店铺恢复。
5. 清醒度归零或亏损过多会失败，支持当晚重开。打扫完成后天亮并保存进度。

第一晚为三位正常顾客，第二晚四位顾客中有一位异常顾客，第三晚五位顾客中有两位异常顾客。任务时限分别为 240／180／120 秒。具体配置在 `Source/ShiftStore/ShiftTypes.cpp`。

## 本次完成的工作

### 顾客挡路专项复测

旧用例把四种移动状态都指向固定坐标 `(1150, 50, 100)`，缺少无阻挡行走的前置验证。新用例按进店、购物、去收银台、离店分别设置起点和目的地，确认顾客正在行走，再将玩家放到前方可通行路段。

每种状态检查无阻挡移动、玩家站位、及时停止、目标保持、静止、计时冻结、让路台词、台词防重复、0.2 秒恢复延迟及恢复移动。购物场景还覆盖伪装顾客。当前顾客实现通过这些场景，产品行为沿用现有 `ShiftCustomer.cpp`。

测试还等待真实输入处理后再断言取箱成功，并为显形用例选取有效的异常顾客商品配置。源码见 `Source/ShiftStore/ShiftFeedbackTest.cpp` 和 `.h`。

### 三晚流程与走测

新增显式测试参数 `-ShiftPlaythroughTest`，由 `ShiftStoreGameMode::BeginPlay` 创建测试 Actor。脚本使用独立的 `ShiftStorePlaythrough-*` 用户目录，核对正式存档哈希。

顾客依照实际角色移动与状态机进店、购物、显形、离店。测试通过玩法回调推进普通顾客帮助、结账、怪物交付和任务完成，并核对每晚天亮及最终结局。它验证流程连接和顾客移动，完整键鼠通关、玩家理解与难度评价仍需人工验收。

第二晚首次显形期间，测试角色从收银区沿通道走到仓库，并记录墙钟帧间隔。该记录包含视角转向、Shift 过渡和实际角色移动。入口为 `Tools/check_feedback.py --playthrough`。

### 操作说明

`README.md`、`Docs/ONBOARDING.md`、`Tools/write_launchers.py` 已统一说明右上角任务区、黄色箭头和到达高亮。生成后的 `Builds/Windows/操作说明.txt` 已同步更新。

## 验证结果与范围

| 项目 | 本次结果 | 证据 |
| --- | --- | --- |
| 编辑器及 Windows Development 构建 | 成功 | `Saved/QA/EditorBuild.log`、`WindowsPackage.log` |
| 简体中文 720p 试玩反馈专项 | 84 项通过，其中挡路相关 40 项 | `Verification/feedback_zh-Hans_1280.json` |
| 核心玩法 | 54 项通过，正式存档哈希一致 | `Saved/QA/gameplay_pass.json` |
| 故意失败检查 | 55 项中只有强制失败项失败，退出码 1 | `Saved/QA/gameplay_expected_failure.json` |
| 三晚自动流程 | 13 项通过，走到第三晚结局，退出码 0 | `Verification/playthrough_1920.json` |
| 静态仓库检查 | Python、JSON、资产引用与 LFS 检查通过 | `.github/scripts/check_repository.py` |
| 让路字幕画面 | 已目视核对简体中文 | `Previews/Feedback/zh-Hans_1280_Courtesy.png` |

核心回归在本轮第一次成功打包后执行。随后对三晚测试的等待时限按顾客状态重新计算，再次构建并通过三晚测试。历史四语言暂停、灵敏度和本地化报告的日期、范围见 `VERIFICATION.md`，本次反馈专项覆盖简体中文 720p。

性能样本来自 RTX 5060 Laptop GPU，1920×1080 全屏 High，渲染比例 100%。第二晚收银区至仓库路线共 676 帧，平均 **54.98 FPS**，P95 **22.11 ms**，显形帧 **18.59 ms**，最慢帧 **47.80 ms**。该路线低于 60 FPS 目标，下一步需要采集 CPU／GPU 分项耗时来定位原因。历史 `Verification/performance.json` 为旧环境版固定机位数据。

## 构建与复现

在 Git Bash 中进入仓库目录。源码检出先完成 `git lfs pull`。构建时关闭占用本工程模块的 UE 编辑器。

```bash
cd '/d/Game Jam/Game Off 2026/ShiftStore'
export PYTHONUTF8=1 PYTHONIOENCODING=utf-8
export UE_ROOT='D:/UE/UE_5.8'
python .github/scripts/check_repository.py --require-lfs
python .github/scripts/build_windows.py
python Tools/check_gameplay.py
python Tools/check_feedback.py --languages zh-Hans
python Tools/check_feedback.py --playthrough
```

本机 Python 可用 `/c/D/anaconda/python`。打包后验证顺序执行，让性能走测独占游戏渲染进程。反馈专项支持 `--languages en ja zh-Hant zh-Hans`，1080p 可追加 `--width 1920 --height 1080`。

`--playthrough` 会创建独立用户目录并在结束后清理，实际日志保存在 `Saved/QA/playthrough_1920.log`。如果脚本超时，先确认它启动的游戏进程已退出，再进行下一次构建或测量。

## 后续优先项

1. 用同一路线采集游戏线程、渲染线程与 GPU 耗时，定位约 55 FPS 的瓶颈。保留当前 High／1080p 数据作为比较基线。
2. 安排没看过策划的玩家从第一晚开始盲测。记录首次独立找到工作点、首次说出异常变化、首次理解异常顾客需求的时刻，并记录卡住时的提示和动作。
3. 完成完整键鼠三晚通关，检查获取并递交商品、正常服务、抓取挣脱、失败重开以及所有随机购物点。
4. 扩展反馈专项到另外三种语言和 1080p，随后在一台没有 UE／Python 的 Windows 电脑上验证完整下载包。
5. 以上结果稳定后再安排七晚内容、最终人物与恐怖脸部美术。

## 交接时的工作区状态

本次提交包含本文件、复测代码、操作说明及新增验证证据。`Config/DefaultEngine.ini` 在本次工作前已有 15 行本地改动，留在工作区，单独检查其用途后再决定是否提交。配置中的本机令牌应避免进入日志或交接文本。

原有策划在 `Docs/游戏讨论整理.md`，其更新日期较早。当前可运行行为以源码和最近的验收条目核对，规划中的七晚与现有三晚需分别安排进度。
