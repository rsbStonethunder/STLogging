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
	TestEqual(TEXT("stops at the first repeated object"), First,
		FString(TEXT("Start.Name: A, Start.Next.Name: B, Start.Next.Next: <cycle>")));

	// A guard that failed to unwind would change the second result.
	const FString Second = STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Start"), &A));
	TestEqual(TEXT("repeatable"), Second, First);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeDepthLimitTest, "STLogging.Tree.DepthLimitBackstop", ST_TEST_FLAGS)
bool FSTTreeDepthLimitTest::RunTest(const FString& Parameters)
{
	// A long chain of DISTINCT objects has no cycle, so only the depth limit can stop it.
	FNode Chain[12];
	for (int32 i = 0; i < 12; ++i)
	{
		Chain[i].Name = FString::FromInt(i);
		Chain[i].Next = (i + 1 < 12) ? &Chain[i + 1] : nullptr;
	}
	const FString Dump = STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Start"), &Chain[0]));
	TestTrue(TEXT("depth limit reached"), Dump.Contains(TEXT("<max depth>")));
	TestFalse(TEXT("not reported as a cycle"), Dump.Contains(TEXT("<cycle>")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTTreeBackLinkTest, "STLogging.Tree.ParentBackLinksStayCompact", ST_TEST_FLAGS)
bool FSTTreeBackLinkTest::RunTest(const FString& Parameters)
{
	FTreeNode Root;
	Root.Name = TEXT("R");
	FTreeNode C0;
	C0.Name = TEXT("C0");
	FTreeNode C1;
	C1.Name = TEXT("C1");
	FTreeNode C2;
	C2.Name = TEXT("C2");
	FTreeNode* Kids[] = { &C0, &C1, &C2 };
	for (FTreeNode* Kid : Kids)
	{
		Kid->Parent = &Root;
		Root.Children.Add(Kid);
	}

	// Every child points back at the root, which points at every child. Output must be
	// proportional to the number of objects, not to (links per node)^depth.
	const FString Dump = STLogging::RenderDump(STLogging::MakeNestedField(TEXT("Tree"), &Root));
	TestEqual(TEXT("exact compact output"), Dump,
		FString(TEXT("Tree.Name: R, Tree.Parent: null, Tree.Child0.Name: C0, Tree.Child0.Parent: <cycle>, ")
			TEXT("Tree.Child1.Name: C1, Tree.Child1.Parent: <cycle>, ")
			TEXT("Tree.Child2.Name: C2, Tree.Child2.Parent: <cycle>")));
	return true;
}

#endif
