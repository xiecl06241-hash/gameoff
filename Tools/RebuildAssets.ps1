param([string]$EngineRoot = 'D:\UE\UE_5.8', [string]$Python = 'python')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editor)) { throw "Unreal editor not found: $editor" }
& $Python (Join-Path $PSScriptRoot 'author_labels.py')
if ($LASTEXITCODE -ne 0) { throw 'Original label authoring failed' }
$projectFile = (Join-Path $projectRoot 'ShiftStore.uproject').Replace('\','/')
$generator = (Join-Path $PSScriptRoot 'generate_store.py').Replace('\','/')
& $editor $projectFile '-run=pythonscript' "-script=$generator" '-unattended' '-NullRHI' '-nosplash'
if ($LASTEXITCODE -ne 0) { throw 'Scene generation failed' }
if (Test-Path -LiteralPath (Join-Path $projectRoot 'Saved/generation_error.txt')) { throw 'Generator reported an error; inspect Saved/generation_error.txt' }
$validator = (Join-Path $PSScriptRoot 'validate_scene.py').Replace('\','/')
& $editor $projectFile '-run=pythonscript' "-script=$validator" '-unattended' '-NullRHI' '-nosplash'
if ($LASTEXITCODE -ne 0) { throw 'Collision or asset validation failed' }
$report=Get-Content -LiteralPath (Join-Path $projectRoot 'Saved/validation.json') -Raw | ConvertFrom-Json
if ($report.errors.Count -gt 0) { throw 'Scene validation reported failures; inspect Saved/validation.json' }
