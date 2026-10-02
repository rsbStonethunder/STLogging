#include "STLogBenchmark.h"
#include "STLogMacros.h"
#include "STLogContext.h"
#include "STStringify.h"
#include "STLogTypes.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogSTBench, Log, All);

namespace
{
struct FBenchInner : public ISTLoggable
{
	int32 X = 0;
	int32 Y = 0;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("X"), STLogging::Stringify(X), {} },
			{ TEXT("Y"), STLogging::Stringify(Y), {} },
		};
	}
};

struct FBenchOuter : public ISTLoggable
{
	FString Tag;
	FBenchInner Inner;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Tag"), Tag, {} },
			STLogging::MakeNestedField(TEXT("Inner"), &Inner),
		};
	}
};

constexpr int32 LoggedIterations = 2000;
constexpr int32 UnloggedIterations = 100000;

// Runs Func Iterations times (after a short warm-up) and times the measured loop only.
template <typename FuncType>
STLogging::Bench::FResult RunBench(const TCHAR* Group, const TCHAR* Name, int32 Iterations, FuncType&& Func)
{
	constexpr int32 WarmupIterations = 200;
	for (int32 i = 0; i < WarmupIterations; ++i)
	{
		Func();
	}

	const double Start = FPlatformTime::Seconds();
	for (int32 i = 0; i < Iterations; ++i)
	{
		Func();
	}
	const double Elapsed = FPlatformTime::Seconds() - Start;
	return STLogging::Bench::FResult{ FString(Group), FString(Name), Iterations, Elapsed };
}

void RunPlain(TArray<STLogging::Bench::FResult>& Results)
{
	Results.Add(RunBench(TEXT("Plain"), TEXT("Plain_STLOG"), LoggedIterations, []()
	{
		ST_LOG(LogSTBench, Log, "plain message");
	}));
	Results.Add(RunBench(TEXT("Plain"), TEXT("Plain_UELOG"), LoggedIterations, []()
	{
		UE_LOG(LogSTBench, Log, TEXT("[%s] plain message"), ANSI_TO_TCHAR(__FUNCTION__));
	}));
}

void RunContext(TArray<STLogging::Bench::FResult>& Results)
{
	int32 A = 1, B = 2, C = 3, D = 4;
	Results.Add(RunBench(TEXT("Context"), TEXT("Context1_STLOG"), LoggedIterations, [&A]()
	{
		ST_LOG_CONTEXT(Ctx, A);
		ST_LOG(LogSTBench, Log, "v={A}", Ctx);
	}));
	Results.Add(RunBench(TEXT("Context"), TEXT("Context1_UELOG"), LoggedIterations, [&A]()
	{
		UE_LOG(LogSTBench, Log, TEXT("[%s] v=%d"), ANSI_TO_TCHAR(__FUNCTION__), A);
	}));

	Results.Add(RunBench(TEXT("Context"), TEXT("Context4_STLOG"), LoggedIterations, [&A, &B, &C, &D]()
	{
		ST_LOG_CONTEXT(Ctx, A, B, C, D);
		ST_LOG(LogSTBench, Log, "v={A}", Ctx);
	}));
	Results.Add(RunBench(TEXT("Context"), TEXT("Context4_UELOG"), LoggedIterations, [&A, &B, &C, &D]()
	{
		UE_LOG(LogSTBench, Log, TEXT("[%s] v=%d {A: %d, B: %d, C: %d, D: %d}"), ANSI_TO_TCHAR(__FUNCTION__), A, A, B, C, D);
	}));

	int32 V[16];
	for (int32 i = 0; i < 16; ++i)
	{
		V[i] = i;
	}
	Results.Add(RunBench(TEXT("Context"), TEXT("Context16_STLOG"), LoggedIterations, [&V]()
	{
		ST_LOG_CONTEXT(Ctx, V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7], V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]);
		ST_LOG(LogSTBench, Log, "state: ", Ctx);
	}));
	Results.Add(RunBench(TEXT("Context"), TEXT("Context16_UELOG"), LoggedIterations, [&V]()
	{
		UE_LOG(LogSTBench, Log,
			TEXT("[%s] state: {V0: %d, V1: %d, V2: %d, V3: %d, V4: %d, V5: %d, V6: %d, V7: %d, ")
			TEXT("V8: %d, V9: %d, V10: %d, V11: %d, V12: %d, V13: %d, V14: %d, V15: %d}"),
			ANSI_TO_TCHAR(__FUNCTION__),
			V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7], V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]);
	}));
}

