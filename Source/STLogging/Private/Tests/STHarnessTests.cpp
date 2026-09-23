#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTHarnessTest, "STLogging.Harness.Loads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSTHarnessTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("harness runs"), true);
	return true;
}

#endif
