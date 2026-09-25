param(
    [string]$HostRoot = $(if ($env:PLUGIN_HOST) { $env:PLUGIN_HOST } else { Join-Path $PSScriptRoot '..\..\..' }),
    [string]$Filter = 'STLogging',
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
if (-not $SkipBuild) { & (Join-Path $HostRoot 'Scripts\Build-TestProject.ps1') -EngineRoot $EngineRoot }

$uproject = (Resolve-Path (Join-Path $HostRoot 'TestProject\TestProject.uproject')).Path
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path ([IO.Path]::GetTempPath()) 'STLoggingTests.log'
if (Test-Path $log) { Remove-Item $log }

$argLine = "`"$uproject`" -ExecCmds=`"Automation RunTests $Filter; Quit`" -unattended -nullrhi -nosplash -nosound -abslog=`"$log`""
Start-Process -FilePath $exe -ArgumentList $argLine -Wait -NoNewWindow | Out-Null

$text = Get-Content $log -Raw
$results = [regex]::Matches($text, 'Test Completed\. Result=\{(\w+)\}')
$passed = @($results | Where-Object { $_.Groups[1].Value -eq 'Success' }).Count
$failed = @($results | Where-Object { $_.Groups[1].Value -ne 'Success' }).Count

if ($failed -gt 0) {
    Select-String -Path $log -Pattern 'Result=\{Fail|Error: ' | ForEach-Object { Write-Host $_.Line }
}
Write-Host "STLogging tests: $passed passed, $failed failed (log: $log)"
if ($failed -gt 0 -or $passed -eq 0) { exit 1 }
