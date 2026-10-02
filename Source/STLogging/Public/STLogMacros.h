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

// ---- public macros ----

// Stores a live reference to X under the key "X". X must be an lvalue.
// Under NO_LOGGING (e.g. Shipping), UE_LOG itself compiles down to near-nothing for
// non-Fatal verbosities, but ST_LOG_CONTEXT/ST_LOG_ADD are separate statements that run
// regardless - without this, every FName lookup and TArray growth they do would survive
// into Shipping for a context that can never be logged. X is still referenced (as a
// discarded value) so this doesn't change whether it's an unused-variable warning.
#if NO_LOGGING
#define ST_LOG_ADD_ONE(C, X) (void)(X);
#else
#define ST_LOG_ADD_ONE(C, X) (C).Add(FName(TEXT(#X)), X);
#endif

// Adds 1-16 variables to an existing context.
#define ST_LOG_ADD(Ctx, ...) \
	do { ST_PP_FOR_EACH(ST_LOG_ADD_ONE, Ctx, __VA_ARGS__) } while (0)

// Declares a context, optionally adding variables in the same statement.
#define ST_LOG_CONTEXT(Ctx, ...) \
	FSTLogContext Ctx; \
	__VA_OPT__(ST_LOG_ADD(Ctx, __VA_ARGS__))

// Adds a computed value under Key (a bare name, stringized like ST_LOG_ADD's variables) -
// for a value with no local to take a live reference to. A snapshot, not live: see AddValue().
// See ST_LOG_ADD_ONE above for why this is a no-op under NO_LOGGING.
#if NO_LOGGING
#define ST_LOG_ADD_VALUE(Ctx, Key, Expr) (void)(Expr)
#else
#define ST_LOG_ADD_VALUE(Ctx, Key, Expr) (Ctx).AddValue(FName(TEXT(#Key)), (Expr))
#endif

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
