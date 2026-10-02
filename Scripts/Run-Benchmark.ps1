param(
    [string]$HostRoot = $(if ($env:PLUGIN_HOST) { $env:PLUGIN_HOST } else { Join-Path $PSScriptRoot '..\..\..' }),
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$Shipping,
    [string]$StagingDir = (Join-Path ([IO.Path]::GetTempPath()) 'STLogBenchStaged'),
    [switch]$SkipBuild
)
# Runs the STLogBench commandlet: times ST_LOG against plain UE_LOG for plain messages,
# contexts of 1/4/16 values, a nested object, and the hot-path cost of building a context
# whose log never ends up emitting, plus controls that show the timing loop isn't optimised
# away. It also checks whether ST_LOG argument expressions were evaluated (they must not be
# under NO_LOGGING).
#   default:    Development editor (UnrealEditor-Cmd).
#   -Shipping:  builds the TestProject game target in Shipping, cooks/stages/paks it into
#               -StagingDir, and runs the staged exe. -SkipBuild reuses an existing stage.
$ErrorActionPreference = 'Stop'
$uproject = (Resolve-Path (Join-Path $HostRoot 'TestProject\TestProject.uproject')).Path
$config = if ($Shipping) { 'Shipping' } else { 'Development' }
$out = Join-Path ([IO.Path]::GetTempPath()) "STLogBench.$config.json"
$log = Join-Path ([IO.Path]::GetTempPath()) 'STLogBench.log'
if (Test-Path $out) { Remove-Item $out }
if (Test-Path $log) { Remove-Item $log }

if (-not $SkipBuild) {
    # The cook runs the editor, so it needs an up-to-date editor build either way. Built here
    # rather than by BuildCookRun -build, which Live Coding in any open editor would block.
    & (Join-Path $HostRoot 'Scripts\Build-TestProject.ps1') -EngineRoot $EngineRoot
    if ($Shipping) {
        & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') TestProject Win64 Shipping "-Project=$uproject" -WaitMutex -NoHotReload
        if ($LASTEXITCODE -ne 0) { throw "Shipping build failed with exit code $LASTEXITCODE" }
        & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$uproject" -platform=Win64 `
            -clientconfig=Shipping -cook -stage -pak -nocompile -nocompileeditor -unattended -nop4 -utf8output "-stagingdirectory=$StagingDir"
        if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed with exit code $LASTEXITCODE" }
    }
}

if ($Shipping) {
    # Shipping compiles logging out, so there is no -abslog to read: the JSON is the only output.
    $exe = Join-Path $StagingDir 'Windows\TestProject\Binaries\Win64\TestProject-Win64-Shipping.exe'
    if (-not (Test-Path $exe)) { Write-Host "STLogBench FAILED: no staged exe at $exe (run without -SkipBuild)"; exit 1 }
    $argLine = "-run=STLogBench `"-out=$out`" -unattended -nullrhi"
} else {
    $exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $argLine = "`"$uproject`" -run=STLogBench `"-out=$out`" -unattended -nullrhi -nosplash -nosound -abslog=`"$log`""
}
$process = Start-Process -FilePath $exe -ArgumentList $argLine -Wait -NoNewWindow -PassThru

if (-not (Test-Path $out)) { Write-Host "STLogBench FAILED: no output written (exit $($process.ExitCode); log: $log)"; exit 1 }

$result = Get-Content $out -Raw | ConvertFrom-Json
Write-Host "STLogBench: $($result.config) / $($result.platform), noLogging=$($result.noLogging)"
$result.results | ForEach-Object {
    "{0,-24} {1,8} iters  {2,10:N1} ns/call" -f $_.scenario, $_.iterations, $_.nsPerCall | Write-Host
}
$probe = $result.argumentEvaluation
Write-Host ("Argument evaluation: expected {0}, actual {1} - {2}" -f $probe.expected, $probe.actual, $(if ($probe.pass) { 'ok' } else { 'FAILED' }))
Write-Host "Full results: $out"
if (-not $probe.pass) { exit 1 }
