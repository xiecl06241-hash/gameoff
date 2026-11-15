# SHIFT / 24 — 验收与回归记录

当前工作区包含 C++ 三晚 MVP、正常／异常环境地图及 Windows 独立包。本次修复运行时资源、碰撞、重开状态和自检问题；以下回归结果不能替代完整的人工通关验收。

## 本轮修复

1. 顾客模型和动画使用构造期资源引用，随依赖一起烘焙，避免编辑器可见而独立包缺失。
2. Shift 告示等动态加载资产使用 `UPROPERTY(Transient)` 保活；欢迎牌组件设为 Movable，允许运行时更换网格。
3. 顾客胶囊使用独立 `ShiftCustomer` 碰撞通道；商品仅在飞行时与其阻挡，持续忽略玩家 Pawn，并启用 CCD。
4. 每晚开始建立内存检查点，死亡重开恢复该夜的收入、亏损和清醒度；缺失存档时有明确默认值。
5. 自检仅允许在带 `-ShiftSelfTest` 的专用进程中运行，使用随机 `ShiftStoreSelfTest_*` 存档槽并清理；正式存档保持不变。
6. 自检等待真实帧推进，检查实际货架、实例商品、灯光、告示与曝光；失败写入 JSON 并返回非零退出码。
7. 闪烁倒计时结束时强制应用一次完整状态，恢复亮度与灯罩材质。

## 本轮自动验证

使用 UE 5.8.2、Visual Studio 2022 / MSVC 14.44，Windows Development、DX12 / SM6。

| 检查 | 结果与证据 |
| --- | --- |
| 编辑器 C++ 编译 | `Result: Succeeded`；本机日志 `Saved/QA/EditorBuild.log` |
| Windows 编译、烘焙、打包 | `BUILD SUCCESSFUL`，退出 0；本机日志 `Saved/QA/PackageFixes.log` |
| 独立包玩法回归 | 24 项全部通过，退出 0；`Verification/gameplay_selftest.json` |
| 故意注入失败 | 其余检查通过，`forced_failure=false`、总体 `passed=false`，退出 1；`Verification/gameplay_selftest_failure.json` |
| 正式存档保护 | 正、反两次用例均核对正式存档内容；外部脚本另核对已知存档位置的 SHA-256，均未改变 |
| 测试存档清理 | 正、反两次用例均确认专用存档槽已删除 |
| 普通窗口启动与退出 | 正常／异常地图均通过、退出 0；`Verification/launch_checks.json` |

玩法回归在独立包的 NullRHI 模式执行，覆盖真实物理命中与成本扣除、顾客及其烘焙资源、首夜／后续夜晚检查点、任务完成后的清醒度恢复、GC 后资产保活、顾客显形、跨帧 Shift 及恢复、闪灯结束恢复，以及专用存档的读写与清理。它没有通过玩家按键走完全部三晚。

复现：先运行 `Tools/PackageWindows.ps1` 打包，再从 Git Bash 执行 `python Tools/check_gameplay.py`。脚本必须同时验证正常成功和预期失败，才会更新 `Verification` 中的结果。常规游玩启动器不启用自检。

## 环境资产与历史基线

- 原创环境：59 个独立静态网格、44 个材质、21 张包装／标识贴图，模型库 36,304 个三角形。
- 两态场景几何检查：正常最窄通道 160 cm，异常 130 cm；9 条路线、10 段真实角色尺寸胶囊扫掠通过。原始环境检查见 `Verification/scene_checks.json`。
- `Previews` 的 14 张截图是此前环境版本的对照图，不代表本轮新增顾客与 HUD 的画面验收。
- `Verification/performance.json` 是此前环境版本的固定入口机位基线：本机 RTX 5060 Laptop GPU、1080p High，全屏约 60 FPS；不能据此声称当前 C++ 玩法版本已完成整段漫游性能验收。
- 旧 CSV 性能采样存在退出码 777003 的记录，未在本轮将其认定为已修复；本轮回归不使用该记录器。

## 未完成的验收

- 完整三晚人工通关、键鼠交互体验、所有顾客路线及新版本的持续性能测试。
- 在另一台没有 UE／Python 的干净 Windows 电脑上复测运行包。
- 正式七晚内容、最终人物／怪物美术品质，以及更多环境音效。当前代码为三晚 MVP，策划中的七晚目标仍属于后续内容。
