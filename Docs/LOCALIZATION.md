# 界面语言

游戏在每次启动时先显示语言选择，再进入标题页，固定顺序为 **1 English、2 日本語、3 繁體中文、4 简体中文**。上次选择会作为默认高亮；鼠标点击语言行即可确认，也可使用方向键／W、S 或数字 1、2、3、4 选择，按 Enter／Space 确认。标题页按 L 可以重新选择语言。

语言偏好保存在 UE 的 GameUserSettings.ini 的 `[ShiftStore.Language]` 节中，与 ShiftStoreSlot 游戏进度独立；重开夜晚不会重置语言。不根据操作系统语言自动跳过选择页。

## 文本与字体

- `Content/Localization/Strings/en.json`、`ja.json`、`zh-Hant.json`、`zh-Hans.json`：四份 UTF-8 语言包，各 154 条。覆盖语言与暂停菜单、任务名、商品名称、任务步骤与路线指引、关东煮准备、交互提示、顾客对白、异常提示、失败原因与结算。
- 英文原文是稳定键；格式字符串保留原来的 `%s/%d/%.2f` 等键名，语言包值使用 `{0}`、`{1}` 参数。代码用 `ShiftLoc::Format` 传入已格式化的数值，商品名等字符串参数会先查表翻译。SKU、资产名和游戏存档使用原来的稳定标识。
- `Tools/author_localization.py` 是四个语言选项的文案源，修改后运行以重新生成 JSON；运行时直接加载 JSON。繁体中文保存为独立、经过检查的文案列，重新生成时不依赖简繁转换库。缺少译文时回退英文，未列入语言包的品牌文本保持原样。
- Noto Sans CJK SC／JP／TC Regular 字体以独立 FontFace 和 Runtime Font 资产保存，HUD 构造期硬引用确保进入烘焙。顾客头顶文字由 HUD 投影绘制，并检查距离及视线，避免旧 TextRender 字体缺字。
- 场景内原创包装、品牌与实体招牌保留既有美术，未制作多套贴图。无配音翻译。

## 重建与验证

语言包生成及静态检查（Git Bash，Python 3，检查脚本需要 fontTools）：

```bash
export PYTHONUTF8=1 PYTHONIOENCODING=utf-8
python Tools/author_localization.py
python Tools/check_localization.py
```

字体源文件和许可证已随工程保留，通常无需重新下载或导入。需要恢复时可运行 `Tools/fetch_fonts.py` 下载 `SourceArt/Fonts/sources.json` 固定版本及校验来源，然后在完整 UE 编辑器的 Python 环境执行 `Tools/import_fonts.py`。该导入器需要 Slate，不能使用 `-run=pythonscript` 命令行模式；使用 `-ExecutePythonScript`。许可证及哈希见 `THIRD_PARTY_NOTICES.md` 和 `SourceArt/Fonts/`。

Windows 打包会将 JSON 作为 UFS 文件加入资源包，并将字体烘焙；玩家无需 Python 或系统字体。打包后执行 `python Tools/write_launchers.py` 更新操作说明并复制字体许可证。

独立包回归：

```bash
python Tools/test_localization_runtime.py
python Tools/check_gameplay.py
```

语言测试启动专用进程，注入引擎按键验证选择／确认／重新选择／进入游戏，读取磁盘设置，再启动新进程检查语言恢复，采集实际画面。脚本在 finally 中恢复已知位置的玩家设置，并核对正式进度存档哈希。需要显示驱动；默认用 RenderOffscreen，截图仍包含 HUD。报告位于 `Verification/localization_runtime_*.json`，选定截图位于 `Previews/Localization/`。完整临时截图与引擎日志在 `Saved/Localization/` 和 `Saved/QA/`。

开发预览可加 `-ShiftLanguage=en`、`-ShiftLanguage=ja`、`-ShiftLanguage=zh-Hant` 或 `-ShiftLanguage=zh-Hans` 临时指定高亮语言；该参数本身不写入偏好，玩家确认后才保存。语言偏好按语言代码保存，调整菜单顺序后仍能正确恢复旧选择。
