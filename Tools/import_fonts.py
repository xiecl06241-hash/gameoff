"""Run in Unreal Editor Python to create cookable runtime font assets."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
report = {"passed": False, "assets": []}
try:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for suffix in ["sc", "jp", "tc"]:
        font_path = f"/Game/ShiftStore/Fonts/NotoSansCJK{suffix}-Regular_Font"
        if unreal.EditorAssetLibrary.does_asset_exist(font_path):
            report["assets"].append(font_path)
            continue
        filename = root / f"SourceArt/Fonts/NotoSansCJK{suffix}-Regular.otf"
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(filename))
        task.set_editor_property("destination_path", "/Game/ShiftStore/Fonts")
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        factory = unreal.FontFileImportFactory()
        factory.set_editor_property("batch_create_font_asset", unreal.BatchCreateFontAsset.CREATE_IF_NO_FONT_EXISTS)
        task.set_editor_property("factory", factory)
        tools.import_asset_tasks([task])
        report["assets"] += task.get_editor_property("imported_object_paths")
    report["all_assets"] = list(unreal.EditorAssetLibrary.list_assets("/Game/ShiftStore/Fonts"))
    fonts = []
    for suffix in ["sc", "jp", "tc"]:
        path = f"/Game/ShiftStore/Fonts/NotoSansCJK{suffix}-Regular_Font"
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.Font):
            asset.set_editor_property("legacy_font_size", 18)
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
            fonts.append(path)
    if len(fonts) != 3:
        raise RuntimeError(f"Expected three runtime fonts, got {fonts}")
    report["fonts"] = fonts
    report["passed"] = True
finally:
    dest = root / "Saved/QA/font_import.json"
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    unreal.log("SHIFT_FONT_IMPORT: " + json.dumps(report))
