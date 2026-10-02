#include "STLogFunctionName.h"

namespace STLogging
{
FString CleanFunctionName(const FString& RawFunction)
{
	static const FString AnonNamespacePrefix = TEXT("`anonymous-namespace'::");
	static const FString LambdaMarker = TEXT("::<lambda_");

	FString Result = RawFunction;
	if (Result.StartsWith(AnonNamespacePrefix))
	{
		Result = Result.RightChop(AnonNamespacePrefix.Len());
	}

	const int32 LambdaIndex = Result.Find(LambdaMarker, ESearchCase::CaseSensitive);
	if (LambdaIndex != INDEX_NONE)
	{
		Result = Result.Left(LambdaIndex);
	}
	return Result;
}
}
