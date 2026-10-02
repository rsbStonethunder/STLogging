#pragma once

#include "CoreMinimal.h"

// Times ST_LOG against plain UE_LOG. Shared by the STLogging.Perf.* automation tests (which
// report the numbers as Unreal telemetry) and the host project's STLogBench commandlet (which
// also runs in Shipping, where automation tests can't).
namespace STLogging::Bench
{
struct FResult
{
	FString Group;
	FString Name;
	int32 Iterations = 0;
	double TotalSeconds = 0.0;

	double NsPerCall() const { return Iterations > 0 ? (TotalSeconds * 1.0e9) / Iterations : 0.0; }
};

// Scenario groups, in run order: Plain, Context, Nested, ContextOnly, FilteredLog, Controls.
STLOGGING_API const TArray<FString>& Groups();

// Runs the scenarios in Group, or every scenario when Group is empty. The logged scenarios
// write to the real sinks (console/log file), so both ST_LOG and UE_LOG pay the same I/O.
STLOGGING_API TArray<FResult> RunScenarios(const FString& Group = FString());

// "Development", "Shipping", ... for the running build.
STLOGGING_API FString BuildConfigName();
}
