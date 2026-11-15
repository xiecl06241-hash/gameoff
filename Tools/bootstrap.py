"""Create the editable UE project from locally installed Epic template content."""
from pathlib import Path
import json
import shutil
import re

ROOT = Path(__file__).resolve().parents[1]
ENGINE = Path(r"D:\UE\UE_5.8")
TEMPLATE = ENGINE / "Templates/TP_FirstPersonBP"

def main():
    for folder in ("Config", "Content/FirstPerson"):
        shutil.copytree(TEMPLATE / folder, ROOT / folder, dirs_exist_ok=True)
    for shared in ("Characters", "Input", "LevelPrototyping"):
        source = ENGINE / "Templates/TemplateResources/High" / shared / "Content"
        shutil.copytree(source, ROOT / "Content" / shared, dirs_exist_ok=True)
    project = {
        "FileVersion": 3, "EngineAssociation": "5.8",
        "Category": "Games", "Description": "Shift Store — modular environment walkthrough",
        "Plugins": [
            {"Name": name, "Enabled": True, "TargetAllowList": ["Editor"]}
            for name in ("PythonScriptPlugin", "EditorScriptingUtilities", "GeometryScripting", "ModelingToolsEditorMode")
        ] + [{"Name": "EnhancedInput", "Enabled": True}]
    }
    (ROOT / "ShiftStore.uproject").write_text(json.dumps(project, indent=2), encoding="utf-8")
    config = (ROOT / "Config/DefaultEngine.ini").read_text(encoding="utf-8")
    config = re.sub(r'(?ms)^\[/Script/AndroidFileServerEditor\.AndroidFileServerRuntimeSettings\]\s*\n.*?(?=^\[|\Z)', '', config)
    config = config.replace("/Game/FirstPerson/Lvl_FirstPerson.Lvl_FirstPerson", "/Game/ShiftStore/Maps/Normal.Normal")
    config += "\n[/Script/Engine.RendererSettings]\nr.RayTracing=False\nr.Lumen.HardwareRayTracing=False\nr.DefaultFeature.MotionBlur=False\nr.DefaultFeature.AutoExposure=True\nr.DefaultFeature.Bloom=True\n\n[/Script/Engine.AudioSettings]\nbAllowPlayWhenSilent=False\n"
    (ROOT / "Config/DefaultEngine.ini").write_text(config, encoding="utf-8")
    (ROOT / "Config/DefaultGame.ini").write_text('''[/Script/EngineSettings.GeneralProjectSettings]
ProjectName=Shift Store
ProjectVersion=0.1.0
Description=Convenience store environment preview
CompanyName=Shift Store Project
CopyrightNotice=Original environment assets; Epic template character content under Unreal Engine terms.

[/Script/UnrealEd.ProjectPackagingSettings]
BuildConfiguration=PPBC_Development
FullRebuild=False
ForDistribution=False
IncludeDebugFiles=False
IncludeCrashReporter=True
UsePakFile=True
bUseIoStore=True
bUseZenStore=False
bEnablePakStreaming=False
bCookAll=False
bSkipEditorContent=True
bCompressed=True
+MapsToCook=(FilePath="/Game/ShiftStore/Maps/Normal")
+MapsToCook=(FilePath="/Game/ShiftStore/Maps/Shift")
''', encoding="utf-8")
    (ROOT / "Config/DefaultGameUserSettings.ini").write_text('''[/Script/Engine.GameUserSettings]
bUseVSync=False
ResolutionSizeX=1920
ResolutionSizeY=1080
LastUserConfirmedResolutionSizeX=1920
LastUserConfirmedResolutionSizeY=1080
FullscreenMode=0
LastConfirmedFullscreenMode=0
Version=5
FrameRateLimit=0.000000

[ScalabilityGroups]
sg.ResolutionQuality=100
sg.ViewDistanceQuality=2
sg.AntiAliasingQuality=2
sg.ShadowQuality=2
sg.GlobalIlluminationQuality=2
sg.ReflectionQuality=2
sg.PostProcessQuality=2
sg.TextureQuality=2
sg.EffectsQuality=2
sg.FoliageQuality=1
sg.ShadingQuality=2
''', encoding="utf-8")
    print("BOOTSTRAP_OK", ROOT)

if __name__ == "__main__":
    main()
