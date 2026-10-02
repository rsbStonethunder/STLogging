#include "Misc/AutomationTest.h"
#include "STLogBenchmark.h"

#if WITH_DEV_AUTOMATION_TESTS

// One test per scenario group (STLogging.Perf.Plain, .Context, ...). Each reports ns per call
// as Unreal telemetry, which the automation controller appends to
// Saved/Automation/Telemetry/STLoggingPerf.csv, one data point per scenario.
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FSTLogPerfTest, "STLogging.Perf",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

void FSTLogPerfTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const FString& Group : STLogging::Bench::Groups())
	{
		OutBeautifiedNames.Add(Group);
		OutTestCommands.Add(Group);
	}
}

bool FSTLogPerfTest::RunTest(const FString& Parameters)
{
	const TArray<STLogging::Bench::FResult> Results = STLogging::Bench::RunScenarios(Parameters);
	if (Results.IsEmpty())
	{
		AddError(FString::Printf(TEXT("No benchmark scenarios in group '%s'"), *Parameters));
		return false;
	}

	// Unreal treats anything after a dot in the storage name as the file extension.
	SetTelemetryStorage(TEXT("STLoggingPerf"));
	for (const STLogging::Bench::FResult& Result : Results)
	{
		// Control_Empty measures ~0 ns, so timer noise alone would look like a large regression.
		if (Result.Name != TEXT("Control_Empty"))
		{
			AddTelemetryData(Result.Name + TEXT(".NsPerCall"), Result.NsPerCall());
		}
		AddInfo(FString::Printf(TEXT("%-24s %8d iters  %10.1f ns/call"), *Result.Name, Result.Iterations, Result.NsPerCall()));
	}

	if (Parameters == TEXT("Controls"))
	{
		// If the direct context costs no more than an empty loop, the optimiser deleted the timing
		// loop and none of the numbers mean anything.
		const STLogging::Bench::FResult* Empty = Results.FindByPredicate([](const STLogging::Bench::FResult& R) { return R.Name == TEXT("Control_Empty"); });
		const STLogging::Bench::FResult* Direct = Results.FindByPredicate([](const STLogging::Bench::FResult& R) { return R.Name == TEXT("Control_DirectContext_4"); });
		if (TestNotNull(TEXT("Control_Empty ran"), Empty) && TestNotNull(TEXT("Control_DirectContext_4 ran"), Direct))
		{
			TestTrue(TEXT("Control_DirectContext_4 costs more than an empty loop"), Direct->NsPerCall() > Empty->NsPerCall());
		}
	}
	return true;
}

#endif
