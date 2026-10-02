#include "STLoggingTestFixtures.h"
#include "STLogContext.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace STLoggingTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextLiveScalarTest, "STLogging.Context.LiveScalar", ST_TEST_FLAGS)
bool FSTContextLiveScalarTest::RunTest(const FString& Parameters)
{
	int32 Count = 1;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("Count"), Count);
	TestEqual(TEXT("at add time"), Ctx.RenderInline(TEXT("Count")).Get(TEXT("<unset>")), FString(TEXT("1")));

	Count = 42;
	TestEqual(TEXT("after mutation"), Ctx.RenderInline(TEXT("Count")).Get(TEXT("<unset>")), FString(TEXT("42")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextLiveHierarchyTest, "STLogging.Context.LiveHierarchy", ST_TEST_FLAGS)
bool FSTContextLiveHierarchyTest::RunTest(const FString& Parameters)
{
	FLeaf Leaf;
	Leaf.Count = 7;
	Leaf.Label = TEXT("L");
	FMid Mid;
	Mid.Level = 2;
	Mid.Child = &Leaf;
	FRoot Root;
	Root.Name = TEXT("R");
	Root.Mid = &Mid;

	FSTLogContext Ctx;
	Ctx.Add(TEXT("Root"), Root);
	TestTrue(TEXT("before"), Ctx.BuildDump().Contains(TEXT("Root.Mid.Child.Count: 7")));

	Leaf.Count = 8;
	Mid.Level = 3;
	const FString After = Ctx.BuildDump();
	TestTrue(TEXT("leaf change seen"), After.Contains(TEXT("Root.Mid.Child.Count: 8")));
	TestTrue(TEXT("mid change seen"), After.Contains(TEXT("Root.Mid.Level: 3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextNullFlipTest, "STLogging.Context.PointerBecomesNull", ST_TEST_FLAGS)
bool FSTContextNullFlipTest::RunTest(const FString& Parameters)
{
	FLeaf Leaf;
	Leaf.Count = 7;
	Leaf.Label = TEXT("L");
	const FLeaf* Ptr = &Leaf;

	FSTLogContext Ctx;
	Ctx.Add(TEXT("Ptr"), Ptr);
	TestEqual(TEXT("non-null"), Ctx.RenderInline(TEXT("Ptr")).Get(TEXT("<unset>")), FString(TEXT("{Ptr.Count: 7, Ptr.Label: L}")));

	Ptr = nullptr;
	TestEqual(TEXT("null after flip"), Ctx.RenderInline(TEXT("Ptr")).Get(TEXT("<unset>")), FString(TEXT("null")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextOrderTest, "STLogging.Context.InsertionOrder", ST_TEST_FLAGS)
bool FSTContextOrderTest::RunTest(const FString& Parameters)
{
	int32 C = 1;
	int32 A = 2;
	int32 B = 3;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("C"), C);
	Ctx.Add(TEXT("A"), A);
	Ctx.Add(TEXT("B"), B);
	TestEqual(TEXT("num"), Ctx.Num(), 3);
	TestEqual(TEXT("order"), Ctx.BuildDump(), FString(TEXT("C: 1, A: 2, B: 3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextRebindTest, "STLogging.Context.ReAddKeepsPosition", ST_TEST_FLAGS)
bool FSTContextRebindTest::RunTest(const FString& Parameters)
{
	int32 First = 1;
	int32 Second = 2;
	int32 Replacement = 99;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("A"), First);
	Ctx.Add(TEXT("B"), Second);
	Ctx.Add(TEXT("A"), Replacement);
	TestEqual(TEXT("num"), Ctx.Num(), 2);
	TestEqual(TEXT("rebound in place"), Ctx.BuildDump(), FString(TEXT("A: 99, B: 2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextLookupTest, "STLogging.Context.LookupAndExclude", ST_TEST_FLAGS)
bool FSTContextLookupTest::RunTest(const FString& Parameters)
{
	int32 Count = 5;
	int32 Other = 6;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("Count"), Count);
	Ctx.Add(TEXT("Other"), Other);

	TestFalse(TEXT("missing key"), Ctx.RenderInline(TEXT("Nope")).IsSet());
	TestEqual(TEXT("case-insensitive"), Ctx.RenderInline(TEXT("count")).Get(TEXT("<unset>")), FString(TEXT("5")));

	TSet<FName> Exclude;
	Exclude.Add(FName(TEXT("Count")));
	TestEqual(TEXT("exclude"), Ctx.BuildDump(Exclude), FString(TEXT("Other: 6")));

	FSTLogContext Empty;
	TestEqual(TEXT("empty num"), Empty.Num(), 0);
	TestEqual(TEXT("empty dump"), Empty.BuildDump(), FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextPrecisionTest, "STLogging.Context.Precision", ST_TEST_FLAGS)
bool FSTContextPrecisionTest::RunTest(const FString& Parameters)
{
	float Speed = 12.3456f;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("Speed"), Speed);

	TestEqual(TEXT("with precision"), Ctx.RenderInline(TEXT("Speed"), 2).Get(TEXT("<unset>")), FString(TEXT("12.35")));
	TestEqual(TEXT("without precision unaffected"), Ctx.RenderInline(TEXT("Speed")).Get(TEXT("<unset>")), STLogging::Stringify(Speed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTContextAddValueTest, "STLogging.Context.AddValue", ST_TEST_FLAGS)
bool FSTContextAddValueTest::RunTest(const FString& Parameters)
{
	int32 Health = 5;
	FSTLogContext Ctx;
	Ctx.AddValue(TEXT("Doubled"), Health * 2);
	TestEqual(TEXT("computed rvalue"), Ctx.RenderInline(TEXT("Doubled")).Get(TEXT("<unset>")), FString(TEXT("10")));

	Ctx.AddValue(TEXT("Snapshot"), Health);
	Health = 99;
	TestEqual(TEXT("lvalue is copied, not a live reference"), Ctx.RenderInline(TEXT("Snapshot")).Get(TEXT("<unset>")), FString(TEXT("5")));

	Ctx.AddValue(TEXT("Snapshot"), Health);
	TestEqual(TEXT("re-add rebinds the key"), Ctx.RenderInline(TEXT("Snapshot")).Get(TEXT("<unset>")), FString(TEXT("99")));
	TestEqual(TEXT("re-add does not duplicate the entry"), Ctx.Num(), 2);
	return true;
}

#endif
