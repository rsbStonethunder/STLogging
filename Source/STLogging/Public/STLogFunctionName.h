#pragma once

#include "CoreMinimal.h"

namespace STLogging
{
// Strips compiler noise from __FUNCTION__ so ST_LOG's [Class::Function] prefix stays
// readable from inside a lambda or an anonymous namespace. Both patterns matched are
// MSVC's exact __FUNCTION__ spelling; on a compiler that spells them differently this is
// a harmless no-op, not a cleanup.
STLOGGING_API FString CleanFunctionName(const FString& RawFunction);
}
