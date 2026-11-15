from pathlib import Path
import json,shutil,datetime
r=Path(__file__).resolve().parents[1]
out=r/'Verification';out.mkdir(exist_ok=True)
scene=json.loads((r/'Saved/validation.json').read_text(encoding='utf-8'))
launch=json.loads((r/'Saved/launch_checks.json').read_text(encoding='utf-8'))
if (r/'Saved/launch_fullscreen_checks.json').exists():
    launch+=json.loads((r/'Saved/launch_fullscreen_checks.json').read_text(encoding='utf-8'))
assert not scene['errors'] and scene['round_trip_restores_normal']
assert all(x['exit_code']==0 for x in launch)
shutil.copy2(r/'Saved/validation.json',out/'scene_checks.json')
(out/'launch_checks.json').write_text(json.dumps(launch,indent=2),encoding='utf-8')
results=[]
for name in ('Normal_Offscreen','Shift_Offscreen','Normal_Windowed','Shift_Windowed','Normal_Fullscreen','Shift_Fullscreen'):
    p=r/'Saved/QA'/(name+'.json')
    if p.exists():
        x=json.loads(p.read_text(encoding='utf-8'))
        results.append({k:x[k] for k in ('state','render_mode','exit_code','screenshot','performance') if k in x})
(out/'performance.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
rows=[]
for x in results:
    p=x.get('performance',{})
    if p:
        rows.append('| '+('正常' if x['state']=='Normal' else '异常')+' | '+{'offscreen':'离屏','windowed':'窗口','fullscreen':'全屏'}[x['render_mode']]+f" | {p['average_fps']:.1f} | {p['mean_frame_ms']:.2f} | {p['p95_frame_ms']:.2f} | {p['mean_gpu_ms']:.2f} | {x['exit_code']} |")
catalog=json.loads((r/'SourceArt/asset_catalog.json').read_text(encoding='utf-8'))
doc=f'''# 首批便利店环境验收记录

记录日期：{datetime.date.today().isoformat()}。本批交付环境、模型、材质、生成源码、两态地图与 Windows 漫游预览。已完成项目级生成、烘焙、打包与启动测试；交互实操和前台稳定 60 FPS 仍有验收限制，不能将下述离屏结果当成完整游戏性能承诺。

## 资产与工程

- 引擎：本机 UE 5.8.2，Windows Development 独立包，DX12 / SM6、软件 Lumen。
- 原创生成部分：{len(catalog)} 个独立静态网格、44 个材质、21 张原创包装／标识贴图；模型库共 {sum(x['triangles'] for x in catalog.values()):,} 个三角形，场景陈列复用实例。
- 店铺约 16×16 米、层高 3.2 米；营业厅、1.5 米后勤走廊、仓库、办公室、卫生间。
- 六组双面货架、两组墙架、四个冷柜、饮品柜、收银／关东煮区，后勤家具、工具与十二类商品。
- Shift 变体：中间两组货架各偏移 30 厘米、四件商品换位、告示与照明变化。建筑与模型资产共用；通过独立地图及 `SourceArt/scene_states.json` 保存两态配置。
- 材质有边缘倒角、金属／塑料／玻璃／纸张区别及程序化细微纹理。此为程序化首版环境，不表示已达到参考商业作品的最终写实品质。

## 自动检查结果

| 检查 | 结果 |
| --- | --- |
| 从生成脚本完整重建 | 成功；生成命令 0 错误 |
| Windows BuildCookRun | BUILD SUCCESSFUL；退出码 0 |
| 正常地图缺失材质／空网格 | 无 |
| 异常地图缺失材质／空网格 | 无 |
| 十二种印刷材质的纹理连接与实例化支持 | 全部通过 |
| 正常货架净距 | 最窄 160 cm |
| 异常货架净距 | 最窄 130 cm，超过 120 cm 要求 |
| 两态各 9 条路线、10 段胶囊扫掠 | 全部通过，覆盖入口、营业厅、后勤走廊与全部房间 |
| 胶囊检查参数 | 从实际第一人称角色读取；半径 {scene['player_capsule_cm']['radius']:.2f} cm，半高 {scene['player_capsule_cm']['half_height']:.0f} cm |
| 穿过货架的碰撞负对照 | 两态均命中，确认检测没有整体失效 |
| 可交互网格简单碰撞 | 18 个商品／工具／门网格均有简单碰撞，可供后续物理系统使用 |
| 可交互 Actor 移动性 | 两态各 22 个目标均为 Movable，带交互标记 |
| Normal → Shift → Normal 恢复 | 通过，货架位置恢复 |
| 独立包启动与退出 | 正常、异常的窗口／全屏结果见 `Verification/launch_checks.json`；常规运行均返回 0 |

原始场景检查保存在 `Verification/scene_checks.json`，启动结果保存在 `Verification/launch_checks.json`。门轴按左下铰链建模，门在预览中保持打开；尚未实现或验收开门动画、抓取与投掷玩法。

## 画面检查

`Previews/index.html` 提供入口、营业厅、收银、后勤走廊、仓库、办公室和卫生间共 14 张正常／异常对照图，均为 UE 实际渲染。另有独立包入口截图 `Normal_Runtime.png` 与 `Shift_Runtime.png`。检查了整体比例、货架遮挡、包装显示与局部灯光；未发现缺失材质或明显破面。

正常状态使用顶灯，异常状态压低主灯、保留冷柜、出口与应急局部照明。异常场景较暗，截图可见货架、通道与冷柜轮廓；在不同显示器亮度下的辨路体验尚需前台实操复查。环境声音仅预留位置，未接入实际声音。

## 1080p 性能实测

设备：AMD Ryzen 9 8945HX、NVIDIA RTX 5060 Laptop GPU（8 GB），驱动 610.88，Windows 11。独显 DX12，High（`sg.*=2`）、100% 渲染比例、关闭垂直同步与帧率上限。

固定入口机位采样，取 CSV 的第 400–799 帧，排除启动与第 300 帧的高分辨率截图操作，每项 400 帧。平均 FPS 使用真实 `FrameTime` 的均值计算，并非设置目标帧率所得。短时静止采样不代表全部机位、完整漫游或最终玩法的最坏表现。

| 状态 | 模式 | 平均 FPS | 平均帧耗时 ms | P95 ms | 平均 GPU ms | 测试退出码 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
{chr(10).join(rows)}

全屏模式的正常、异常固定入口机位均约 60 FPS，P95 帧耗时约 16.8 ms；CSV 元数据确认分辨率为 1920×1080。交付启动器使用全屏模式。窗口测试期间未能确认游戏取得前台焦点，窗口与离屏／全屏结果存在明显差异，具体原因尚未完全定位；**完整前台漫游稳定 60 FPS 未通过全部操作验收**。没有修改系统或显卡驱动的全局设置。

启用 CSV 性能记录的自动测试在完成采样、截图和引擎关闭日志后返回 **777003**；本机引擎源码将其定义为崩溃报告线程自身异常。该自动测试退出项记为失败，尚未修复。无 CSV 记录器的正常／异常窗口运行均能正常返回 0，因此没有把性能记录器加入试玩启动器。没有将异常退出计作通过。

精简性能数据在 `Verification/performance.json`；原始 CSV 和运行日志保留在本机 `Saved/QA`、`Saved/*_Runtime.log`，没有放进发布压缩包。

## 尚未完成的验证与范围边界

- 原生 UI 自动化两次无法激活捕获到的游戏窗口，已停止该方式；WASD、鼠标观察与所有房间的手动连续漫游未完成操作验收。控制使用 Epic 第一人称模板，路线已做真实角色尺寸的碰撞验证，但两者不能替代完整人工实操。
- 尚未在另一台没有 UE／Python 的干净 Windows 电脑上复测。独立包已烘焙全部运行资源，编辑器 Python 插件仅限 Editor 目标，运行无生成步骤；随包保留 Visual C++ 运行库安装文件。
- 顾客、怪物、角色动画、店员任务、七晚流程和实际音效属于后续阶段。玩家没有主动切换世界的能力；两个启动入口只用于开发验证。
- 正式最低配置、所有硬件的性能、最终画面品质和完整玩法验收不属于本批已通过项目。
'''
(r/'VERIFICATION.md').write_text(doc,encoding='utf-8')
manifest=[]
for state in ('Normal','Shift'):
    for view in ('Entrance','Shop','Checkout','Corridor','Stockroom','Office','Restroom'):
        name=f'{state}_{view}.png'
        assert (r/'Previews'/name).exists()
        manifest.append({'state':state,'view':view,'file':name})
(r/'Previews/capture_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('Verification written; scene checks passed; limitations explicitly recorded.')
