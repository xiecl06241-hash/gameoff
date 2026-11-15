param([string]$EngineRoot = 'D:\UE\UE_5.8')
$projectRoot=Split-Path -Parent $PSScriptRoot
$project=(Join-Path $projectRoot 'ShiftStore.uproject').Replace('\','/')
$script=(Join-Path $PSScriptRoot 'render_previews.py').Replace('\','/')
$log=(Join-Path $projectRoot 'Saved/PreviewEditor.log').Replace('\','/')
$arguments='"'+$project+'" -ExecutePythonScript="'+$script+'" -abslog="'+$log+'" -unattended -nosplash'
Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
