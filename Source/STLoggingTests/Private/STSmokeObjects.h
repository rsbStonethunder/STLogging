#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "STLogging.h"
#include "STSmokeObjects.generated.h"

UCLASS()
class USTSmokeLeaf : public UObject, public ISTLoggable
{
	GENERATED_BODY()

public:
	int32 Count = 0;
	FString Label;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Count"), STLogging::Stringify(Count), {} },
			{ TEXT("Label"), Label, {} },
		};
	}
};

UCLASS()
class USTSmokeRoot : public UObject, public ISTLoggable
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USTSmokeLeaf> Leaf;

	int32 Frames = 0;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Frames"), STLogging::Stringify(Frames), {} },
			STLogging::MakeNestedField(TEXT("Leaf"), Leaf.Get()),
		};
	}

	void Describe();
	static void StaticDescribe();
};

void STSmokeFreeFunction();
