#include "STLoggingTestFixtures.h"
#include "STLogging.h"
#include "Misc/OutputDevice.h"

#if WITH_DEV_AUTOMATION_TESTS

DEFINE_LOG_CATEGORY_STATIC(LogSTLoggingTest, Log, All);

namespace STLoggingTests
{
class FLogCapture : public FOutputDevice
{
public:
	FLogCapture() { GLog->AddOutputDevice(this); }
	virtual ~FLogCapture() override { GLog->RemoveOutputDevice(this); }

	virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (Category != LogSTLoggingTest.GetCategoryName())
		{
			return;
		}
		Lines.Add(Message);
	}

	// The redirector hands lines to non-thread-safe devices lazily; flush before reading.
	const TArray<FString>& GetLines()
	{
		GLog->FlushThreadedLogs();
		return Lines;
	}

private:
	TArray<FString> Lines;
};

struct FMacroFixture
{
	int32 Count = 3;

	void InstanceMethod() { ST_LOG(LogSTLoggingTest, Log, "instance"); }
	static void StaticMethod() { ST_LOG(LogSTLoggingTest, Log, "static"); }
};

void FreeFunction()
{
	ST_LOG(LogSTLoggingTest, Log, "free");
}
}

namespace
{
void AnonNamespaceFunction()
{
	ST_LOG(LogSTLoggingTest, Log, "anon-ns");
}

struct FAnonClass
{
	void Method() { ST_LOG(LogSTLoggingTest, Log, "anon-ns-method"); }
};

void AnonNamespaceFunctionWithLambda()
{
	auto L = []() { ST_LOG(LogSTLoggingTest, Log, "anon-ns-lambda"); };
	L();
}
}

void STLoggingTests_TopLevelFunctionWithLambda()
{
	auto L = []() { ST_LOG(LogSTLoggingTest, Log, "toplevel-lambda"); };
	L();
}

