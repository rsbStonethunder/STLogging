#include "STLogTestCommandlet.h"
#include "STLogTestObjects.h"
#include "Async/Async.h"
#include "Misc/OutputDevice.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY_STATIC(LogSTLoggingTestReport, Log, All);

// ---- compile-time coverage of the format validator (invalid formats can't be ST_LOG'd) ----

static_assert(STLogging::IsValidFormatLiteral(TEXT("")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("plain")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("{A}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("a {A} b {B}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("{{}} {{A}}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{A")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("A}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{A{B}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{{{A")));

// Everything below depends on capturing and byte-comparing real UE_LOG output, which
// UE_LOG itself compiles away for non-Fatal verbosities under NO_LOGGING (e.g. Shipping) -
// there is nothing to capture, and FLogCategory degrades to FNoLoggingCategory (no
// GetCategoryName()/SetVerbosity()), so this whole scenario suite is Dev-build-only.
#if !NO_LOGGING

namespace
{
enum class ETestMode : uint8 { Off = 0, On = 7 };
enum ELegacy { Legacy_Neg = -3 };

struct FExpected
{
	ELogVerbosity::Type Verbosity;
	FString Text;
	bool operator==(const FExpected& Other) const { return Verbosity == Other.Verbosity && Text == Other.Text; }
};

FExpected E(ELogVerbosity::Type Verbosity, const FString& Prefix, const FString& Message)
{
	return { Verbosity, FString::Printf(TEXT("[%s] %s"), *Prefix, *Message) };
}

// Records everything logged to LogSTLoggingTest.
class FCaptureDevice : public FOutputDevice
{
public:
	virtual bool CanBeUsedOnAnyThread() const override { return true; }

	virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (Category != LogSTLoggingTest.GetCategoryName())
		{
			return;
		}
		FScopeLock Lock(&Mutex);
		Lines.Add({ static_cast<ELogVerbosity::Type>(Verbosity & ELogVerbosity::VerbosityMask), Text });
	}

	TArray<FExpected> Take()
	{
		FScopeLock Lock(&Mutex);
		return MoveTemp(Lines);
	}

private:
	FCriticalSection Mutex;
	TArray<FExpected> Lines;
};

TArray<UObject*> GRooted;

template <typename T>
T* Make(const TCHAR* Name)
{
	T* Object = NewObject<T>(GetTransientPackage(), FName(Name));
	Object->AddToRoot();
	GRooted.Add(Object);
	return Object;
}

USTTestNode* MakeNode(const TCHAR* Name, int32 Id, USTTestNode* Next = nullptr)
{
	USTTestNode* Node = Make<USTTestNode>(Name);
	Node->Id = Id;
	Node->Next = Next;
	return Node;
}
}

// Each scenario is a static member so __FUNCTION__ yields "FSTLogScenarios::<Name>".
struct FSTLogScenarios
{
	static void Verbosities()
	{
		ST_LOG(LogSTLoggingTest, Display, "display");
		ST_LOG(LogSTLoggingTest, Log, "log");
		ST_LOG(LogSTLoggingTest, Warning, "warning");
		ST_LOG(LogSTLoggingTest, Error, "error");
		ST_LOG(LogSTLoggingTest, Verbose, "verbose");
	}
	static TArray<FExpected> Expect_Verbosities()
	{
		const FString F = TEXT("FSTLogScenarios::Verbosities");
		return {
			E(ELogVerbosity::Display, F, TEXT("display")),
			E(ELogVerbosity::Log, F, TEXT("log")),
			E(ELogVerbosity::Warning, F, TEXT("warning")),
			E(ELogVerbosity::Error, F, TEXT("error")),
			E(ELogVerbosity::Verbose, F, TEXT("verbose")),
		};
	}

