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

## Testing

The plugin is tested inside a host project (see the host repo's README) with the plugin cloned into its `Plugins/` folder. Test code lives in the editor-only `STLoggingTests` module.

```
pwsh -File Scripts/Run-Tests.ps1      # Automation tests (STLogging.*)
pwsh -File Scripts/Run-Smoke.ps1      # real-UObject smoke commandlet
pwsh -File Scripts/Run-LogTest.ps1    # STLogTest commandlet: 15 scenarios covering every branch, real logs, checked output
pwsh -File Scripts/Run-Benchmark.ps1  # ST_LOG vs plain UE_LOG timing (Development only - see note below)
```

`Run-Benchmark.ps1` currently only measures the Development editor. A true Shipping-config measurement needs the host `TestProject` packaged as a standalone Game target, which is blocked: the packaged `.exe` builds and cooks fine but silently exits right after `LogMemory`'s startup stats with no further log, crash dialog, or Windows Event Log entry, across every launch variant tried. Still open.

Set `UE_ENGINE_ROOT` to use an engine other than `C:\Program Files\Epic Games\UE_5.8`, or `PLUGIN_HOST` if the host project is not three levels above the scripts.
