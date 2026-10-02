#pragma once

#include "CoreMinimal.h"
#include "STStringify.h"
#include <memory>
#include <type_traits>

// One captured variable: a pointer to it plus a builder that reads its current value.
struct FSTLogValue
{
	const void* Ptr = nullptr;
	FSTLogField (*BuildFn)(const FString& Name, const void* Ptr, TOptional<int32> Precision) = nullptr;
};

// Holds live references to the caller's variables. Must not be rendered after any
// referenced variable has gone out of scope (declare it in the same function).
class STLOGGING_API FSTLogContext
{
public:
	FSTLogContext() = default;

	// Not copyable or movable: it's meant to be declared and used in one stack frame, and
	// (since AddValue()) it can own values that other entries' Ptr may point at indirectly
	// through nested fields, which a relocating copy/move would not keep valid.
	FSTLogContext(const FSTLogContext&) = delete;
	FSTLogContext& operator=(const FSTLogContext&) = delete;
	FSTLogContext(FSTLogContext&&) = delete;
	FSTLogContext& operator=(FSTLogContext&&) = delete;

	template <typename T>
	void Add(FName Key, T&& Value)
	{
		static_assert(std::is_lvalue_reference_v<T&&>,
			"ST_LOG_ADD requires a named variable, not a temporary - "
			"the context stores a live reference to it, which would otherwise dangle.");
		using Bare = std::remove_reference_t<T>;

		FSTLogValue Entry;
		Entry.Ptr = std::addressof(Value);
		Entry.BuildFn = [](const FString& Name, const void* Ptr, TOptional<int32> Precision) -> FSTLogField
		{
			return STLogging::BuildField(Name, *static_cast<const Bare*>(Ptr), Precision);
		};
		SetEntry(Key, Entry);
	}

	// Stores a point-in-time copy of Value under Key, for computed values that have no
	// variable to take a live reference to. Unlike Add(), this does not need an lvalue and
	// does not track later changes - it snapshots Value now. Safe with an rvalue or lvalue.
	template <typename T>
	void AddValue(FName Key, T&& Value)
	{
		using Bare = std::decay_t<T>;

		auto Owned = MakeUnique<TOwnedValue<Bare>>();
		Owned->Value = Forward<T>(Value);
		const Bare* Ptr = &Owned->Value;

		// Converting TUniquePtr<Derived>&& to TUniquePtr<IOwnedValue> inline as an Add()
		// argument leads MSVC to the deleted copy-converting constructor instead of the
		// move one; an explicit base-typed local disambiguates it.
		TUniquePtr<IOwnedValue> Base = MoveTemp(Owned);
		OwnedValues.Add(MoveTemp(Base));

		FSTLogValue Entry;
		Entry.Ptr = Ptr;
		Entry.BuildFn = [](const FString& Name, const void* InPtr, TOptional<int32> Precision) -> FSTLogField
		{
			return STLogging::BuildField(Name, *static_cast<const Bare*>(InPtr), Precision);
		};
		SetEntry(Key, Entry);
	}

	// Renders one entry for {Name} / {Name:.N} substitution; unset if the key is absent.
	// Precision is ignored by types that don't support it (see STStringify.h).
	TOptional<FString> RenderInline(FName Key, TOptional<int32> Precision = {}) const;

	// Every entry not in ExcludeKeys, in insertion order, joined with ", ".
	FString BuildDump(const TSet<FName>& ExcludeKeys = TSet<FName>()) const;

	int32 Num() const { return Entries.Num(); }

private:
	// Type-erased owning box for an AddValue() snapshot (base for TUniquePtr<IOwnedValue>;
	// the virtual destructor makes deleting the derived TOwnedValue<T> through it correct).
	struct IOwnedValue
	{
		virtual ~IOwnedValue() = default;
	};
	template <typename T>
	struct TOwnedValue : IOwnedValue
	{
		T Value;
	};

	void SetEntry(FName Key, const FSTLogValue& Entry);

	TArray<TPair<FName, FSTLogValue>> Entries;
	TArray<TUniquePtr<IOwnedValue>> OwnedValues;
};
