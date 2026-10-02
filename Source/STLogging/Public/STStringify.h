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

namespace Private
{
// True when T is a reflected UENUM (StaticEnum<T>() resolves to a non-deleted specialization).
template <typename T>
concept CReflectedEnum = requires { StaticEnum<T>(); };
}

template <typename T>
	requires std::is_enum_v<T>
FString Stringify(T Value)
{
	if constexpr (Private::CReflectedEnum<T>)
	{
		if (const UEnum* Enum = StaticEnum<T>())
		{
			const FString Name = Enum->GetNameStringByValue(static_cast<int64>(Value));
			if (!Name.IsEmpty())
			{
				return Name;
			}
		}
	}
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

namespace Private
{
// {Name:.N} precision formatting, for floats and the float-component math types. Field
// names/order match each type's own ToString() so the precise form reads like the default.
inline FString StringifyPrecise(float Value, int32 Precision) { return FString::Printf(TEXT("%.*f"), Precision, Value); }
inline FString StringifyPrecise(double Value, int32 Precision) { return FString::Printf(TEXT("%.*f"), Precision, Value); }
inline FString StringifyPrecise(const FVector& Value, int32 Precision)
{
	return FString::Printf(TEXT("X=%.*f Y=%.*f Z=%.*f"), Precision, Value.X, Precision, Value.Y, Precision, Value.Z);
}
inline FString StringifyPrecise(const FVector2D& Value, int32 Precision)
{
	return FString::Printf(TEXT("X=%.*f Y=%.*f"), Precision, Value.X, Precision, Value.Y);
}
inline FString StringifyPrecise(const FVector4& Value, int32 Precision)
{
	return FString::Printf(TEXT("X=%.*f Y=%.*f Z=%.*f W=%.*f"), Precision, Value.X, Precision, Value.Y, Precision, Value.Z, Precision, Value.W);
}
inline FString StringifyPrecise(const FRotator& Value, int32 Precision)
{
	return FString::Printf(TEXT("P=%.*f Y=%.*f R=%.*f"), Precision, Value.Pitch, Precision, Value.Yaw, Precision, Value.Roll);
}
inline FString StringifyPrecise(const FQuat& Value, int32 Precision)
{
	return FString::Printf(TEXT("X=%.*f Y=%.*f Z=%.*f W=%.*f"), Precision, Value.X, Precision, Value.Y, Precision, Value.Z, Precision, Value.W);
}

// True when StringifyPrecise(const T&, int32) resolves (ADL included, like CStringifiable).
template <typename T>
concept CPrecisionStringifiable = requires(const T& Value, int32 Precision) {
	{ StringifyPrecise(Value, Precision) } -> std::convertible_to<FString>;
};
}

namespace Private
{
// Class types only: UE defines LexToString (and some types a ToString()) for built-in
// scalars too, which would otherwise collide with the exact overloads above for them.
template <typename T>
concept CHasToString = std::is_class_v<T> && requires(const T& Value) {
	{ Value.ToString() } -> std::convertible_to<FString>;
};

// True when a free LexToString(T) resolves via ADL, returning something convertible to FString.
template <typename T>
concept CHasLexToString = std::is_class_v<T> && requires(const T& Value) {
	{ LexToString(Value) } -> std::convertible_to<FString>;
};
}

// Fallback for types with a member ToString() but no explicit Stringify overload.
template <typename T>
	requires Private::CHasToString<T>
FString Stringify(const T& Value)
{
	return Value.ToString();
}

// Fallback for types with a free LexToString() but no ToString() or explicit Stringify overload.
template <typename T>
	requires(Private::CHasLexToString<T> && !Private::CHasToString<T>)
FString Stringify(const T& Value)
{
	return LexToString(Value);
}

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
FSTLogField BuildField(const FString& Name, const T& Value, TOptional<int32> Precision = {})
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
		if constexpr (Private::CPrecisionStringifiable<Bare>)
		{
			if (Precision.IsSet())
			{
				return FSTLogField{ Name, Private::StringifyPrecise(Value, Precision.GetValue()), {} };
			}
		}
		return FSTLogField{ Name, Stringify(Value), {} };
	}
	else
	{
		static_assert(Private::bAlwaysFalse<T>,
			"ST_LOG_ADD: no STLogging::Stringify overload for this type, it has no ToString() or LexToString(), "
			"and it does not implement ISTLoggable. Add a Stringify(const T&) overload, a ToString() method, "
			"a LexToString(const T&) overload, or implement ISTLoggable.");
		return FSTLogField{};
	}
}
}