	// Without a context the literal is logged untouched: braces and printf tokens included.
	static void NoContextLiterals()
	{
		ST_LOG(LogSTLoggingTest, Log, "");
		ST_LOG(LogSTLoggingTest, Log, "100% done %s %d {braces} }{ {{x}} {");
	}
	static TArray<FExpected> Expect_NoContextLiterals()
	{
		const FString F = TEXT("FSTLogScenarios::NoContextLiterals");
		return {
			E(ELogVerbosity::Log, F, TEXT("")),
			E(ELogVerbosity::Log, F, TEXT("100% done %s %d {braces} }{ {{x}} {")),
		};
	}

	static void IntegersAndBools()
	{
		const bool bTrue = true;
		bool bFalse = false;
		int8 I8 = -5;
		uint8 U8 = 200;
		int16 I16 = -300;
		uint16 U16 = 60000;
		int32 I32 = -70000;
		uint32 U32 = 4000000000u;
		int64 I64 = MIN_int64;
		uint64 U64 = MAX_uint64;
		ST_LOG_CONTEXT(Ctx, bTrue, bFalse, I8, U8, I16, U16, I32, U32, I64, U64);
		ST_LOG(LogSTLoggingTest, Log, "ints: ", Ctx);
	}
	static TArray<FExpected> Expect_IntegersAndBools()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::IntegersAndBools"),
			TEXT("ints: {bTrue: true, bFalse: false, I8: -5, U8: 200, I16: -300, U16: 60000, I32: -70000, U32: 4000000000, "
				 "I64: -9223372036854775808, U64: 18446744073709551615}")) };
	}

	static void ScalarsAndStrings()
	{
		float F32 = 1.5f;
		double F64 = 0.25;
		FString Str = TEXT("hello");
		FName NameVal = TEXT("SomeName");
		FText TextVal = FText::FromString(TEXT("some text"));
		const TCHAR* CStr = TEXT("cstr");
		const TCHAR* NullCStr = nullptr;
		ETestMode Mode = ETestMode::On;
		ELegacy Legacy = Legacy_Neg;
		ST_LOG_CONTEXT(Ctx, F32, F64, Str, NameVal, TextVal, CStr, NullCStr, Mode, Legacy);
		ST_LOG(LogSTLoggingTest, Log, "scalars: ", Ctx);
	}
	static TArray<FExpected> Expect_ScalarsAndStrings()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::ScalarsAndStrings"),
			FString::Printf(TEXT("scalars: {F32: %s, F64: %s, Str: hello, NameVal: SomeName, TextVal: some text, CStr: cstr, "
								 "NullCStr: null, Mode: 7, Legacy: -3}"),
				*FString::SanitizeFloat(1.5f), *FString::SanitizeFloat(0.25))) };
	}

	static void MathTypesAndCustomStringify()
	{
		FVector Vec(1.0, 2.0, 3.0);
		FRotator Rot(10.0, 20.0, 30.0);
		FTransform Xform(FRotator(0.0, 90.0, 0.0), FVector(4.0, 5.0, 6.0));
		FSTTestPoint2D Point;
		Point.X = 5;
		Point.Y = 6;
		ST_LOG_CONTEXT(Ctx, Vec, Rot, Xform, Point);
		ST_LOG(LogSTLoggingTest, Log, "math: ", Ctx);
	}
	static TArray<FExpected> Expect_MathTypesAndCustomStringify()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::MathTypesAndCustomStringify"),
			FString::Printf(TEXT("math: {Vec: %s, Rot: %s, Xform: %s, Point: (5,6)}"),
				*FVector(1.0, 2.0, 3.0).ToString(), *FRotator(10.0, 20.0, 30.0).ToString(),
				*FTransform(FRotator(0.0, 90.0, 0.0), FVector(4.0, 5.0, 6.0)).ToString())) };
	}

	static void Substitution()
	{
		int32 Health = 5;
		float Speed = 2.5f;
		ST_LOG_CONTEXT(Ctx, Health, Speed);
		ST_LOG(LogSTLoggingTest, Log, "health is {Health}", Ctx);   // remainder dumped
		ST_LOG(LogSTLoggingTest, Log, "{Health} and {speed}", Ctx); // case-insensitive, all referenced: no dump
		ST_LOG(LogSTLoggingTest, Log, "{Health}{Health}", Ctx);     // same key twice
		ST_LOG(LogSTLoggingTest, Log, "{Nope} here", Ctx);          // unknown key
		ST_LOG(LogSTLoggingTest, Log, "", Ctx);                     // empty message: dump only, no leading space
		ST_LOG(LogSTLoggingTest, Log, "state: ", Ctx);              // trailing space: no extra space
		ST_LOG(LogSTLoggingTest, Log, "state:", Ctx);               // no trailing space: one is added
		ST_LOG(LogSTLoggingTest, Log, "{{lit}} {Health} }}", Ctx);  // escaped braces

		ST_LOG_CONTEXT(Empty);
		ST_LOG(LogSTLoggingTest, Log, "nothing to dump", Empty);
		ST_LOG(LogSTLoggingTest, Log, "{X}", Empty);
	}
	static TArray<FExpected> Expect_Substitution()
	{
		const FString F = TEXT("FSTLogScenarios::Substitution");
		return {
			E(ELogVerbosity::Log, F, TEXT("health is 5 {Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("5 and 2.5")),
			E(ELogVerbosity::Log, F, TEXT("55 {Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("{Nope:MISSING} here {Health: 5, Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("{Health: 5, Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("state: {Health: 5, Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("state: {Health: 5, Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("{lit} 5 } {Speed: 2.5}")),
			E(ELogVerbosity::Log, F, TEXT("nothing to dump")),
			E(ELogVerbosity::Log, F, TEXT("{X:MISSING}")),
		};
	}

	static void LiveReferencesAndOverwrite()
	{
		int32 Count = 1;
		ST_LOG_CONTEXT(Ctx, Count);
		Count = 2; // change after ST_LOG_ADD: the live reference sees it
		ST_LOG(LogSTLoggingTest, Log, "count {Count}", Ctx);

		FString Extra = TEXT("x");
		ST_LOG_ADD(Ctx, Extra); // add later
		ST_LOG(LogSTLoggingTest, Log, "later: ", Ctx);
		Count = 3;
		ST_LOG(LogSTLoggingTest, Log, "now {Count}", Ctx);

		// Keys are case-insensitive: adding "alpha" replaces "Alpha" in place, keeping its position.
		int32 Alpha = 1;
		int32 Beta = 2;
		int32 alpha = 3;
		ST_LOG_CONTEXT(Dedupe, Alpha, Beta);
		ST_LOG_ADD(Dedupe, alpha);
		int32 NumEntries = Dedupe.Num();
		ST_LOG(LogSTLoggingTest, Log, "dedupe: ", Dedupe);
		ST_LOG(LogSTLoggingTest, Log, "{ALPHA}", Dedupe);
		ST_LOG_CONTEXT(NumCtx, NumEntries);
		ST_LOG(LogSTLoggingTest, Log, "entries {NumEntries}", NumCtx);
	}
	static TArray<FExpected> Expect_LiveReferencesAndOverwrite()
	{
		const FString F = TEXT("FSTLogScenarios::LiveReferencesAndOverwrite");
		return {
			E(ELogVerbosity::Log, F, TEXT("count 2")),
			E(ELogVerbosity::Log, F, TEXT("later: {Count: 2, Extra: x}")),
			E(ELogVerbosity::Log, F, TEXT("now 3 {Extra: x}")),
			E(ELogVerbosity::Log, F, TEXT("dedupe: {Alpha: 3, Beta: 2}")),
			E(ELogVerbosity::Log, F, TEXT("3 {Beta: 2}")),
			E(ELogVerbosity::Log, F, TEXT("entries 2")),
		};
	}

	static void SixteenVariables()
	{
		int32 A01 = 1, A02 = 2, A03 = 3, A04 = 4, A05 = 5, A06 = 6, A07 = 7, A08 = 8;
		int32 A09 = 9, A10 = 10, A11 = 11, A12 = 12, A13 = 13, A14 = 14, A15 = 15, A16 = 16;
		ST_LOG_CONTEXT(Ctx, A01, A02, A03, A04, A05, A06, A07, A08, A09, A10, A11, A12, A13, A14, A15, A16);
		ST_LOG(LogSTLoggingTest, Log, "sixteen: ", Ctx);
	}
	static TArray<FExpected> Expect_SixteenVariables()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::SixteenVariables"),
			TEXT("sixteen: {A01: 1, A02: 2, A03: 3, A04: 4, A05: 5, A06: 6, A07: 7, A08: 8, "
				 "A09: 9, A10: 10, A11: 11, A12: 12, A13: 13, A14: 14, A15: 15, A16: 16}")) };
	}

	static void ObjectPointers()
	{
		USTTestNode* Live = MakeNode(TEXT("PtrLive"), 1);
		USTTestNode* Doomed = MakeNode(TEXT("PtrDoomed"), 2);

		USTTestNode* Raw = Live;
		const USTTestNode* ConstRaw = Live;
		TObjectPtr<USTTestNode> ObjPtr = Live;
		TWeakObjectPtr<USTTestNode> Weak = Live;
		USTTestNode* NullRaw = nullptr;
		TWeakObjectPtr<USTTestNode> DeadWeak = Doomed;
		USTTestNode* DeadRaw = Doomed;
		Doomed->RemoveFromRoot(); // rooted objects can't be marked garbage
		GRooted.Remove(Doomed);
		Doomed->MarkAsGarbage(); // IsValid() is now false

		ST_LOG_CONTEXT(Ctx, Raw, ConstRaw, ObjPtr, Weak, NullRaw, DeadWeak, DeadRaw);
		ST_LOG(LogSTLoggingTest, Log, "ptrs: ", Ctx);
	}
	static TArray<FExpected> Expect_ObjectPointers()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::ObjectPointers"),
			TEXT("ptrs: {Raw.Id: 1, Raw.Next: null, ConstRaw.Id: 1, ConstRaw.Next: null, ObjPtr.Id: 1, ObjPtr.Next: null, "
				 "Weak.Id: 1, Weak.Next: null, NullRaw: null, DeadWeak: null, DeadRaw: null}")) };
	}

	// UObjects that aren't ISTLoggable (or are declared as plain UObject*) print their name.
	static void NameOnlyObjects()
	{
		USTTestPlain* PlainObject = Make<USTTestPlain>(TEXT("PlainObj"));
		USTTestPlain* PlainRaw = PlainObject;
		TObjectPtr<USTTestPlain> PlainObjPtr = PlainObject;
		TWeakObjectPtr<USTTestPlain> PlainWeak = PlainObject;
		USTTestPlain* PlainNull = nullptr;
		UObject* AsUObject = MakeNode(TEXT("LoggableAsUObject"), 3);
		ST_LOG_CONTEXT(Ctx, PlainRaw, PlainObjPtr, PlainWeak, PlainNull, AsUObject);
		ST_LOG(LogSTLoggingTest, Log, "names: ", Ctx);
	}
	static TArray<FExpected> Expect_NameOnlyObjects()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::NameOnlyObjects"),
			TEXT("names: {PlainRaw: PlainObj, PlainObjPtr: PlainObj, PlainWeak: PlainObj, PlainNull: null, "
				 "AsUObject: LoggableAsUObject}")) };
	}

	static void ValueLoggablesAndEmpty()
	{
		FSTTestOuter Outer;
		Outer.Tag = TEXT("t");
		Outer.Inner.X = 3;
		Outer.Inner.Y = 4;
		USTTestEmpty* Empty = Make<USTTestEmpty>(TEXT("EmptyObj"));

		ST_LOG_CONTEXT(Dump, Outer, Empty);
		ST_LOG(LogSTLoggingTest, Log, "dump: ", Dump);
		ST_LOG(LogSTLoggingTest, Log, "inline {Outer} / {Empty}", Dump);
	}
	static TArray<FExpected> Expect_ValueLoggablesAndEmpty()
	{
		const FString F = TEXT("FSTLogScenarios::ValueLoggablesAndEmpty");
		return {
			E(ELogVerbosity::Log, F, TEXT("dump: {Outer.Tag: t, Outer.Inner.X: 3, Outer.Inner.Y: 4, Empty: {}}")),
			E(ELogVerbosity::Log, F, TEXT("inline {Outer.Tag: t, Outer.Inner.X: 3, Outer.Inner.Y: 4} / {}")),
		};
	}

	static void CyclesAndDiamond()
	{
		USTTestNode* A = MakeNode(TEXT("CycleA"), 1);
		USTTestNode* B = MakeNode(TEXT("CycleB"), 2, A);
		A->Next = B; // A -> B -> A
		USTTestNode* Self = MakeNode(TEXT("CycleSelf"), 9);
		Self->Next = Self;

		USTTestNode* Shared = MakeNode(TEXT("DiamondLeaf"), 4);
		USTTestPair* Diamond = Make<USTTestPair>(TEXT("DiamondPair"));
		Diamond->Left = Shared;
		Diamond->Right = Shared;

		ST_LOG_CONTEXT(Ctx, A, Self, Diamond);
		ST_LOG(LogSTLoggingTest, Log, "graph: ", Ctx);
	}
	static TArray<FExpected> Expect_CyclesAndDiamond()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::CyclesAndDiamond"),
			TEXT("graph: {A.Id: 1, A.Next.Id: 2, A.Next.Next: <cycle>, Self.Id: 9, Self.Next: <cycle>, "
				 "Diamond.Left.Id: 4, Diamond.Left.Next: null, Diamond.Right.Id: 4, Diamond.Right.Next: null}")) };
	}

	static void MaxDepth()
	{
		USTTestNode* Chain = nullptr;
		for (int32 i = 9; i >= 0; --i)
		{
			Chain = MakeNode(*FString::Printf(TEXT("DeepNode%d"), i), i, Chain);
		}
		ST_LOG_CONTEXT(Ctx, Chain);
		ST_LOG(LogSTLoggingTest, Log, "deep: ", Ctx);
	}
	static TArray<FExpected> Expect_MaxDepth()
	{
		// Chain -> 8 levels of Id, then the 9th level on the path is cut off.
		TArray<FString> Parts;
		FString Path = TEXT("Chain");
		for (int32 i = 0; i < STLogging::MaxNestingDepth; ++i)
		{
			Parts.Add(FString::Printf(TEXT("%s.Id: %d"), *Path, i));
			Path += TEXT(".Next");
		}
		Parts.Add(Path + TEXT(": <max depth>"));
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::MaxDepth"),
			TEXT("deep: {") + FString::Join(Parts, TEXT(", ")) + TEXT("}")) };
	}

	// The nesting path is thread_local: a cycle log on a worker thread must behave the same.
	static void CycleOnWorkerThread(USTTestNode* Node)
	{
		ST_LOG_CONTEXT(Ctx, Node);
		ST_LOG(LogSTLoggingTest, Log, "worker: ", Ctx);
	}
	static void WorkerThread()
	{
		USTTestNode* A = MakeNode(TEXT("ThreadA"), 1);
		USTTestNode* B = MakeNode(TEXT("ThreadB"), 2, A);
		A->Next = B;
		Async(EAsyncExecution::ThreadPool, [A]() { CycleOnWorkerThread(A); }).Wait();
	}
	static TArray<FExpected> Expect_WorkerThread()
	{
		return { E(ELogVerbosity::Log, TEXT("FSTLogScenarios::CycleOnWorkerThread"),
			TEXT("worker: {Node.Id: 1, Node.Next.Id: 2, Node.Next.Next: <cycle>}")) };
	}

	// Prefix is [Class::Function] for methods and [Function] for free functions.
	static void CallSites()
	{
		USTTestNode* Node = MakeNode(TEXT("CallSiteNode"), 1);
		Node->Describe();
		USTTestNode::DescribeStatic();
		STLogTestFreeFunction();
	}
	static TArray<FExpected> Expect_CallSites()
	{
		return {
			E(ELogVerbosity::Log, TEXT("USTTestNode::Describe"), TEXT("node 1 {Next: null}")),
			E(ELogVerbosity::Log, TEXT("USTTestNode::DescribeStatic"), TEXT("static")),
			E(ELogVerbosity::Log, TEXT("STLogTestFreeFunction"), TEXT("free")),
		};
	}
};

