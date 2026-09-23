#pragma once

#include "CoreMinimal.h"
#include "STStringify.h"
#include <memory>
#include <type_traits>

// One captured variable: a pointer to it plus a builder that reads its current value.
struct FSTLogValue
{
	const void* Ptr = nullptr;
	FSTLogField (*BuildFn)(const FString& Name, const void* Ptr) = nullptr;
};

// Holds live references to the caller's variables. Must not be rendered after any
// referenced variable has gone out of scope (declare it in the same function).
class STLOGGING_API FSTLogContext
{
public:
	template <typename T>
	void Add(FName Key, T&& Value)
	{
		static_assert(std::is_lvalue_reference_v<T&&>,
			"ST_LOG_ADD requires a named variable, not a temporary - "
			"the context stores a live reference to it, which would otherwise dangle.");
		using Bare = std::remove_reference_t<T>;

		FSTLogValue Entry;
		Entry.Ptr = std::addressof(Value);
		Entry.BuildFn = [](const FString& Name, const void* Ptr) -> FSTLogField
		{
			return STLogging::BuildField(Name, *static_cast<const Bare*>(Ptr));
		};
		SetEntry(Key, Entry);
	}

	// Renders one entry for {Name} substitution; unset if the key is absent.
	TOptional<FString> RenderInline(FName Key) const;

	// Every entry not in ExcludeKeys, in insertion order, joined with ", ".
	FString BuildDump(const TSet<FName>& ExcludeKeys = TSet<FName>()) const;

	int32 Num() const { return Entries.Num(); }

private:
	void SetEntry(FName Key, const FSTLogValue& Entry);

	TArray<TPair<FName, FSTLogValue>> Entries;
};
