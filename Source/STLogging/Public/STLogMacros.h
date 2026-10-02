#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "STLogContext.h"
#include "STLogFormat.h"
#include "STLogFunctionName.h"

// ---- preprocessor helpers (conforming preprocessor; UBT enables /Zc:preprocessor) ----

#define ST_PP_CAT(A, B) ST_PP_CAT_I(A, B)
#define ST_PP_CAT_I(A, B) A##B

#define ST_PP_NARG(...) ST_PP_NARG_I(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define ST_PP_NARG_I(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N

#define ST_PP_FE_1(M, C, X) M(C, X)
#define ST_PP_FE_2(M, C, X, ...) M(C, X) ST_PP_FE_1(M, C, __VA_ARGS__)
#define ST_PP_FE_3(M, C, X, ...) M(C, X) ST_PP_FE_2(M, C, __VA_ARGS__)
#define ST_PP_FE_4(M, C, X, ...) M(C, X) ST_PP_FE_3(M, C, __VA_ARGS__)
#define ST_PP_FE_5(M, C, X, ...) M(C, X) ST_PP_FE_4(M, C, __VA_ARGS__)
#define ST_PP_FE_6(M, C, X, ...) M(C, X) ST_PP_FE_5(M, C, __VA_ARGS__)
#define ST_PP_FE_7(M, C, X, ...) M(C, X) ST_PP_FE_6(M, C, __VA_ARGS__)
#define ST_PP_FE_8(M, C, X, ...) M(C, X) ST_PP_FE_7(M, C, __VA_ARGS__)
#define ST_PP_FE_9(M, C, X, ...) M(C, X) ST_PP_FE_8(M, C, __VA_ARGS__)
#define ST_PP_FE_10(M, C, X, ...) M(C, X) ST_PP_FE_9(M, C, __VA_ARGS__)
#define ST_PP_FE_11(M, C, X, ...) M(C, X) ST_PP_FE_10(M, C, __VA_ARGS__)
#define ST_PP_FE_12(M, C, X, ...) M(C, X) ST_PP_FE_11(M, C, __VA_ARGS__)
#define ST_PP_FE_13(M, C, X, ...) M(C, X) ST_PP_FE_12(M, C, __VA_ARGS__)
#define ST_PP_FE_14(M, C, X, ...) M(C, X) ST_PP_FE_13(M, C, __VA_ARGS__)
#define ST_PP_FE_15(M, C, X, ...) M(C, X) ST_PP_FE_14(M, C, __VA_ARGS__)
#define ST_PP_FE_16(M, C, X, ...) M(C, X) ST_PP_FE_15(M, C, __VA_ARGS__)

#define ST_PP_FOR_EACH(M, C, ...) ST_PP_CAT(ST_PP_FE_, ST_PP_NARG(__VA_ARGS__))(M, C, __VA_ARGS__)

// ---- compiled-out forms ----
// What the public macros become under NO_LOGGING (Shipping, unless the target sets
// bUseLoggingInShipping), mirroring UE_LOG there: no argument expression is evaluated, no
// context object or string is built, and only Fatal still fires. Arguments are referenced
// inside sizeof/decltype - an unevaluated context - so they still count as used (no
// unused-variable warnings) and still have to name something that exists. Defined
// unconditionally so the automation tests can exercise them in a logging build too.

namespace STLogging::Private
{
template <typename T>
struct TUnevaluated
{
};
}

// Names X without evaluating it. Works for any expression type, including void and incomplete.
#define ST_LOG_UNEVALUATED(X) ((void)sizeof(::STLogging::Private::TUnevaluated<decltype((X))>))

// Stand-in for FSTLogContext under NO_LOGGING: nothing to construct, fill or destroy, but
// code that declares a context and later passes it to ST_LOG still compiles.
struct FSTNullLogContext
{
};

// Keeps the lvalue requirement, so a temporary fails to compile in every configuration.
#define ST_LOG_OFF_ADD_ONE(C, X) \
	static_assert(std::is_lvalue_reference_v<decltype((X))>, \
		"ST_LOG_ADD requires a named variable, not a temporary - " \
		"the context stores a live reference to it, which would otherwise dangle."); \
	ST_LOG_UNEVALUATED(X);

#define ST_LOG_OFF_ADD(Ctx, ...) \
	do { ST_LOG_UNEVALUATED(Ctx); ST_PP_FOR_EACH(ST_LOG_OFF_ADD_ONE, Ctx, __VA_ARGS__) } while (0)

#define ST_LOG_OFF_CONTEXT(Ctx, ...) \
	[[maybe_unused]] FSTNullLogContext Ctx; \
	__VA_OPT__(ST_LOG_OFF_ADD(Ctx, __VA_ARGS__))

#define ST_LOG_OFF_ADD_VALUE(Ctx, Key, Expr) (ST_LOG_UNEVALUATED(Ctx), ST_LOG_UNEVALUATED(Expr))

