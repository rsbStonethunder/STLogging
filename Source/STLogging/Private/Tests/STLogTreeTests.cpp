#include "STLoggingTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace STLoggingTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeLeafTest, "STLogging.Tree.Leaf", ST_TEST_FLAGS)
bool FSTTreeLeafTest::RunTest(const FString& Parameters)
{
	const FSTLogField Field{ TEXT("Count"), TEXT("5"), {} };
	TestEqual(TEXT("dump"), STLogging::RenderDump(Field), FString(TEXT("Count: 5")));
	TestEqual(TEXT("inline"), STLogging::RenderInline(Field), FString(TEXT("5")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeHierarchyTest, "STLogging.Tree.Hierarchy3Levels", ST_TEST_FLAGS)
bool FSTTreeHierarchyTest::RunTest(const FString& Parameters)
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

	const FSTLogField Field = STLogging::MakeNestedField(TEXT("Root"), &Root);
	TestEqual(TEXT("dump"), STLogging::RenderDump(Field),
		FString(TEXT("Root.Name: R, Root.Mid.Level: 2, Root.Mid.Child.Count: 7, Root.Mid.Child.Label: L")));
	TestEqual(TEXT("inline"), STLogging::RenderInline(Field),
		FString(TEXT("{Root.Name: R, Root.Mid.Level: 2, Root.Mid.Child.Count: 7, Root.Mid.Child.Label: L}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeNullTest, "STLogging.Tree.NullInHierarchy", ST_TEST_FLAGS)
bool FSTTreeNullTest::RunTest(const FString& Parameters)
{
	FMid Mid;
	Mid.Level = 2;
	FRoot Root;
	Root.Name = TEXT("R");

	// Null in the middle: parent non-null, child null; nothing below it is touched.
	Root.Mid = nullptr;
	TestEqual(TEXT("null mid"), STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Root"), &Root)),
		FString(TEXT("Root.Name: R, Root.Mid: null")));

	Root.Mid = &Mid;
	Mid.Child = nullptr;
	TestEqual(TEXT("null leaf"), STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Root"), &Root)),
		FString(TEXT("Root.Name: R, Root.Mid.Level: 2, Root.Mid.Child: null")));

	const FRoot* NullRoot = nullptr;
	TestEqual(TEXT("null top"), STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Root"), NullRoot)),
		FString(TEXT("Root: null")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeEmptyTest, "STLogging.Tree.EmptyLoggable", ST_TEST_FLAGS)
bool FSTTreeEmptyTest::RunTest(const FString& Parameters)
{
	const FEmpty Empty;
	const FSTLogField Field = STLogging::MakeNestedField(TEXT("E"), &Empty);
	TestEqual(TEXT("value"), Field.Value, FString(TEXT("{}")));
	TestEqual(TEXT("children"), Field.Children.Num(), 0);
	TestEqual(TEXT("dump"), STLogging::RenderDump(Field), FString(TEXT("E: {}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeCycleTest, "STLogging.Tree.CyclicReferences", ST_TEST_FLAGS)
bool FSTTreeCycleTest::RunTest(const FString& Parameters)
{
	FNode A;
	A.Name = TEXT("A");
	FNode B;
	B.Name = TEXT("B");
	A.Next = &B;
	B.Next = &A;

	const FString First = STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Start"), &A));
	TestTrue(TEXT("starts with the real values"), First.StartsWith(TEXT("Start.Name: A, Start.Next.Name: B, Start.Next.Next.Name: A")));
	TestTrue(TEXT("terminates at the depth limit"), First.Contains(TEXT("<max depth>")));

	// A guard that failed to unwind would change the second result.
	const FString Second = STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Start"), &A));
	TestEqual(TEXT("repeatable"), Second, First);
	return true;
}

#endif
