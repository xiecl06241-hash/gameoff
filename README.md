# SHIFT / 24 — Convenience Store Night-Shift Horror (GitHub Game Off 2026)

**SHIFT / 24** is a 3D first-person convenience store night-shift horror game created in **Unreal Engine 5.8.2** for **GitHub Game Off 2026** around the theme **SHIFT** (*working the night shift* + *the store physically shifting into an abnormal state when an entity reveals itself*).

---

## English Overview (For Game Off 2026 Judges & Players)

### How to Play
1. Open `Builds/Windows` and double-click **`ShiftStore.exe`** (or **`启动正常场景.cmd`** for 1920×1080 fullscreen).
   Choose **1 English**, **2 日本語**, **3 繁體中文**, or **4 简体中文** before the title screen. Click a language or select it with arrows / `1`–`4` and press `Enter`. Your choice is remembered; press `L` on the title screen to change it.
   `Space` / `E` starts a **new game at Night 1**. Use `C` to explicitly continue an existing save.
2. Complete your assigned clerk chores before their timers expire—overdue tasks steadily drain your **Sanity**.
3. Once all active chores are finished, a customer enters the store (only one customer is inside at a time):
   - **Normal Customers** browse, occasionally ask you to fetch a specific product (`[E]` to pick up from a shelf and `[E]` to hand it over), and pay at the **Checkout Register** (`[E]`).
   - **Abnormal Entities** disguise themselves as customers and reveal themselves while you are fetching an item. When they reveal, **the entire store automatically SHIFTS**: middle gondolas slide 30 cm, main ceiling lights cut out leaving only cold-storage and exit lights, and a displaced item begins glowing red somewhere in the store.
   After the customer leaves, one chore is assigned: shelf tidying and oden preparation alternate, with a batch restock only after three paid sales. The last customer leads directly to dawn cleanup.
4. **Banishing an Entity**: Find the item the entity demands and deliver it to the **Checkout Register** (`[E]`) while avoiding or looping the entity around shelves.
   - Throwing merchandise (`[F]` or `Right Mouse Button`) stuns the entity temporarily (each product has a distinct wholesale cost and stun duration), or you can grab a hot cup from the **Oden Station** for a heavy stun.
   - Destroyed merchandise is deducted at wholesale cost from store revenue; exceeding the loss limit (`-$40`) terminates your run.
   - If grabbed, hold/mash `[E]` to struggle free slowly, or press `[F]`/`[RMB]` while holding a product to smash the entity and break free immediately.
5. After all customers for the night are served, equip the **Mop** in the back corridor, clean the floor spills, and survive until **06:00 AM Dawn**. Progress auto-saves after each night (3 nights total).

### Controls
| Key / Input | Action |
| --- | --- |
| `W` `A` `S` `D` | Move |
| `Mouse` (or Arrow Keys) | Look |
| `E` or `Left Mouse Button` | Interact / Pick up item / Hold to complete chore / Ring up / Struggle |
| `F` or `Right Mouse Button` | Throw held item (stuns entity on hit) |
| `G` | Drop held item |
| `Space` / `Enter` | Advance Title / Night Intro / Dawn screens |
| `R` | Restart current night from auto-save (on Game Over screen) |
| `Alt + F4` | Quit |
| `L` (title screen) | Change language |
| `C` (title screen) | Continue saved game |
| `Tab` (during chores) | Guide to the next chore |

### AI Usage Disclosure (GitHub Game Off 2026 Compliance)
In accordance with the GitHub Game Off 2026 rules on AI-assisted development:
- **Code & Tooling**: C++ gameplay systems (`Source/ShiftStore/`), Unreal Editor Python geometry/material authoring scripts (`Tools/generate_store.py`), and packaging/verification scripts were developed with the assistance of AI coding agents (Codex, Claude / Gemini in Antigravity).
- **Environment & Packaging Art**: All 59 store meshes, 44 procedural materials, and 21 product/signage textures (`SourceArt/Labels/`) are deterministically generated via Python (`Tools/author_labels.py` and `Tools/generate_store.py`) without third-party store asset packs or real-world brand logos.
- **Character & Template Assets**: First-person movement foundations and the `SKM_Manny_Simple` character mannequin/unarmed locomotion animations come from the official **Unreal Engine 5 First Person Template** (governed by the Unreal Engine EULA).

### License
- **Source Code & Scripts** (`Source/`, `Tools/`): [MIT License](LICENSE)
- **Original Procedural Store Meshes & Label Artwork** (`SourceArt/`, `Content/ShiftStore/`): [CC BY 4.0](LICENSE)
- **Noto Sans CJK interface fonts** (`SourceArt/Fonts/`, `Content/ShiftStore/Fonts/`): SIL Open Font License 1.1; see [third-party notices](THIRD_PARTY_NOTICES.md).

---

## 中文说明（玩法、工程与构建指南）