namespace
{
struct FScenario
{
	const TCHAR* Name;
	void (*Run)();
	TArray<FExpected> (*Expect)();
};

#define ST_SCENARIO(N) { TEXT(#N), &FSTLogScenarios::N, &FSTLogScenarios::Expect_##N }

const FScenario GScenarios[] = {
	ST_SCENARIO(Verbosities),
	ST_SCENARIO(NoContextLiterals),
	ST_SCENARIO(IntegersAndBools),
	ST_SCENARIO(ScalarsAndStrings),
	ST_SCENARIO(MathTypesAndCustomStringify),
	ST_SCENARIO(Substitution),
	ST_SCENARIO(LiveReferencesAndOverwrite),
	ST_SCENARIO(SixteenVariables),
	ST_SCENARIO(ObjectPointers),
	ST_SCENARIO(NameOnlyObjects),
	ST_SCENARIO(ValueLoggablesAndEmpty),
	ST_SCENARIO(CyclesAndDiamond),
	ST_SCENARIO(MaxDepth),
	ST_SCENARIO(WorkerThread),
	ST_SCENARIO(CallSites),
};
#undef ST_SCENARIO
}

#endif // !NO_LOGGING

int32 USTLogTestCommandlet::Main(const FString& Params)
{
#if NO_LOGGING
	UE_LOG(LogSTLoggingTestReport, Display, TEXT("STLogTest skipped: NO_LOGGING"));
	return 0;
#else
	LogSTLoggingTest.SetVerbosity(ELogVerbosity::VeryVerbose); // let the Verbose scenario through

	FCaptureDevice Capture;
	GLog->AddOutputDevice(&Capture);

	int32 Failures = 0;
	for (const FScenario& Scenario : GScenarios)
	{
		Capture.Take();
		Scenario.Run();
		GLog->Flush();

		const TArray<FExpected> Actual = Capture.Take();
		const TArray<FExpected> Expected = Scenario.Expect();
		if (Actual == Expected)
		{
			UE_LOG(LogSTLoggingTestReport, Display, TEXT("PASS %s (%d lines)"), Scenario.Name, Actual.Num());
			continue;
		}

		++Failures;
		UE_LOG(LogSTLoggingTestReport, Error, TEXT("FAIL %s: expected %d lines, got %d"), Scenario.Name, Expected.Num(), Actual.Num());
		const int32 Count = FMath::Max(Actual.Num(), Expected.Num());
		for (int32 i = 0; i < Count; ++i)
		{
			const FString Want = Expected.IsValidIndex(i) ? Expected[i].Text : TEXT("<none>");
			const FString Got = Actual.IsValidIndex(i) ? Actual[i].Text : TEXT("<none>");
			if (Expected.IsValidIndex(i) && Actual.IsValidIndex(i) && Expected[i] == Actual[i])
			{
				continue;
			}
			UE_LOG(LogSTLoggingTestReport, Error, TEXT("  line %d\n    want: %s\n    got:  %s"), i, *Want, *Got);
		}
	}

	GLog->RemoveOutputDevice(&Capture);
	for (UObject* Object : GRooted)
	{
		Object->RemoveFromRoot();
	}
	GRooted.Empty();

	UE_LOG(LogSTLoggingTestReport, Display, TEXT("STLogTest: %d scenarios, %d failed"), UE_ARRAY_COUNT(GScenarios), Failures);
	return Failures == 0 ? 0 : 1;
#endif // NO_LOGGING
}
