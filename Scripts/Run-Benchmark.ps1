param(
    [string]$HostRoot = $(if ($env:PLUGIN_HOST) { $env:PLUGIN_HOST } else { Join-Path $PSScriptRoot '..\..\..' }),
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipBuild
)
# Runs the STLogBench commandlet: times ST_LOG against plain UE_LOG for plain messages,
# contexts of 1/4/16 values, a nested object, and the hot-path cost of building a context
# whose log never ends up emitting. Development editor only for now - packaging a Shipping
# Game target for this host project currently fails to launch at all (silent exit after
# LogMemory, no log/crash output); see docs for the open investigation.
$ErrorActionPreference = 'Stop'
if (-not $SkipBuild) { & (Join-Path $HostRoot 'Scripts\Build-TestProject.ps1') -EngineRoot $EngineRoot }

$uproject = (Resolve-Path (Join-Path $HostRoot 'TestProject\TestProject.uproject')).Path
$exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$out = Join-Path ([IO.Path]::GetTempPath()) 'STLogBench.Development.json'
$log = Join-Path ([IO.Path]::GetTempPath()) 'STLogBench.log'
if (Test-Path $log) { Remove-Item $log }

$argLine = "`"$uproject`" -run=STLogBench `"-out=$out`" -unattended -nullrhi -nosplash -nosound -abslog=`"$log`""
Start-Process -FilePath $exe -ArgumentList $argLine -Wait -NoNewWindow | Out-Null

if (-not (Test-Path $out)) { Write-Host "STLogBench FAILED: no output written (log: $log)"; exit 1 }

$result = Get-Content $out -Raw | ConvertFrom-Json
Write-Host "STLogBench: $($result.config) / $($result.platform), noLogging=$($result.noLogging)"
$result.results | ForEach-Object {
    "{0,-18} {1,8} iters  {2,10:N1} ns/call" -f $_.scenario, $_.iterations, $_.nsPerCall | Write-Host
}
Write-Host "Full results: $out"
