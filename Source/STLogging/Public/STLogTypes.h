#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include <type_traits>

struct FSTLogField
{
	FString Name;
	FString Value;                // leaf value; unused when Children is non-empty
	TArray<FSTLogField> Children; // nested fields, for object hierarchies
};

class ISTLoggable
{
public:
	virtual ~ISTLoggable() = default;
	virtual TArray<FSTLogField> GetLogFields() const = 0;
};

namespace STLogging
{
constexpr int32 MaxNestingDepth = 8;

// RAII counter of active nested GetLogFields() expansions on this thread.
class STLOGGING_API FSTNestingGuard
{
public:
	FSTNestingGuard();
	~FSTNestingGuard();
	bool IsTooDeep() const;
};

template <typename T>
bool IsLive(const T* Ptr)
{
	if constexpr (std::is_base_of_v<UObject, T>)
	{
		return IsValid(Ptr);
	}
	else
	{
		return Ptr != nullptr;
	}
}

template <typename T>
FSTLogField MakeNestedField(FString Name, const T* Obj)
{
	static_assert(std::is_base_of_v<ISTLoggable, T>, "MakeNestedField requires a type implementing ISTLoggable.");

	if (!IsLive(Obj))
	{
		return FSTLogField{ MoveTemp(Name), TEXT("null"), {} };
	}

	FSTNestingGuard Guard;
	if (Guard.IsTooDeep())
	{
		return FSTLogField{ MoveTemp(Name), TEXT("<max depth>"), {} };
	}

	TArray<FSTLogField> Fields = Obj->GetLogFields();
	if (Fields.IsEmpty())
	{
		return FSTLogField{ MoveTemp(Name), TEXT("{}"), {} };
	}
	return FSTLogField{ MoveTemp(Name), FString(), MoveTemp(Fields) };
}

// Leaf: Value. Composite: "{Path: Value, ...}".
STLOGGING_API FString RenderInline(const FSTLogField& Field);

// Leaves joined with ", " as "Path: Value" (Path is dotted from Field.Name).
STLOGGING_API FString RenderDump(const FSTLogField& Field);
}
