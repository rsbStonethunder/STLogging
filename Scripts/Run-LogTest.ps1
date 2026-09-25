param(
    [string]$HostRoot = $(if ($env:PLUGIN_HOST) { $env:PLUGIN_HOST } else { Join-Path $PSScriptRoot '..\..\..' }),
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipBuild
)
# Runs the STLogTest commandlet: scripted ST_LOG scenarios that emit real UE logs and are
# checked in-process against expected output. Pass/fail comes from the commandlet's summary line,
# not its exit code: the Error-verbosity scenario makes the engine report a nonzero exit by design.
$ErrorActionPreference = 'Stop'
if (-not $SkipBuild) { & (Join-Path $HostRoot 'Scripts\Build-TestProject.ps1') -EngineRoot $EngineRoot }

$uproject = (Resolve-Path (Join-Path $HostRoot 'TestProject\TestProject.uproject')).Path
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path ([IO.Path]::GetTempPath()) 'STLoggingLogTest.log'
if (Test-Path $log) { Remove-Item $log }

$argLine = "`"$uproject`" -run=STLogTest -unattended -nullrhi -nosplash -nosound -abslog=`"$log`""
$proc = Start-Process -FilePath $exe -ArgumentList $argLine -Wait -NoNewWindow -PassThru

$lines = Get-Content $log | Where-Object { $_ -match 'LogSTLoggingTest' }
$lines | ForEach-Object { Write-Host $_ }
if (-not ($lines -match 'STLogTest: \d+ scenarios, 0 failed')) { Write-Host "STLogTest FAILED (log: $log)"; exit 1 }
Write-Host "STLogTest OK (log: $log)"
