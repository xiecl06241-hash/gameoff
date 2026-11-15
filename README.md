# SHIFT / 24 — 首批环境资产

UE 5.8.2 制作的架空便利店环境，包含正常、异常两套地图和独立 Windows 第一人称漫游预览。

## 直接试玩

打开 `Builds/Windows`，双击 **启动正常场景.cmd** 或 **启动异常场景.cmd**，以 1920×1080 全屏预览。直接双击 `ShiftStore.exe` 默认进入正常场景。

WASD 移动，鼠标观察，Alt+F4 退出。试玩需要保留整个 Windows 文件夹，无需安装 UE 或 Python。门在这批环境预览中保持打开，方便检查全部区域。

## 已制作内容

- 约 16×16 米的单层店铺，层高 3.2 米；营业厅、后勤走廊、仓库、办公室与卫生间。
- 六组双面货架、两组靠墙货架、四个冷柜、一个饮品柜、收银区及关东煮台。
- 办公、仓储、卫生间家具，以及扫帚、拖把、桶、垃圾桶、灭火器等道具。
- 十二类原创商品模型及包装：水、牛奶、汽水、杯面、零食、饼干、罐头、面包、纸巾、盒饭、调味瓶、清洁剂。
- 正常／异常状态：两组中间货架分别横移 30 厘米、四件独立商品换位、一处告示变化、照明变化。
- 保留冷柜与出口局部照明，配置制冷、呼吸、脚步等未来音效使用的空间锚点。

本批交付环境资产、材质、状态配置与漫游预览。顾客、怪物、角色动画、任务逻辑和七晚流程按批准计划留在后续阶段。状态选择用于查看环境效果，不是游戏中的主动切换能力。

## 编辑工程与模型

用 UE 5.8.2 打开 `ShiftStore.uproject`。

从 GitHub 克隆时需要 Git LFS，以下载 `.uasset`、`.umap` 模型和地图文件。克隆后如缺少资产，先在工程目录运行 `git lfs install`、`git lfs pull`。引擎缓存、运行日志和 Windows 打包文件不进入源码仓库；独立预览需要运行 `Tools/PackageWindows.ps1` 生成。

`Docs/游戏讨论整理.md` 是策划文档的同步快照；本机策划原稿仍维护在 `C:\obisidian\GAMEJAM\game off 2026\游戏讨论整理.md`。

| 内容 | 工程位置 |
| --- | --- |
| 正常地图 | `/Game/ShiftStore/Maps/Normal` |
| 异常地图 | `/Game/ShiftStore/Maps/Shift` |
| 独立模型 | `/Game/ShiftStore/Meshes` |
| 程序化材质、印刷材质 | `/Game/ShiftStore/Materials` |
| 原创包装与招牌贴图 | `/Game/ShiftStore/Textures` |
| 模型目录与转轴说明 | `SourceArt/asset_catalog.json` |
| 两态位置配置 | `SourceArt/scene_states.json` |
| 商品基础资料 | `SourceArt/products.json` |

门与冷柜门的转轴位于左下铰链处，分别保存为独立网格；家具和商品主要采用底部中心转轴。可拾取样品、工具、门及音源锚点带有 Actor Tags，供后续交互系统识别。普通陈列商品采用实例化网格减少绘制开销，四件独立商品样品可单独配置交互。

价格资料是资产样例数据，尚未作为正式游戏平衡数值。

## 可重复生成

几何和材质由 `Tools/generate_store.py` 使用 UE Geometry Scripting 生成。包装图案由 `Tools/author_labels.py` 从文字、矢量图形生成，源文件保存在 `SourceArt`。没有购买或下载第三方店铺模型、品牌包装或贴图。

开发机可运行 `Tools/RebuildAssets.ps1` 重新生成并验证；所需工具为 UE 5.8.2、Python 和 Pillow。**重新生成会重建这两张自有场景并更新生成资产，手工调整版本应先另存。**正常编辑和试玩不需要重新生成。

`Tools/PackageWindows.ps1` 打包到 `Builds/Windows`。烘焙采用本地文件输出，独立包中的网格与材质已保存，不依赖运行时 Python、生成脚本或 Zen 服务器。

## 验证与预览

`Previews` 保存引擎实际渲染的正常／异常对比截图。`Saved/validation.json` 检查两态通道、后勤房间可达性、碰撞对照、材料分配与商品位置。`Tools/runtime_smoke.py` 启动独立包，记录运行日志、截图与 CSV 帧性能。

验证结果汇总见 `VERIFICATION.md`，精简检查数据保存在 `Verification`。本机全屏固定入口机位采样约 60 FPS；完整手动漫游仍有验收限制。CSV 性能记录器的退出异常单独记录，试玩启动器不启用该记录器。

## 内容来源

店铺几何、道具、程序化材质、图案和包装由本项目生成。第一人称控制器及其依赖来自本机 Epic First Person 模板，适用 Unreal Engine 的相关条款。源工程保留这部分模板依赖，便于继续开发。