using namespace STLoggingTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroPrefixTest, "STLogging.Macros.FunctionPrefix", ST_TEST_FLAGS)
bool FSTMacroPrefixTest::RunTest(const FString& Parameters)
{
	FLogCapture Capture;
	FMacroFixture Fixture;
	Fixture.InstanceMethod();
	FMacroFixture::StaticMethod();
	FreeFunction();

	if (!TestEqual(TEXT("line count"), Capture.GetLines().Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("instance"), Capture.GetLines()[0].StartsWith(TEXT("[")) && Capture.GetLines()[0].Contains(TEXT("FMacroFixture::InstanceMethod")) && Capture.GetLines()[0].EndsWith(TEXT("] instance")));
	TestTrue(TEXT("static"), Capture.GetLines()[1].Contains(TEXT("FMacroFixture::StaticMethod")) && Capture.GetLines()[1].EndsWith(TEXT("] static")));
	TestTrue(TEXT("free function has no class"), Capture.GetLines()[2].Contains(TEXT("FreeFunction")) && !Capture.GetLines()[2].Contains(TEXT("FMacroFixture")) && Capture.GetLines()[2].EndsWith(TEXT("] free")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroCleanPrefixTest, "STLogging.Macros.CleanFunctionNames", ST_TEST_FLAGS)
bool FSTMacroCleanPrefixTest::RunTest(const FString& Parameters)
{
	FLogCapture Capture;

	auto LocalLambda = []() { ST_LOG(LogSTLoggingTest, Log, "local-lambda"); };
	LocalLambda();
	AnonNamespaceFunction();
	FAnonClass{}.Method();
	AnonNamespaceFunctionWithLambda();
	STLoggingTests_TopLevelFunctionWithLambda();

	if (!TestEqual(TEXT("line count"), Capture.GetLines().Num(), 5))
	{
		return false;
	}
	TestTrue(TEXT("lambda collapses to enclosing function"),
		Capture.GetLines()[0].StartsWith(TEXT("[FSTMacroCleanPrefixTest::RunTest]")));
	TestTrue(TEXT("anon namespace prefix stripped from a free function"),
		Capture.GetLines()[1].StartsWith(TEXT("[AnonNamespaceFunction]")));
	TestTrue(TEXT("anon namespace prefix stripped, class::method kept"),
		Capture.GetLines()[2].StartsWith(TEXT("[FAnonClass::Method]")));
	TestTrue(TEXT("anon namespace + lambda both stripped"),
		Capture.GetLines()[3].StartsWith(TEXT("[AnonNamespaceFunctionWithLambda]")));
	TestTrue(TEXT("lambda stripped, top-level function kept"),
		Capture.GetLines()[4].StartsWith(TEXT("[STLoggingTests_TopLevelFunctionWithLambda]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroPlainBracesTest, "STLogging.Macros.PlainMessageKeepsBraces", ST_TEST_FLAGS)
bool FSTMacroPlainBracesTest::RunTest(const FString& Parameters)
{
	FLogCapture Capture;
	ST_LOG(LogSTLoggingTest, Log, "json {\"a\":1} {x} 100%");

	if (!TestEqual(TEXT("line count"), Capture.GetLines().Num(), 1))
	{
		return false;
	}
	TestTrue(TEXT("verbatim"), Capture.GetLines()[0].EndsWith(TEXT("] json {\"a\":1} {x} 100%")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroContextTest, "STLogging.Macros.ContextInlineAndDump", ST_TEST_FLAGS)
bool FSTMacroContextTest::RunTest(const FString& Parameters)
{
	FLogCapture Capture;
	int32 Count = 3;
	FString Name = TEXT("Foo");
	ST_LOG_CONTEXT(Ctx, Count, Name);

	ST_LOG(LogSTLoggingTest, Log, "count={Count}", Ctx);
	Count = 10;
	ST_LOG(LogSTLoggingTest, Log, "count={Count}", Ctx);
	ST_LOG(LogSTLoggingTest, Log, "state ", Ctx);

	if (!TestEqual(TEXT("line count"), Capture.GetLines().Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("inline plus dump of the rest"), Capture.GetLines()[0].EndsWith(TEXT("] count=3 {Name: Foo}")));
	TestTrue(TEXT("live value after mutation"), Capture.GetLines()[1].EndsWith(TEXT("] count=10 {Name: Foo}")));
	TestTrue(TEXT("full dump when nothing referenced"), Capture.GetLines()[2].EndsWith(TEXT("] state {Count: 10, Name: Foo}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroHierarchyNullTest, "STLogging.Macros.HierarchyWithNulls", ST_TEST_FLAGS)
bool FSTMacroHierarchyNullTest::RunTest(const FString& Parameters)
{
	FLogCapture Capture;
	FRoot Root;
	Root.Name = TEXT("R");
	ST_LOG_CONTEXT(Ctx, Root);

	ST_LOG(LogSTLoggingTest, Log, "root", Ctx);

	FMid Mid;
	Mid.Level = 2;
	Root.Mid = &Mid;
	ST_LOG(LogSTLoggingTest, Log, "root", Ctx);

	if (!TestEqual(TEXT("line count"), Capture.GetLines().Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("null mid"), Capture.GetLines()[0].EndsWith(TEXT("] root {Root.Name: R, Root.Mid: null}")));
	TestTrue(TEXT("mid appears once set, child null"), Capture.GetLines()[1].EndsWith(TEXT("] root {Root.Name: R, Root.Mid.Level: 2, Root.Mid.Child: null}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroAddTest, "STLogging.Macros.AddArities", ST_TEST_FLAGS)
bool FSTMacroAddTest::RunTest(const FString& Parameters)
{
	int32 v1 = 1, v2 = 2, v3 = 3, v4 = 4, v5 = 5, v6 = 6, v7 = 7, v8 = 8;
	int32 v9 = 9, v10 = 10, v11 = 11, v12 = 12, v13 = 13, v14 = 14, v15 = 15, v16 = 16;

	FSTLogContext One;
	ST_LOG_ADD(One, v1);
	TestEqual(TEXT("one"), One.BuildDump(), FString(TEXT("v1: 1")));

	FSTLogContext Two;
	ST_LOG_ADD(Two, v1, v2);
	TestEqual(TEXT("two"), Two.BuildDump(), FString(TEXT("v1: 1, v2: 2")));

	FSTLogContext Sixteen;
	ST_LOG_ADD(Sixteen, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16);
	TestEqual(TEXT("sixteen count"), Sixteen.Num(), 16);
	TestEqual(TEXT("sixteen order"), Sixteen.BuildDump(),
		FString(TEXT("v1: 1, v2: 2, v3: 3, v4: 4, v5: 5, v6: 6, v7: 7, v8: 8, v9: 9, v10: 10, v11: 11, v12: 12, v13: 13, v14: 14, v15: 15, v16: 16")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroAddValueTest, "STLogging.Macros.AddValue", ST_TEST_FLAGS)
bool FSTMacroAddValueTest::RunTest(const FString& Parameters)
{
	int32 Health = 5;
	ST_LOG_CONTEXT(Ctx, Health);
	ST_LOG_ADD_VALUE(Ctx, Doubled, Health * 2);
	TestEqual(TEXT("computed value alongside a live one"), Ctx.BuildDump(), FString(TEXT("Health: 5, Doubled: 10")));

	Health = 7;
	TestEqual(TEXT("Health stays live, Doubled stays a snapshot"), Ctx.BuildDump(), FString(TEXT("Health: 7, Doubled: 10")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTMacroContextDeclTest, "STLogging.Macros.ContextDeclaration", ST_TEST_FLAGS)
bool FSTMacroContextDeclTest::RunTest(const FString& Parameters)
{
	int32 A = 1;
	int32 B = 2;
	ST_LOG_CONTEXT(Empty);
	ST_LOG_CONTEXT(WithValues, A, B);
	TestEqual(TEXT("empty declaration"), Empty.Num(), 0);
	TestEqual(TEXT("inline values"), WithValues.BuildDump(), FString(TEXT("A: 1, B: 2")));

	FMacroFixture Holder;
	ST_LOG_ADD(Empty, Holder.Count);
	TestEqual(TEXT("expression key"), Empty.RenderInline(TEXT("Holder.Count")).Get(TEXT("<unset>")), FString(TEXT("3")));
	return true;
}

#endif
