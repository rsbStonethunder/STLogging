#include "STLogTypes.h"

namespace STLogging
{
namespace
{
thread_local TArray<const void*> GActivePath;

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

FSTNestingGuard::FSTNestingGuard(const void* InObject)
	: Object(InObject)
{
	GActivePath.Add(Object);
}

FSTNestingGuard::~FSTNestingGuard()
{
	GActivePath.Pop();
}

bool FSTNestingGuard::IsCycle() const
{
	// This guard's own entry is last; any earlier match means Object is already on the path.
	return GActivePath.IndexOfByKey(Object) != GActivePath.Num() - 1;
}

bool FSTNestingGuard::IsTooDeep() const
{
	return GActivePath.Num() > MaxNestingDepth;
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
