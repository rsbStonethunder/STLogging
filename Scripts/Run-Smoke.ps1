param(
    [string]$HostRoot = $(if ($env:PLUGIN_HOST) { $env:PLUGIN_HOST } else { Join-Path $PSScriptRoot '..\..\..' }),
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
if (-not $SkipBuild) { & (Join-Path $HostRoot 'Scripts\Build-TestProject.ps1') -EngineRoot $EngineRoot }

$uproject = (Resolve-Path (Join-Path $HostRoot 'TestProject\TestProject.uproject')).Path
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path ([IO.Path]::GetTempPath()) 'STLoggingSmoke.log'
if (Test-Path $log) { Remove-Item $log }

$argLine = "`"$uproject`" -run=STSmoke -unattended -nullrhi -nosplash -nosound -abslog=`"$log`""
Start-Process -FilePath $exe -ArgumentList $argLine -Wait -NoNewWindow | Out-Null

$text = Get-Content $log -Raw
$expected = @(
    '\[USTSmokeRoot::Describe\] describe frames=3 \{Leaf: null\}',
    '\[USTSmokeRoot::Describe\] describe frames=3 \{Leaf\.Count: 7, Leaf\.Label: L\}',
    '\[USTSmokeCommandlet::Main\] main done \{Root\.Frames: 3, Root\.Leaf\.Count: 8, Root\.Leaf\.Label: L\}',
    '\[USTSmokeRoot::StaticDescribe\] static',
    '\[STSmokeFreeFunction\] free'
)
$missing = @($expected | Where-Object { $text -notmatch $_ })
if ($missing.Count -gt 0) {
    $missing | ForEach-Object { Write-Host "MISSING: $_" }
    Write-Host "Smoke FAILED (log: $log)"
    exit 1
}
Write-Host "Smoke OK (log: $log)"
