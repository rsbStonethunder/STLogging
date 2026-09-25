#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "STLogTestCommandlet.generated.h"

// Runs a scripted set of ST_LOG scenarios that exercise every branch of the STLogging
// plugin, emits real UE_LOG lines, captures them and compares against expected output.
// Returns 0 when every line matches, 1 otherwise.
//   UnrealEditor-Cmd TestProject.uproject -run=STLogTest
UCLASS()
class USTLogTestCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
