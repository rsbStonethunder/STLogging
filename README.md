# STLogging

Context-aware logging for Unreal Engine 5.4+ (C++20).

Add `"STLogging"` to your module's `PublicDependencyModuleNames` and `#include "STLogging.h"`.

## Usage

```cpp
void AMyActor::Tick(float DeltaTime)
{
    ST_LOG_CONTEXT(Ctx, Health, Target);   // declare + add variables (1-16), all optional
    ST_LOG_ADD(Ctx, Speed);                // add more later
    ST_LOG_ADD_VALUE(Ctx, Doubled, Health * 2); // a computed value, no local needed

    ST_LOG(LogTemp, Log, "plain message");                     // [AMyActor::Tick] plain message
    ST_LOG(LogTemp, Log, "health is {Health}", Ctx);           // substitutes {Health}, dumps the rest
    ST_LOG(LogTemp, Log, "state: ", Ctx);                      // dumps everything: {Health: 5, ...}
}
```

- Every line is prefixed `[Class::Function]` (free functions: `[Function]`). Inside a lambda or an anonymous namespace, MSVC's noisy `__FUNCTION__` (e.g. `` `anonymous-namespace'::Foo::<lambda_1>::operator () ``) is cleaned up to the enclosing name (`Foo`); on other compilers, which spell these differently, the prefix is left as-is.
- `ST_LOG_ADD`/`ST_LOG_CONTEXT` keep **live references**: values are read when `ST_LOG` runs, so changes made after show up. Arguments must be named variables (lvalues); temporaries are a compile error. Do not log a context after its variables are out of scope.
- `ST_LOG_ADD_VALUE(Ctx, Key, Expr)` instead stores a **point-in-time copy** of `Expr` — for a computed value with no local to take a reference to. `Key` is a bare name, not a string. Unlike `ST_LOG_ADD`, it does not need an lvalue, and it does not track later changes (there's nothing live to track).
- `FSTLogContext` is not copyable or movable: declare and use it within one stack frame.
- Entries appear in the order they were added. Keys are case-insensitive.
- With a context the format string must be a literal, and malformed braces are a compile error. Use `{{` / `}}` for literal braces. Without a context, braces are printed as-is.
- Unknown `{Name}` renders `{Name:MISSING}`.
- `{Name:.3}` sets decimal places for that substitution: on `float`/`double`, and on each component of `FVector`, `FVector2D`, `FVector4`, `FRotator` and `FQuat`. Malformed specs (`{Name:3}`, `{Name:.}`, …) are a compile error, same as malformed braces. Precision applies only to that `{Name:.N}` token — the auto-dump of unreferenced entries always uses default formatting — and is ignored (the value renders normally) on a type that doesn't support it.

## Describing your own classes

```cpp
class UMyObject : public UObject, public ISTLoggable
{
    virtual TArray<FSTLogField> GetLogFields() const override
    {
        return {
            { TEXT("Count"), STLogging::Stringify(Count), {} },
            STLogging::MakeNestedField(TEXT("Child"), Child),   // nested; null-safe
        };
    }
};
```

Output: `Obj.Count: 3, Obj.Child.Name: Foo`. Null pointers render `null`; an object already being expanded (a cycle or parent back-link) renders `<cycle>`; nesting deeper than 8 renders `<max depth>`.
Dispatch is by static type: a variable declared as `UObject*` prints its name even if the object implements `ISTLoggable`; declare it as the concrete type to expand fields. Supported pointer types: `T*`, `TObjectPtr<T>`, `TWeakObjectPtr<T>`. Add `Stringify(const T&)` overloads for your own value types.

## How a value type gets stringified

For a value that isn't `ISTLoggable`, in order: an explicit `Stringify(const T&)` overload, then (for class types) a member `ToString()`, then a free `LexToString(const T&)`. Whichever is found first wins; a type with none of these fails to compile with a message naming all three options. `UENUM` enums print by name (via `UEnum::GetNameStringByValue`); other enums print their underlying integer.

## Shipping and `NO_LOGGING`

Shipping builds define `NO_LOGGING=1` by default (`Misc/Build.h`: `NO_LOGGING = !USE_LOGGING_IN_SHIPPING`). UE_LOG then compiles to nothing for every verbosity except Fatal, and its arguments are never evaluated. ST_LOG follows the same rule, gated on `NO_LOGGING` itself (not `UE_BUILD_SHIPPING`):

- `ST_LOG`, `ST_LOG_MSG`, `ST_LOG_CONTEXT`, `ST_LOG_ADD`, `ST_LOG_ADD_ONE` and `ST_LOG_ADD_VALUE` evaluate none of their arguments and build no context or string. `ST_LOG_CONTEXT` declares an empty `FSTNullLogContext` instead of an `FSTLogContext`, so code that declares a context and later passes it to `ST_LOG` still compiles. Don't call `FSTLogContext` methods on a macro-declared context directly if that code must also build under `NO_LOGGING`.
- Arguments are still named inside `sizeof`/`decltype` (never evaluated), so they still have to compile, still count as used (no unused-variable warnings), and `ST_LOG_ADD` still rejects temporaries. Format strings passed with a context are still validated at compile time.
- `Fatal` is still fatal and its message is still formatted, but since the context was never filled, `{Name}` tokens are not substituted: `ST_LOG(LogFoo, Fatal, "bad count={Count}", Ctx)` reports `[Func] bad count={Count}`. Same as UE_LOG, which in Shipping goes through `LowLevelFatalError`.

To keep logging in a Shipping build, set `bUseLoggingInShipping = true;` in the game's `*.Target.cs`. That changes engine-wide defines, so the target also needs a unique build environment (`BuildEnvironment = TargetBuildEnvironment.Unique;`). ST_LOG picks it up automatically because it keys off `NO_LOGGING`. Verbosity is then controlled at runtime as usual — per category in `DefaultEngine.ini`:

```ini
[Core.Log]
LogTemp=Verbose
LogMyGame=Warning
```

or on the command line: `-LogCmds="LogTemp Verbose, LogMyGame off"`. A category's compile-time maximum (the third argument of `DECLARE_LOG_CATEGORY_EXTERN`) still caps what either can enable.

## Testing

The plugin is tested inside a host project (see the host repo's README) with the plugin cloned into its `Plugins/` folder. Test code lives in the editor-only `STLoggingTests` module.

```
pwsh -File Scripts/Run-Tests.ps1      # Automation tests (STLogging.*)
pwsh -File Scripts/Run-Smoke.ps1      # real-UObject smoke commandlet
pwsh -File Scripts/Run-LogTest.ps1    # STLogTest commandlet: 15 scenarios covering every branch, real logs, checked output
pwsh -File Scripts/Run-Benchmark.ps1  # ST_LOG vs plain UE_LOG timing, Development editor
pwsh -File Scripts/Run-Benchmark.ps1 -Shipping  # same, from a packaged Shipping build (see below)
```

Set `UE_ENGINE_ROOT` to use an engine other than `C:\Program Files\Epic Games\UE_5.8`, or `PLUGIN_HOST` if the host project is not three levels above the scripts.

### Shipping benchmark

`-Shipping` builds the host `TestProject` game target in Shipping, cooks/stages/paks it, and runs the staged exe. By hand, that is:

```
Build.bat TestProjectEditor Win64 Development -Project=<TestProject.uproject>   # the cook runs the editor
Build.bat TestProject Win64 Shipping -Project=<TestProject.uproject>
RunUAT.bat BuildCookRun -project=<TestProject.uproject> -platform=Win64 -clientconfig=Shipping -cook -stage -pak -nocompile -nocompileeditor -stagingdirectory=<dir>
<dir>\Windows\TestProject\Binaries\Win64\TestProject-Win64-Shipping.exe -run=STLogBench -out=<results.json> -unattended -nullrhi
```

The targets are built separately rather than with `BuildCookRun -build` because Live Coding in *any* open editor on the same engine blocks UAT's editor build. A commandlet meant to run from a game exe must clear `IsEditor` (and `IsClient`/`IsServer`) under `!WITH_EDITOR`, as `USTLogBenchCommandlet` does: `UCommandlet` defaults `IsEditor` to true, which a game exe rejects with exit code 1 — silently in Shipping, where the error log is compiled out. Shipping compiles logging out, so the exe prints nothing: the JSON is the only output. Its `argumentEvaluation` block counts how many ST_LOG argument expressions (and `ISTLoggable` renders) actually ran — 5 with logging, 0 under `NO_LOGGING` — and the commandlet exits 2 if that's wrong. `-FatalProbe` makes it hit an `ST_LOG(..., Fatal, ...)` first, which must still stop the process (exit 3) in every config.

Results on an i7-10700K, UE 5.8 (ns per call; the logged scenarios include console/log-file I/O; a Shipping "0" is under 0.1 ns, timer noise on an empty loop):

| Scenario | Development | Shipping |
|---|---:|---:|
| Plain_STLOG / Plain_UELOG | 1172 / 1208 | 0 / 0 |
| Context1_STLOG / Context1_UELOG | 1971 / 1235 | 0 / 0 |
| Context4_STLOG / Context4_UELOG | 4380 / 1613 | 0 / 0 |
| Context16_STLOG / Context16_UELOG | 14659 / 2938 | 0 / 0 |
| Nested_STLOG | 4897 | 0 |
| ContextOnly_1 / _4 / _16 (context built, never logged) | 91 / 269 / 1392 | 0 / 0 / 0 |
| FilteredLog_4 (logged at a suppressed verbosity) | 267 | 0 |
| Control_Empty | 0 | 0 |
| Control_DirectContext_4 | 266 | 247 |

The two controls are what make the Shipping zeros meaningful. `Control_DirectContext_4` does the same work as `ST_LOG_CONTEXT(Ctx, A, B, C, D)` but calls `FSTLogContext` directly, so `NO_LOGGING` can't remove it: it stays at ~250 ns in Shipping, so the optimiser is not deleting the timing loop — the ST_LOG scenarios read 0 because the macros expanded to nothing, exactly like UE_LOG's. (Before ST_LOG compiled out fully, these scenarios measured ~1.0–1.6 ns in Shipping.)