// With a context the message is the bare literal: the context was never filled, so there is
// nothing to substitute. The literal is still validated at compile time (the consteval
// FSTLogFormat constructor, forced through a template argument), and nothing runs.
#define ST_LOG_OFF_MSG_1(Format) (TEXT(Format))
#define ST_LOG_OFF_MSG_2(Format, Ctx) \
	(ST_LOG_UNEVALUATED(Ctx), (void)std::integral_constant<int32, STLogging::FSTLogFormat(TEXT(Format)).Len>::value, TEXT(Format))
#define ST_LOG_OFF_MSG(Format, ...) \
	ST_PP_CAT(ST_LOG_OFF_MSG_, ST_PP_NARG(Format __VA_OPT__(,) __VA_ARGS__))(Format __VA_OPT__(,) __VA_ARGS__)

// UE_LOG's own NO_LOGGING expansion: Fatal still formats its message and stops; any other
// verbosity is a discarded if-constexpr branch, type-checked but never run.
#define ST_LOG_OFF(Category, Verbosity, Format, ...) \
	{ \
		if constexpr ((::ELogVerbosity::Verbosity & ::ELogVerbosity::VerbosityMask) == ::ELogVerbosity::Fatal) \
		{ \
			LowLevelFatalError(TEXT("[%s] %s"), *STLogging::CleanFunctionName(ANSI_TO_TCHAR(__FUNCTION__)), \
				ST_LOG_OFF_MSG(Format __VA_OPT__(,) __VA_ARGS__)); \
			CA_ASSUME(false); \
		} \
	}

// ---- public macros ----
// Gated on NO_LOGGING rather than UE_BUILD_SHIPPING, so a target with bUseLoggingInShipping
// gets the full macros back in Shipping.

#if NO_LOGGING

#define ST_LOG_ADD_ONE(C, X) ST_LOG_OFF_ADD_ONE(C, X)
#define ST_LOG_ADD(Ctx, ...) ST_LOG_OFF_ADD(Ctx, __VA_ARGS__)
#define ST_LOG_CONTEXT(Ctx, ...) ST_LOG_OFF_CONTEXT(Ctx __VA_OPT__(,) __VA_ARGS__)
#define ST_LOG_ADD_VALUE(Ctx, Key, Expr) ST_LOG_OFF_ADD_VALUE(Ctx, Key, Expr)
#define ST_LOG_MSG(Format, ...) ST_LOG_OFF_MSG(Format __VA_OPT__(,) __VA_ARGS__)
#define ST_LOG(Category, Verbosity, Format, ...) ST_LOG_OFF(Category, Verbosity, Format __VA_OPT__(,) __VA_ARGS__)

#else

// Stores a live reference to X under the key "X". X must be an lvalue.
#define ST_LOG_ADD_ONE(C, X) (C).Add(FName(TEXT(#X)), X);

// Adds 1-16 variables to an existing context.
#define ST_LOG_ADD(Ctx, ...) \
	do { ST_PP_FOR_EACH(ST_LOG_ADD_ONE, Ctx, __VA_ARGS__) } while (0)

// Declares a context, optionally adding variables in the same statement.
#define ST_LOG_CONTEXT(Ctx, ...) \
	FSTLogContext Ctx; \
	__VA_OPT__(ST_LOG_ADD(Ctx, __VA_ARGS__))

// Adds a computed value under Key (a bare name, stringized like ST_LOG_ADD's variables) -
// for a value with no local to take a live reference to. A snapshot, not live: see AddValue().
#define ST_LOG_ADD_VALUE(Ctx, Key, Expr) (Ctx).AddValue(FName(TEXT(#Key)), (Expr))

#define ST_LOG_MSG_1(Format) (TEXT(Format))
#define ST_LOG_MSG_2(Format, Ctx) (*STLogging::BuildLogMessage(STLogging::FSTLogFormat(TEXT(Format)), Ctx))
#define ST_LOG_MSG(Format, ...) \
	ST_PP_CAT(ST_LOG_MSG_, ST_PP_NARG(Format __VA_OPT__(,) __VA_ARGS__))(Format __VA_OPT__(,) __VA_ARGS__)

// ST_LOG(Category, Verbosity, "literal" [, Ctx])
// Without Ctx the literal is logged as-is (braces are not interpreted). With Ctx the
// literal is compile-time validated, {Name} tokens are substituted, and unreferenced
// context entries are appended as a dump.
#define ST_LOG(Category, Verbosity, Format, ...) \
	UE_LOG(Category, Verbosity, TEXT("[%s] %s"), *STLogging::CleanFunctionName(ANSI_TO_TCHAR(__FUNCTION__)), ST_LOG_MSG(Format __VA_OPT__(,) __VA_ARGS__))

#endif
