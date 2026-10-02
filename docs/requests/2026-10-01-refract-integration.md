# Feature request: gaps found integrating STLogging into Refract

**From:** Refract (`RefractCore`, UE 5.8). We are replacing its `UE_LOG` calls with `ST_LOG`.
**Status:** Open. Refract will pilot the plugin in one log category and won't roll it out project-wide until items 1-3 have landed and the performance numbers from item 3 have been reviewed.

Refract code that needs these items (for reference only; don't edit Refract from this repo):
- `GridSubsystem.cpp` and `GridManager.cpp` log `FVector2D` values.
- `GridTileInstancedMeshComponent.cpp` logs a scale as `(%.3f, %.3f)`.
- `BeamSimulationSubsystem::SolveBoard` logs from inside lambdas and runs every time the board topology changes.

---

## 1. Fall back to `ToString()` for UE types (required)

**Problem:** `BuildField` hits its `static_assert` for any type that has no `Stringify` overload, and that includes common engine structs like `FVector2D`, `FIntPoint`, `FIntVector`, `FLinearColor`, `FColor`, `FQuat`, `FBox`, `FGuid` and `FDateTime`. A consumer can't fix this from its own code. Two-phase lookup means an overload added after `STStringify.h` is only found through ADL, and ADL for `FVector2D` searches `UE::Math`, not the consumer's namespace.

**What we're asking for:**
- Add a `CHasToString` concept, satisfied when `{ V.ToString() } -> std::convertible_to<FString>`. Add a `BuildField` branch that uses it, placed after `CStringifiable` so an explicit `Stringify` overload still wins.
- Optionally also fall back to `LexToString(V)` before giving up.
- Optionally render `UENUM` types by name, via `StaticEnum<T>()` / `UEnum::GetValueAsString`, instead of as the underlying integer. Plain C++ enums keep printing the integer.
- Keep the `static_assert` for types that match none of these, and update its message to list the new options.
- Update the README's "Supported types" section.

**Tests:**
- Each struct listed above renders the same as its `ToString()`.
- An explicit `Stringify` overload takes priority over `ToString()`.
- A struct with neither still fails to compile. Document this as a manual check if a compile-failure test is impractical.

## 2. Precision and format specifiers in `{Name}` (required)

**Problem:** floats always go through `FString::SanitizeFloat`, so a call that used `%.3f` can't be converted without a change in output. Refract needs control over precision in its log output.

**Proposal** (please change it if you find something better, but document what you settle on):
- Syntax: `{Name:spec}`. The spec comes after the first `:`. Names never contain `:`, and they may contain `.` because member paths like `Tile.Index` are keys.
- Minimum spec: `.N`, meaning N decimal places. It applies to `float` and `double`, and to every component of float-based UE math types (`FVector`, `FVector2D`, `FRotator`, `FQuat`, `FLinearColor`). The spec has to reach the per-type rendering, not get applied to the finished string.
- Nice to have: width and zero-padding for integers (`{Index:03}`), and hex output (`{Id:x}`).
- The `consteval` check in `FSTLogFormat` must reject malformed specs at compile time, the same way it rejects bad braces today.
- Entries that end up in the dump (not referenced in the format string) keep the default rendering. Alternatively, add a way to set precision per entry when it's added. Your choice; document it.
- `{Name:MISSING}` still works when the key is absent.

**Tests:** cover each spec on each supported type, a spec on a type that doesn't support it (define the behaviour), escaped braces next to specs, and the compile-time rejection of malformed specs.

## 3. Built-in performance testing (required)

Refract won't roll STLogging out beyond its pilot category until we have these numbers. Please add a repeatable benchmark that runs from the scripts, either a `Scripts/Run-Perf.ps1` commandlet or an Automation test group `STLogging.Perf.*`. It should write machine-readable results (CSV or JSON in `Saved/`) so runs can be compared.

**Scenarios.** Each one compares against the equivalent hand-written `UE_LOG` / `FString::Printf` line, and reports ns per call as median and p95 over enough iterations to be stable:

| # | Scenario | Why it matters |
|---|---|---|
| a | Plain message, no context | Baseline cost of the macro and its prefix |
| b | Context with 1, 4 and 16 entries, all substituted | Substitution cost as the context grows |
| c | Context with 1, 4 and 16 entries, all dumped | Dump cost |
| d | Nested `ISTLoggable` at depths 1, 4 and 8 | Tree building and the nesting guard |
| e | **Category suppressed** (verbosity below the threshold) with the context still built | The hot-path case: we declare contexts in functions that usually don't log. `FSTLogContext::Add` builds an `FName` from a string (a name-table lookup) and appends to a `TArray` on every call, even when nothing gets logged |
| f | Context declared and `ST_LOG` never reached (a branch not taken) | Same concern as (e), for a context declared at the top of a function |
| g | Using the format specifiers from item 2 | Make sure the feature doesn't regress (b) |

**Also report:** heap allocations per call, if a counter is cheap to add, and the results in both Development and Shipping builds.

**Follow-ups we expect the numbers to justify (make the changes if they do):**
- Cache the key `FName`s per call site, for example with a function-local `static const FName` generated by the macro, or by storing `const TCHAR*` keys and comparing them case-insensitively.
- Give `FSTLogContext` inline storage (`TInlineAllocator<N>`) so small contexts don't allocate.
- Make `ST_LOG_CONTEXT` / `ST_LOG_ADD` compile to no-ops when `NO_LOGGING` is set. Today `UE_LOG` disappears in Shipping, but our contexts are still built.

**Deliverable:** the script, a results table in the README (machine, configuration, numbers), and a short note on any overhead budget you recommend, e.g. "suppressed path within X ns of `UE_LOG`."

## 4. Clean `[Class::Function]` prefixes in lambdas and anonymous namespaces (nice to have)

On MSVC, `__FUNCTION__` inside a lambda produces `UBeamSimulationSubsystem::SolveBoard::<lambda_1>::operator ()`, and inside an anonymous namespace it produces ``` `anonymous-namespace'::Foo ```. Please strip these down to `Class::Function`, or to `Function` for an anonymous-namespace free function, possibly marking the lambda case as `SolveBoard (lambda)`. Do this with compile-time string processing so it costs nothing at runtime, and add tests for both cases.

## 5. Add computed values to a context (nice to have)

Today every context argument must be a named variable, so call sites end up with `const FString Name = GetName();` just to log a value. A variant that stores a copy of the value would let callers pass expressions directly, e.g. `ST_LOG_ADD_VALUE(Ctx, Name, GetName())`, with an explicit key since the expression's text makes a poor key. Make the semantics clear in the docs: a copied value is captured when it's added, unlike a live reference, which is read when the log runs.
