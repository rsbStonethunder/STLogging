#include "STLogTypes.h"

namespace STLogging
{
namespace
{
thread_local int32 GNestingDepth = 0;

void CollectLeaves(const FSTLogField& Field, const FString& Path, TArray<TPair<FString, FString>>& Out)
{
	if (Field.Children.IsEmpty())
	{
		Out.Emplace(Path, Field.Value);
		return;
	}
	for (const FSTLogField& Child : Field.Children)
	{
		CollectLeaves(Child, Path + TEXT(".") + Child.Name, Out);
	}
}

FString JoinedLeaves(const FSTLogField& Field)
{
	TArray<TPair<FString, FString>> Leaves;
	CollectLeaves(Field, Field.Name, Leaves);

	TArray<FString> Parts;
	for (const TPair<FString, FString>& Leaf : Leaves)
	{
		Parts.Add(Leaf.Key + TEXT(": ") + Leaf.Value);
	}
	return FString::Join(Parts, TEXT(", "));
}
}

FSTNestingGuard::FSTNestingGuard()
{
	++GNestingDepth;
}

FSTNestingGuard::~FSTNestingGuard()
{
	--GNestingDepth;
}

bool FSTNestingGuard::IsTooDeep() const
{
	return GNestingDepth > MaxNestingDepth;
}

FString RenderDump(const FSTLogField& Field)
{
	return JoinedLeaves(Field);
}

FString RenderInline(const FSTLogField& Field)
{
	if (Field.Children.IsEmpty())
	{
		return Field.Value;
	}
	return TEXT("{") + JoinedLeaves(Field) + TEXT("}");
}
}
