#include "STLoggingTestFixtures.h"
#include "STLogFormat.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace STLoggingTests;

static_assert(STLogging::IsValidFormatLiteral(TEXT("")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("plain text")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("a {b} c {d}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("{{literal}}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("{{{x}}}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("{Holder.Count}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("open {")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("empty {}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("stray }")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("nested {a{b}}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("unterminated {abc")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{a}}")));

static_assert(STLogging::IsValidFormatLiteral(TEXT("{Speed:.2}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("{Speed:.10}")));
static_assert(STLogging::IsValidFormatLiteral(TEXT("a {Speed:.2} b {Health}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{Speed:}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{Speed:2}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{Speed:.}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{Speed:.2x}")));
static_assert(!STLogging::IsValidFormatLiteral(TEXT("{Speed:.2")));

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTFormatInlineTest, "STLogging.Format.InlineSubstitution", ST_TEST_FLAGS)
bool FSTFormatInlineTest::RunTest(const FString& Parameters)
{
	int32 A = 1;
	int32 B = 2;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("A"), A);
	Ctx.Add(TEXT("B"), B);

	TestEqual(TEXT("single, all referenced: no dump"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("a={A} b={B}")), Ctx), FString(TEXT("a=1 b=2")));
	TestEqual(TEXT("repeated and reordered"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("{B}-{A}-{B}")), Ctx), FString(TEXT("2-1-2")));
	TestEqual(TEXT("case-insensitive"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("{a}{b}")), Ctx), FString(TEXT("12")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTFormatDumpTest, "STLogging.Format.TrailingDump", ST_TEST_FLAGS)
bool FSTFormatDumpTest::RunTest(const FString& Parameters)
{
	int32 A = 1;
	int32 B = 2;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("A"), A);
	Ctx.Add(TEXT("B"), B);

	TestEqual(TEXT("no tokens: everything dumped"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("state.")), Ctx), FString(TEXT("state. {A: 1, B: 2}")));
	TestEqual(TEXT("trailing space is not doubled"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("state. ")), Ctx), FString(TEXT("state. {A: 1, B: 2}")));
	TestEqual(TEXT("partial reference dumps the rest"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("a={A}")), Ctx), FString(TEXT("a=1 {B: 2}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTFormatMissingAndEmptyTest, "STLogging.Format.MissingAndEmpty", ST_TEST_FLAGS)
bool FSTFormatMissingAndEmptyTest::RunTest(const FString& Parameters)
{
	int32 Count = 5;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("Count"), Count);

	TestEqual(TEXT("missing key renders MISSING and the rest is dumped"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("v={Nope}")), Ctx), FString(TEXT("v={Nope:MISSING} {Count: 5}")));

	const FSTLogContext Empty;
	TestEqual(TEXT("empty context appends nothing"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("plain")), Empty), FString(TEXT("plain")));
	TestEqual(TEXT("empty context, missing key"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("x={x}")), Empty), FString(TEXT("x={x:MISSING}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTFormatEscapeTest, "STLogging.Format.BraceEscapes", ST_TEST_FLAGS)
bool FSTFormatEscapeTest::RunTest(const FString& Parameters)
{
	int32 A = 1;
	FSTLogContext Ctx;
	Ctx.Add(TEXT("A"), A);

	TestEqual(TEXT("escaped braces"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("{{x}} {A}")), Ctx), FString(TEXT("{x} 1")));
	TestEqual(TEXT("escape next to a token"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("{{{A}}}")), Ctx), FString(TEXT("{1}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTFormatCompositeTest, "STLogging.Format.CompositeAndLive", ST_TEST_FLAGS)
bool FSTFormatCompositeTest::RunTest(const FString& Parameters)
{
	FLeaf Leaf;
	Leaf.Count = 7;
	Leaf.Label = TEXT("L");
	FSTLogContext Ctx;
	Ctx.Add(TEXT("Obj"), Leaf);

	TestEqual(TEXT("composite inline"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("o={Obj}")), Ctx), FString(TEXT("o={Obj.Count: 7, Obj.Label: L}")));
	TestEqual(TEXT("composite dump"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("s.")), Ctx), FString(TEXT("s. {Obj.Count: 7, Obj.Label: L}")));

	Leaf.Count = 8;
	TestEqual(TEXT("live after mutation"),
		STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("o={Obj}")), Ctx), FString(TEXT("o={Obj.Count: 8, Obj.Label: L}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTFormatPrecisionTest, "STLogging.Format.Precision", ST_TEST_FLAGS)
bool FSTFormatPrecisionTest::RunTest(const FString& Parameters)
{
	float Speed = 12.3456f;
	int32 Health = 5;
	FVector Pos(1.2345f, 2.3456f, 3.4567f);
	FSTLogContext Ctx;
	Ctx.Add(TEXT("Speed"), Speed);
	Ctx.Add(TEXT("Health"), Health);
	Ctx.Add(TEXT("Pos"), Pos);

	const FString SpeedMsg = STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("v={Speed:.2}")), Ctx);
	TestTrue(TEXT("float precision"), SpeedMsg.StartsWith(TEXT("v=12.35 {")));

	const FString PosMsg = STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("p={Pos:.1}")), Ctx);
	TestTrue(TEXT("vector precision, each component"), PosMsg.StartsWith(TEXT("p=X=1.2 Y=2.3 Z=3.5 {")));

	const FString HealthMsg = STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT("h={Health:.3}")), Ctx);
	TestTrue(TEXT("precision on non-float value is ignored"), HealthMsg.StartsWith(TEXT("h=5 {")));

	return true;
}

#endif
