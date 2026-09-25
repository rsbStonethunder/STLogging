#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "STSmokeCommandlet.generated.h"

UCLASS()
class USTSmokeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
