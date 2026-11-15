param([string]$EngineRoot = 'D:\UE\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$projectFile = (Join-Path $projectRoot 'ShiftStore.uproject').Replace('\','/')
$output = (Join-Path $projectRoot 'Builds').Replace('\','/')
& $uat 'BuildCookRun' "-project=$projectFile" '-noP4' '-platform=Win64' '-clientconfig=Development' '-build' '-cook' '-stage' '-pak' '-iostore' '-archive' "-archivedirectory=$output" '-AdditionalCookerOptions=-SkipZenStore' '-CrashReporter' '-utf8output' '-unattended' '-prereqs'
if ($LASTEXITCODE -ne 0) { throw 'Windows packaging failed; inspect the AutomationTool log' }
& python (Join-Path $PSScriptRoot 'write_launchers.py')
if ($LASTEXITCODE -ne 0) { throw 'Launch file creation failed' }
