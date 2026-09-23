# STLogging

Context-aware logging for Unreal Engine 5.4+ (C++20).

Add `"STLogging"` to your module's `PublicDependencyModuleNames` and `#include "STLogging.h"`.

## Usage

```cpp
void AMyActor::Tick(float DeltaTime)
{
    ST_LOG_CONTEXT(Ctx, Health, Target);   // declare + add variables (1-16), all optional
    ST_LOG_ADD(Ctx, Speed);                // add more later

    ST_LOG(LogTemp, Log, "plain message");                     // [AMyActor::Tick] plain message
    ST_LOG(LogTemp, Log, "health is {Health}", Ctx);           // substitutes {Health}, dumps the rest
    ST_LOG(LogTemp, Log, "state: ", Ctx);                      // dumps everything: {Health: 5, ...}
}
```

- Every line is prefixed `[Class::Function]` (free functions: `[Function]`).
- The context keeps **live references**: values are read when `ST_LOG` runs, so changes made after `ST_LOG_ADD` show up. Arguments must be named variables (lvalues); temporaries are a compile error. Do not log a context after its variables are out of scope.
- Entries appear in the order they were added. Keys are case-insensitive.
- With a context the format string must be a literal, and malformed braces are a compile error. Use `{{` / `}}` for literal braces. Without a context, braces are printed as-is.
- Unknown `{Name}` renders `{Name:MISSING}`.

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

Output: `Obj.Count: 3, Obj.Child.Name: Foo`. Null pointers render `null`; cycles stop at depth 8 with `<max depth>`.
Dispatch is by static type: a variable declared as `UObject*` prints its name even if the object implements `ISTLoggable`; declare it as the concrete type to expand fields. Supported pointer types: `T*`, `TObjectPtr<T>`, `TWeakObjectPtr<T>`. Add `Stringify(const T&)` overloads for your own value types.

## Testing

```
pwsh -File Scripts/Run-STLoggingTests.ps1             # Automation tests (STLogging.*)
pwsh -File Scripts/Run-Smoke.ps1                      # real-UObject smoke commandlet
```

Set `UE_ENGINE_ROOT` to use an engine other than `C:\Program Files\Epic Games\UE_5.8`.
