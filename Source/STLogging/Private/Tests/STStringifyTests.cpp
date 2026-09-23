#include "STLoggingTestFixtures.h"
#include "STStringify.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace STLoggingTests;

namespace STLoggingTests
{
enum class ETestEnum : uint8
{
	Alpha = 3,
};
}

// A pointer must never be accepted as a bool (it would silently print "true").
static_assert(STLogging::CStringifiable<bool>);
static_assert(STLogging::CStringifiable<const TCHAR*>);
static_assert(!STLogging::CStringifiable<const int32*>);
static_assert(!STLogging::CStringifiable<UObject*>);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTStringifyNumbersTest, "STLogging.Stringify.Numbers", ST_TEST_FLAGS)
bool FSTStringifyNumbersTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("int32"), STLogging::Stringify(int32(-5)), FString(TEXT("-5")));
	TestEqual(TEXT("uint8"), STLogging::Stringify(uint8(200)), FString(TEXT("200")));
	TestEqual(TEXT("uint64"), STLogging::Stringify(uint64(18446744073709551615ull)), FString(TEXT("18446744073709551615")));
	TestEqual(TEXT("float"), STLogging::Stringify(2.5f), FString(TEXT("2.5")));
	TestEqual(TEXT("double"), STLogging::Stringify(2.5), FString(TEXT("2.5")));
	TestEqual(TEXT("true"), STLogging::Stringify(true), FString(TEXT("true")));
	TestEqual(TEXT("false"), STLogging::Stringify(false), FString(TEXT("false")));
	TestEqual(TEXT("enum prints its number"), STLogging::Stringify(ETestEnum::Alpha), FString(TEXT("3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTStringifyTextTest, "STLogging.Stringify.TextAndMath", ST_TEST_FLAGS)
bool FSTStringifyTextTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("FString"), STLogging::Stringify(FString(TEXT("abc"))), FString(TEXT("abc")));
	TestEqual(TEXT("FName"), STLogging::Stringify(FName(TEXT("Nm"))), FString(TEXT("Nm")));
	TestEqual(TEXT("FText"), STLogging::Stringify(FText::FromString(TEXT("Txt"))), FString(TEXT("Txt")));
	TestEqual(TEXT("literal"), STLogging::Stringify(TEXT("lit")), FString(TEXT("lit")));

	const TCHAR* NullText = nullptr;
	TestEqual(TEXT("null TCHAR*"), STLogging::Stringify(NullText), FString(TEXT("null")));

	TestEqual(TEXT("FVector"), STLogging::Stringify(FVector(1, 2, 3)), FVector(1, 2, 3).ToString());
	TestEqual(TEXT("FRotator"), STLogging::Stringify(FRotator(1, 2, 3)), FRotator(1, 2, 3).ToString());
	TestEqual(TEXT("FTransform"), STLogging::Stringify(FTransform::Identity), FTransform::Identity.ToString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTBuildFieldPlainTest, "STLogging.BuildField.Plain", ST_TEST_FLAGS)
bool FSTBuildFieldPlainTest::RunTest(const FString& Parameters)
{
	const int32 Count = 42;
	const FSTLogField Field = STLogging::BuildField(TEXT("Count"), Count);
	TestEqual(TEXT("name"), Field.Name, FString(TEXT("Count")));
	TestEqual(TEXT("value"), Field.Value, FString(TEXT("42")));
	TestEqual(TEXT("leaf"), Field.Children.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTBuildFieldLoggableTest, "STLogging.BuildField.Loggable", ST_TEST_FLAGS)
bool FSTBuildFieldLoggableTest::RunTest(const FString& Parameters)
{
	FLeaf Leaf;
	Leaf.Count = 7;
	Leaf.Label = TEXT("L");

	const FSTLogField ByValue = STLogging::BuildField(TEXT("Leaf"), Leaf);
	TestEqual(TEXT("by value"), STLogging::RenderDump(ByValue), FString(TEXT("Leaf.Count: 7, Leaf.Label: L")));

	const FLeaf* Ptr = &Leaf;
	const FSTLogField ByPtr = STLogging::BuildField(TEXT("Leaf"), Ptr);
	TestEqual(TEXT("by pointer"), STLogging::RenderDump(ByPtr), FString(TEXT("Leaf.Count: 7, Leaf.Label: L")));

	const FLeaf* NullPtr = nullptr;
	TestEqual(TEXT("null pointer"), STLogging::RenderDump(STLogging::BuildField(TEXT("Leaf"), NullPtr)), FString(TEXT("Leaf: null")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTBuildFieldObjectTest, "STLogging.BuildField.UObjectPointers", ST_TEST_FLAGS)
bool FSTBuildFieldObjectTest::RunTest(const FString& Parameters)
{
	UObject* Live = UObject::StaticClass()->GetDefaultObject();
	const FString LiveName = Live->GetName();

	UObject* RawNull = nullptr;
	TestEqual(TEXT("raw null"), STLogging::BuildField(TEXT("O"), RawNull).Value, FString(TEXT("null")));
	TestEqual(TEXT("raw live"), STLogging::BuildField(TEXT("O"), Live).Value, LiveName);

	TObjectPtr<UObject> ObjPtrNull;
	TestEqual(TEXT("TObjectPtr null"), STLogging::BuildField(TEXT("O"), ObjPtrNull).Value, FString(TEXT("null")));
	TObjectPtr<UObject> ObjPtrLive = Live;
	TestEqual(TEXT("TObjectPtr live"), STLogging::BuildField(TEXT("O"), ObjPtrLive).Value, LiveName);

	TWeakObjectPtr<UObject> WeakUnset;
	TestEqual(TEXT("TWeakObjectPtr unset"), STLogging::BuildField(TEXT("O"), WeakUnset).Value, FString(TEXT("null")));
	TWeakObjectPtr<UObject> WeakLive = Live;
	TestEqual(TEXT("TWeakObjectPtr live"), STLogging::BuildField(TEXT("O"), WeakLive).Value, LiveName);
	return true;
}

#endif
