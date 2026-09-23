#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "STLogTypes.h"
#include <concepts>
#include <type_traits>

namespace STLogging
{
// bool is a constrained template so a pointer can never convert to bool and print "true".
template <typename T>
	requires std::is_same_v<T, bool>
FString Stringify(T Value)
{
	return Value ? FString(TEXT("true")) : FString(TEXT("false"));
}

template <typename T>
	requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
FString Stringify(T Value)
{
	if constexpr (std::is_signed_v<T>)
	{
		return FString::Printf(TEXT("%lld"), static_cast<long long>(Value));
	}
	else
	{
		return FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(Value));
	}
}

template <typename T>
	requires std::is_enum_v<T>
FString Stringify(T Value)
{
	return Stringify(static_cast<std::underlying_type_t<T>>(Value));
}

inline FString Stringify(float Value) { return FString::SanitizeFloat(Value); }
inline FString Stringify(double Value) { return FString::SanitizeFloat(Value); }
inline FString Stringify(const TCHAR* Value) { return Value ? FString(Value) : FString(TEXT("null")); }
inline FString Stringify(const FString& Value) { return Value; }
inline FString Stringify(const FName& Value) { return Value.ToString(); }
inline FString Stringify(const FText& Value) { return Value.ToString(); }
inline FString Stringify(const FVector& Value) { return Value.ToString(); }
inline FString Stringify(const FRotator& Value) { return Value.ToString(); }
inline FString Stringify(const FTransform& Value) { return Value.ToString(); }

inline FString StringifyObject(const UObject* Object)
{
	return IsValid(Object) ? Object->GetName() : FString(TEXT("null"));
}

// True when Stringify(const T&) resolves (including overloads users add via ADL).
template <typename T>
concept CStringifiable = requires(const T& Value) {
	{ Stringify(Value) } -> std::convertible_to<FString>;
};

namespace Private
{
template <typename T> struct TPointee { using Type = void; };
template <typename U> struct TPointee<U*> { using Type = U; };
template <typename U> struct TPointee<TObjectPtr<U>> { using Type = U; };
template <typename U> struct TPointee<TWeakObjectPtr<U>> { using Type = U; };

template <typename U> const U* ResolvePtr(U* Ptr) { return Ptr; }
template <typename U> const U* ResolvePtr(const TObjectPtr<U>& Ptr) { return Ptr.Get(); }
template <typename U> const U* ResolvePtr(const TWeakObjectPtr<U>& Ptr) { return Ptr.Get(); }

template <typename T> inline constexpr bool bAlwaysFalse = false;
}

template <typename T>
FSTLogField BuildField(const FString& Name, const T& Value)
{
	using Bare = std::remove_cv_t<T>;
	using Pointee = typename Private::TPointee<Bare>::Type;

	if constexpr (!std::is_void_v<Pointee> && std::is_base_of_v<ISTLoggable, Pointee>)
	{
		return MakeNestedField(Name, Private::ResolvePtr(Value));
	}
	else if constexpr (!std::is_void_v<Pointee> && std::is_base_of_v<UObject, Pointee>)
	{
		return FSTLogField{ Name, StringifyObject(Private::ResolvePtr(Value)), {} };
	}
	else if constexpr (std::is_base_of_v<ISTLoggable, Bare>)
	{
		return MakeNestedField(Name, &Value);
	}
	else if constexpr (CStringifiable<Bare>)
	{
		return FSTLogField{ Name, Stringify(Value), {} };
	}
	else
	{
		static_assert(Private::bAlwaysFalse<T>,
			"ST_LOG_ADD: no STLogging::Stringify overload for this type and it does not implement ISTLoggable. "
			"Add a Stringify(const T&) overload or implement ISTLoggable.");
		return FSTLogField{};
	}
}
}