// A nested ISTLoggable object has no natural plain-UE_LOG equivalent.
void RunNested(TArray<STLogging::Bench::FResult>& Results)
{
	FBenchOuter Outer;
	Outer.Tag = TEXT("t");
	Outer.Inner.X = 3;
	Outer.Inner.Y = 4;
	Results.Add(RunBench(TEXT("Nested"), TEXT("Nested_STLOG"), LoggedIterations, [&Outer]()
	{
		ST_LOG_CONTEXT(Ctx, Outer);
		ST_LOG(LogSTBench, Log, "dump: ", Ctx);
	}));
}

// Hot path: a context built and never logged at all, isolating the cost of Add().
void RunContextOnly(TArray<STLogging::Bench::FResult>& Results)
{
	int32 A = 1, B = 2, C = 3, D = 4;
	int32 V[16];
	for (int32 i = 0; i < 16; ++i)
	{
		V[i] = i;
	}
	Results.Add(RunBench(TEXT("ContextOnly"), TEXT("ContextOnly_1"), UnloggedIterations, [&A]()
	{
		ST_LOG_CONTEXT(Ctx, A);
	}));
	Results.Add(RunBench(TEXT("ContextOnly"), TEXT("ContextOnly_4"), UnloggedIterations, [&A, &B, &C, &D]()
	{
		ST_LOG_CONTEXT(Ctx, A, B, C, D);
	}));
	Results.Add(RunBench(TEXT("ContextOnly"), TEXT("ContextOnly_16"), UnloggedIterations, [&V]()
	{
		ST_LOG_CONTEXT(Ctx, V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7], V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]);
	}));
}

// Hot path: a context built and a log call made, but the category filters it out.
void RunFilteredLog(TArray<STLogging::Bench::FResult>& Results)
{
	int32 A = 1, B = 2, C = 3, D = 4;
	UE_SET_LOG_VERBOSITY(LogSTBench, Warning); // suppresses the Log-verbosity call below
	Results.Add(RunBench(TEXT("FilteredLog"), TEXT("FilteredLog_4"), UnloggedIterations, [&A, &B, &C, &D]()
	{
		ST_LOG_CONTEXT(Ctx, A, B, C, D);
		ST_LOG(LogSTBench, Log, "v={A}", Ctx);
	}));
	UE_SET_LOG_VERBOSITY(LogSTBench, Log);
}

// Controls prove the timing loop survives optimisation. A Shipping 0 ns for the ST_LOG
// scenarios should mean the macros compiled out, not that the optimiser deleted the loop.
// Control_Empty is the floor; Control_DirectContext_4 does what ST_LOG_CONTEXT(Ctx, A, B, C, D)
// does with logging on, but calls FSTLogContext directly so NO_LOGGING can't remove it - it
// must stay well above 0 in every config.
void RunControls(TArray<STLogging::Bench::FResult>& Results)
{
	int32 A = 1, B = 2, C = 3, D = 4;
	Results.Add(RunBench(TEXT("Controls"), TEXT("Control_Empty"), UnloggedIterations, []()
	{
	}));
	Results.Add(RunBench(TEXT("Controls"), TEXT("Control_DirectContext_4"), UnloggedIterations, [&A, &B, &C, &D]()
	{
		FSTLogContext Ctx;
		Ctx.Add(TEXT("A"), A);
		Ctx.Add(TEXT("B"), B);
		Ctx.Add(TEXT("C"), C);
		Ctx.Add(TEXT("D"), D);
	}));
}

using FGroupRunner = void (*)(TArray<STLogging::Bench::FResult>&);

const TArray<TPair<FString, FGroupRunner>>& Runners()
{
	static const TArray<TPair<FString, FGroupRunner>> Table = {
		{ TEXT("Plain"), &RunPlain },
		{ TEXT("Context"), &RunContext },
		{ TEXT("Nested"), &RunNested },
		{ TEXT("ContextOnly"), &RunContextOnly },
		{ TEXT("FilteredLog"), &RunFilteredLog },
		{ TEXT("Controls"), &RunControls },
	};
	return Table;
}
}

namespace STLogging::Bench
{
const TArray<FString>& Groups()
{
	static const TArray<FString> Names = []()
	{
		TArray<FString> Result;
		for (const TPair<FString, FGroupRunner>& Runner : Runners())
		{
			Result.Add(Runner.Key);
		}
		return Result;
	}();
	return Names;
}

TArray<FResult> RunScenarios(const FString& Group)
{
	TArray<FResult> Results;
	for (const TPair<FString, FGroupRunner>& Runner : Runners())
	{
		if (!Group.IsEmpty() && Runner.Key != Group)
		{
			continue;
		}
		Runner.Value(Results);
	}
	return Results;
}

FString BuildConfigName()
{
#if UE_BUILD_SHIPPING
	return TEXT("Shipping");
#elif UE_BUILD_TEST
	return TEXT("Test");
#elif UE_BUILD_DEBUG
	return TEXT("Debug");
#else
	return TEXT("Development");
#endif
}
}
