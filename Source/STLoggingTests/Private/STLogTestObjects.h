#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "STLogging.h"
#include "STLogTestObjects.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSTLoggingTest, Verbose, All);

// Plain (non-ISTLoggable) value type with a user-supplied Stringify overload (found by ADL).
struct FSTTestPoint2D
{
	int32 X = 0;
	int32 Y = 0;
};

inline FString Stringify(const FSTTestPoint2D& Point)
{
	return FString::Printf(TEXT("(%d,%d)"), Point.X, Point.Y);
}

// Non-UObject ISTLoggable, used by value and as a nested member.
struct FSTTestInner : public ISTLoggable
{
	int32 X = 0;
	int32 Y = 0;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("X"), STLogging::Stringify(X), {} },
			{ TEXT("Y"), STLogging::Stringify(Y), {} },
		};
	}
};

struct FSTTestOuter : public ISTLoggable
{
	FString Tag;
	FSTTestInner Inner;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Tag"), Tag, {} },
			STLogging::MakeNestedField(TEXT("Inner"), &Inner),
		};
	}
};

// Linked node: builds chains, cycles and max-depth nesting.
UCLASS()
class USTTestNode : public UObject, public ISTLoggable
{
	GENERATED_BODY()

public:
	int32 Id = 0;

	UPROPERTY()
	TObjectPtr<USTTestNode> Next;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Id"), STLogging::Stringify(Id), {} },
			STLogging::MakeNestedField(TEXT("Next"), Next.Get()),
		};
	}

	void Describe() const;
	static void DescribeStatic();
};

// Two nested fields, both pointing at the same child (a diamond is not a cycle).
UCLASS()
class USTTestPair : public UObject, public ISTLoggable
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USTTestNode> Left;

	UPROPERTY()
	TObjectPtr<USTTestNode> Right;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			STLogging::MakeNestedField(TEXT("Left"), Left.Get()),
			STLogging::MakeNestedField(TEXT("Right"), Right.Get()),
		};
	}
};

// ISTLoggable that reports no fields.
UCLASS()
class USTTestEmpty : public UObject, public ISTLoggable
{
	GENERATED_BODY()

public:
	virtual TArray<FSTLogField> GetLogFields() const override { return {}; }
};

// UObject that does not implement ISTLoggable: logged by name only.
UCLASS()
class USTTestPlain : public UObject
{
	GENERATED_BODY()
};

void STLogTestFreeFunction();
