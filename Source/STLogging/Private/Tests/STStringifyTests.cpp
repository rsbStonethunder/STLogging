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

// Has only a member ToString(); no Stringify overload, not ISTLoggable.
struct FToStringOnly
{
	FString ToString() const { return TEXT("tso"); }
};

// Has only a free LexToString(); no member ToString(), no Stringify overload.
struct FLexToStringOnly
{
};
inline FString LexToString(const FLexToStringOnly&) { return TEXT("lex"); }

// Has both an explicit Stringify overload and a ToString(); the overload must win.
struct FExplicitOverloadWins
{
	FString ToString() const { return TEXT("should-not-see-this"); }
};
inline FString Stringify(const FExplicitOverloadWins&) { return TEXT("explicit"); }
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTStringifyFallbackTest, "STLogging.Stringify.Fallbacks", ST_TEST_FLAGS)
bool FSTStringifyFallbackTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ToString() fallback"), STLogging::Stringify(FToStringOnly()), FString(TEXT("tso")));
	TestEqual(TEXT("LexToString() fallback"), STLogging::Stringify(FLexToStringOnly()), FString(TEXT("lex")));
	// Via BuildField (not a qualified STLogging::Stringify(...) call) so the explicit
	// overload, found only through ADL into STLoggingTests, is actually in the candidate set.
	FExplicitOverloadWins ExplicitWins;
	TestEqual(TEXT("explicit Stringify overload wins over ToString()"), STLogging::BuildField(TEXT("X"), ExplicitWins).Value, FString(TEXT("explicit")));
	TestEqual(TEXT("reflected UENUM prints by name"), STLogging::Stringify(ESTReflectedTestEnum::Beta), FString(TEXT("Beta")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTBuildFieldPrecisionTest, "STLogging.BuildField.Precision", ST_TEST_FLAGS)
bool FSTBuildFieldPrecisionTest::RunTest(const FString& Parameters)
{
	const float Speed = 12.3456f;
	TestEqual(TEXT("float, precision 2"), STLogging::BuildField(TEXT("Speed"), Speed, 2).Value, FString(TEXT("12.35")));
	TestEqual(TEXT("float, no precision unaffected"), STLogging::BuildField(TEXT("Speed"), Speed).Value, STLogging::Stringify(Speed));

	const FVector Pos(1.27, 2.24, 3.46);
	TestEqual(TEXT("FVector, precision 1"), STLogging::BuildField(TEXT("Pos"), Pos, 1).Value, FString(TEXT("X=1.3 Y=2.2 Z=3.5")));

	const FVector2D Pos2D(1.27f, 2.24f);
	TestEqual(TEXT("FVector2D, precision 1"), STLogging::BuildField(TEXT("Pos2D"), Pos2D, 1).Value, FString(TEXT("X=1.3 Y=2.2")));

	const FVector4 Pos4(1.27f, 2.24f, 3.46f, 4.21f);
	TestEqual(TEXT("FVector4, precision 1"), STLogging::BuildField(TEXT("Pos4"), Pos4, 1).Value, FString(TEXT("X=1.3 Y=2.2 Z=3.5 W=4.2")));

	const FRotator Rot(1.27f, 2.24f, 3.46f);
	TestEqual(TEXT("FRotator, precision 1"), STLogging::BuildField(TEXT("Rot"), Rot, 1).Value, FString(TEXT("P=1.3 Y=2.2 R=3.5")));

	const FQuat Quat(1.27f, 2.24f, 3.46f, 4.21f);
	TestEqual(TEXT("FQuat, precision 1"), STLogging::BuildField(TEXT("Quat"), Quat, 1).Value, FString(TEXT("X=1.3 Y=2.2 Z=3.5 W=4.2")));

	const int32 Health = 5;
	TestEqual(TEXT("precision ignored for non-float"), STLogging::BuildField(TEXT("Health"), Health, 3).Value, FString(TEXT("5")));
	return true;
}

#endif