### 1. 直接试玩
打开 `Builds/Windows`，双击 **`启动正常场景.cmd`**（1920×1080 全屏）或 **`ShiftStore.exe`**。
按 `Space` 或 `E` 从标题页进入第一晚夜班。
标题页之前先选择 **1 English／2 日本語／3 繁體中文／4 简体中文**。鼠标点击，或用方向键、数字 `1/2/3/4` 选择后按 `Enter` 确认。会记住上次选择，标题页按 `L` 可重新选择；语言设置与夜晚进度存档独立。
标题页默认按 `Space`／`E` 开始**新游戏（第一晚）**；按 `C` 才会继续已有存档。日常任务有常驻步骤和黄色通道／距离标记，按 `Tab` 可切换指引的任务。当前三晚每项任务时限分别为 **240／180／120 秒**，夜晚说明页暂停计时。补货时先沿标记去仓库领取补货纸箱，再跟随新标记回到货架长按 `E`。

界面、任务、交互提示、顾客对白、失败与结算文字均支持上述四个语言选项。场景中的原创品牌、包装和实体标识暂保留原样。语言包和字体制作说明见 [Docs/LOCALIZATION.md](Docs/LOCALIZATION.md)。

### 2. 核心玩法与完整流程（3 晚 MVP）
- **日常店员任务与清醒度（Sanity）**：每项补货（从仓库取货箱 `Stock Carton`）、清点、整理、倒垃圾任务都有独立倒计时；逾期会持续扣除清醒度，完成后停止扣除并部分恢复清醒度。
- **顾客进店与服务优先级**：只有当前待办任务全部完成且店内无残留顾客/怪物时，下一位顾客才会进店（同时仅 1 位在店）。帮求助顾客找到指定商品（按 `E` 拿取并按 `E` 递交），并在收银台按 `E` 结账获得营业收入。
- **怪物显形与店铺自动转变（Shift）**：异常顾客会在你帮忙找商品时显形；显形瞬间**同一家便利店在运行时平滑转入 Shift 异常状态**（中排货架横移 30 cm、主照明熄灭仅留冷柜与出口灯、欢迎牌变为异常告示，同时刷出泛红光的异常需求商品）。若未目击显形，怪物会潜伏偷袭，抓到玩家时额外一次性扣除大段清醒度。
- **反制、挣脱与店铺亏损（Loss Limit）**：把异常顾客要求的商品送到收银台即可驱离怪物并让店铺恢复正常。期间可用商品或关东煮热汤（`F` 或鼠标右键投掷）砸晕怪物争取时间；损坏的商品按成本价计入负收入，累计亏损达到 `-$40` 或清醒度归零即淘汰。被抓时可按住/连按 `E` 缓慢挣脱，或手持商品按 `F`/`右键` 直接砸晕快速挣脱。
- **夜末打扫与自动存档**：服务完当晚全部顾客后，去后勤走廊拿起拖把（`Mop`）清理地面污渍即可迎来 `06:00 AM` 天亮并自动存档；失败可按 `R`/`Space` 回溯至当晚开头。

### 3. 工程结构与重新构建
用 **Unreal Engine 5.8.2** 与 **Visual Studio 2022** 打开 `ShiftStore.uproject`。
克隆仓库后请确保安装了 Git LFS（`git lfs install && git lfs pull`）。

| 目录 / 文件 | 说明 |
| --- | --- |
| `Source/ShiftStore/` | C++ 核心玩法模块（`AShiftStoreGameMode`、`AShiftDirector` 运行时 Shift 切换器、`AShiftCustomer` 顾客与怪物 AI、`AShiftPlayerCharacter`、`AShiftTaskTarget`、`AShiftItem`、`AShiftHUD`、`UShiftSaveGame`） |
| `/Game/ShiftStore/Maps/Normal` | 游戏主地图（包含运行时 Normal ↔ Shift 状态平滑切换） |
| `/Game/ShiftStore/Maps/Shift` | 独立异常状态参考地图（供环境预览对比） |
| `/Game/ShiftStore/Meshes` & `Materials` | 59 个原创静态网格、44 个程序化与印刷材质 |
| `SourceArt/` | 包装贴图生成结果、`products.json`、`scene_states.json`、`asset_catalog.json` |
| `Tools/RebuildAssets.ps1` | 从 Python 脚本确定性重建模型、贴图与场景并执行碰撞验证 |
| `Tools/PackageWindows.ps1` | 编译 C++ 并打包独立 Windows 可执行版本到 `Builds/Windows` |
| `Docs/游戏讨论整理.md` | 游戏策划讨论与规则文档 |
| `VERIFICATION.md` | 自动化场景、构建、玩法自检与性能验证记录 |

### 4. 回归检查

完成 Windows 打包后，在 Git Bash 的工程目录运行 `python Tools/check_gameplay.py`。脚本会启动两个隔离的独立包进程：正常用例必须返回 0，故意注入失败的用例必须返回 1，且两次运行前后的玩家存档内容必须保持一致。

自检覆盖：顾客模型与动画的打包加载、真实物理投掷命中、成本扣款、首夜及后续夜晚的账本回滚、GC 后的告示保活、跨帧验证货架与实例商品位置／灯光／告示切换和恢复、闪灯结束恢复，以及专用存档槽的读写与清理。它不等同于完整的人工通关验收。

自检需要专用进程参数 `-ShiftSelfTest -ShiftSelfTestExit`；正常游玩进程中的 `ShiftRunSelfTest` 命令会拒绝执行，避免破坏当前夜晚。自检只使用随机命名的 `ShiftStoreSelfTest_*` 存档槽，并在结束时清理。原始报告和日志位于 `Saved/QA`，可分享的结果位于 `Verification/gameplay_selftest*.json`。
