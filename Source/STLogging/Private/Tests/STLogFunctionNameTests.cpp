#include "STLoggingTestFixtures.h"
#include "STLogFunctionName.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTCleanFunctionNameTest, "STLogging.FunctionName.Clean", ST_TEST_FLAGS)
bool FSTCleanFunctionNameTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("plain free function"), STLogging::CleanFunctionName(TEXT("Foo")), FString(TEXT("Foo")));
	TestEqual(TEXT("plain class::method"), STLogging::CleanFunctionName(TEXT("AMyActor::Tick")), FString(TEXT("AMyActor::Tick")));

	TestEqual(TEXT("anonymous namespace, free function"),
		STLogging::CleanFunctionName(TEXT("`anonymous-namespace'::Foo")), FString(TEXT("Foo")));
	TestEqual(TEXT("anonymous namespace, class::method"),
		STLogging::CleanFunctionName(TEXT("`anonymous-namespace'::FFoo::Method")), FString(TEXT("FFoo::Method")));

	TestEqual(TEXT("lambda, free function"),
		STLogging::CleanFunctionName(TEXT("Foo::<lambda_1>::operator ()")), FString(TEXT("Foo")));
	TestEqual(TEXT("lambda, class::method"),
		STLogging::CleanFunctionName(TEXT("AMyActor::Tick::<lambda_1>::operator ()")), FString(TEXT("AMyActor::Tick")));
	TestEqual(TEXT("second lambda in the same function"),
		STLogging::CleanFunctionName(TEXT("Foo::<lambda_2>::operator ()")), FString(TEXT("Foo")));

	TestEqual(TEXT("anonymous namespace and lambda together"),
		STLogging::CleanFunctionName(TEXT("`anonymous-namespace'::Foo::<lambda_1>::operator ()")), FString(TEXT("Foo")));
	return true;
}

#endif
