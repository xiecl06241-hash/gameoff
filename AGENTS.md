# SHIFT / 24 开发交接

更新于 2026-11-19。当前位于独立开发分支 `feat/shift-gameplay`（`main` 为 v0.4 发布提交 `5b54aa8`）。本文件记录了紧扣 **SHIFT** 主题的“去公式化 & 去无聊化”全套玩法升级、商品放回与散落清理规则、怪物追逐寻路修复及全套验证结果。

## 工程与启动

- 主分支为 `main`，本版玩法开发分支为 `feat/shift-gameplay`。玩家下载和已知问题见 [v0.4 发布说明](Docs/RELEASES/v0.4.md)。
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
   - 拖把和扫帚投中顾客或怪物后保留，可重新拾取；投中怪物仍能眩晕，且不计商品货损。只有普通商品会在命中后破损销毁，避免丢失两把清洁工具后无法完成夜末拖地。

## 阶段一演出、追逐、音频和结局

- 第一晚首次营业异变倒计时为 9 秒，后续夜晚为 14 秒。每晚开局重置局部异变轮次，第一晚首先出现迎宾牌篡改。该计时从第一位顾客进店后累计，玩家开店任务耗时会影响实际遭遇时刻。
- 顾客服装使用现有人物材质的 Paint Tint 参数形成五种色调。显形时显示不对称发光眼部，用同一骨骼的姿态副本制作反向躯干与 180 度扭头。原骨骼持续更新动画并提供碰撞；炭黑材质已启用骨骼网格用途，资产生成脚本同步记录此设置。
- 被抓时叠加 Pitch／Yaw／Roll 微震动，释放、死亡、返回标题或怪物销毁时清除镜头 Roll。固定镜头自动用例沿用原始镜头参数。
- 悬浮怪被注视时速度 55，背身时速度 335。歪头怪能听到冲刺并以 310 追击，慢走且远离时速度 185、视距 700。红眼怪追逐时每 5 秒闪灯，速度由 245 随时间增加至 290。六类自动测试参数会隔离这些额外追逐行为，正常游玩手感等待玩家实测。
- 怪物仅在近距离且视线未被实体障碍阻断时抓取玩家，墙另一侧无法触发抓取；无遮挡时正常追逐抓取和未目击偷袭仍然生效。
- 五个关键音效入口支持标准 16-bit PCM WAV，缺失或格式错误时回退合成音。未附带录制音频。文件名、格式及 Windows 包的放置目录见 Content/Localization/Audio/README.txt。仅加入尚未打包的 WAV 可以直接放入包内 Content 目录，替换已经放进 PAK 的同名音效需重新打包。
- 第三晚局部异变或全店 Shift 时，左下守则边框闪红并显示信号故障警示。结局展示识破数和全店异变数，零次全店异变且至少一次识破可进入真结局，否则为幸存结局。
- 新游戏重置结局统计，跨夜保存统计，当晚重开恢复当晚起始统计。旧存档缺少历史统计时继续保留进度；从后续夜晚导入的旧存档按历史不完整处理，结局使用幸存分支，显示计数从导入点开始累计。

## 玩法提交的验证记录

| 项目 | 结果 | 证据 |
| --- | --- | --- |
| 编辑器与 Windows Development 构建 | 成功，烘焙使用文件缓存配置 | Saved/QA/EditorBuild.log、WindowsPackage.log |
| 核心玩法 | 75 项通过，正式存档一致，含清洁工具与隔墙抓取回归 | Verification/gameplay_selftest.json |
| 故意失败路径 | 76 项中仅 forced_failure 失败，退出码 1 | Verification/gameplay_selftest_failure.json |
| 简中 720p 试玩反馈 | 84 项通过 | Verification/feedback_zh-Hans_1280.json |
| 三晚自动流程 | 13 项通过 | Verification/playthrough_1920.json |
| 四语言 1080p 界面 | 每种 24 项通过，包含重启语言读取 | Verification/localization_runtime_*_1920.json |
| 四语言暂停与灵敏度 | 每种分别 19／20 项通过 | Verification/pause_menu_*_1280.json、sensitivity_*_1280.json |
| 四语言文本与字体 | 各 213 条，字形覆盖通过 | Verification/localization_catalogues.json |

固定路线为第二晚首次显形时从收银区到仓库。1080p High，平均 60.00 FPS，P95 17.01 ms，最慢帧 21.31 ms。走测关闭随机局部异变和差异追逐，并会临时眩晕贴近的怪物。本轮自动结果不能代表正常追逐的难度或持续帧率。

音频新增断言同时检查缺失文件合成兜底、有效 PCM 读取、截断数据回退以及临时文件清理。结局断言检查完美与幸存分支，存档往返同时检查新增计数。

## 发布验收与后续试玩

从干净检出构建 Windows x64 包，校验 ZIP 后解压，用解压出的程序运行核心玩法、三晚流程、四语言界面、暂停及灵敏度回归。GitHub Release 同时提供游戏包和 SHA-256 校验文件。

三晚自动流程会关闭自然局部异变并缩短伪装等待时间。下一轮人工试玩重点是第二晚正常时限下的观察、取货与驱离，同时检查任务栏提前暴露身份、货架与展示品取货结果不同这两个已知问题。

## 构建与复现

在 Git Bash 中设置 UTF-8 和引擎路径后运行。

```bash
export PYTHONUTF8=1 PYTHONIOENCODING=utf-8
export UE_ROOT='D:/UE/UE_5.8'
python Tools/author_localization.py
python Tools/check_localization.py
python .github/scripts/check_repository.py --require-lfs
python .github/scripts/build_windows.py
python Tools/check_gameplay.py
python Tools/test_localization_runtime.py
python Tools/check_feedback.py
python Tools/check_pause_menu.py
python Tools/check_sensitivity.py
python Tools/check_feedback.py --playthrough
python Tools/test_localization_runtime.py
python Tools/check_pause_menu.py
python Tools/check_sensitivity.py
```

本机 Zen 服务有 IPv6 连接异常时，使用引擎内置的 -ddc=InstalledNoZenLocalFallback 附加烘焙参数。本轮构建通过本地临时包装脚本传参，标准构建脚本仍可用于正常缓存环境。

## 本轮交付与人工验收

修改和提交全部留在 feat/shift-gameplay。收到用户实测确认后再合并 main。本机试玩使用 Builds/Windows/ShiftStore.exe。优先实测首位顾客后的迎宾牌异变、三种怪客的注视／冲刺／闪灯应对、E 与商品挣脱后的镜头恢复、完整零显形通关及跨晚续档，再试听接入的真实 WAV。测试并发启动会影响声音和帧率，应逐项运行。
