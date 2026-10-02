#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "STLogTypes.h"
#include "STLoggingTestFixtures.generated.h"

#define ST_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// Reflected so STLogging::Stringify can print it by name instead of by number.
UENUM()
enum class ESTReflectedTestEnum : uint8
{
	Beta = 5,
};

namespace STLoggingTests
{
struct FLeaf : public ISTLoggable
{
	int32 Count = 0;
	FString Label;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Count"), FString::FromInt(Count), {} },
			{ TEXT("Label"), Label, {} },
		};
	}
};

struct FMid : public ISTLoggable
{
	int32 Level = 0;
	const FLeaf* Child = nullptr;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Level"), FString::FromInt(Level), {} },
			STLogging::MakeNestedField(TEXT("Child"), Child),
		};
	}
};

struct FRoot : public ISTLoggable
{
	FString Name;
	const FMid* Mid = nullptr;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Name"), Name, {} },
			STLogging::MakeNestedField(TEXT("Mid"), Mid),
		};
	}
};

struct FEmpty : public ISTLoggable
{
	virtual TArray<FSTLogField> GetLogFields() const override { return {}; }
};

struct FNode : public ISTLoggable
{
	FString Name;
	const FNode* Next = nullptr;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		return {
			{ TEXT("Name"), Name, {} },
			STLogging::MakeNestedField(TEXT("Next"), Next),
		};
	}
};

// A tree node with a back-link to its parent and several children: the shape that
// makes depth-limited-only cycle handling blow up combinatorially.
struct FTreeNode : public ISTLoggable
{
	FString Name;
	const FTreeNode* Parent = nullptr;
	TArray<const FTreeNode*> Children;

	virtual TArray<FSTLogField> GetLogFields() const override
	{
		TArray<FSTLogField> Fields;
		Fields.Add({ TEXT("Name"), Name, {} });
		Fields.Add(STLogging::MakeNestedField(TEXT("Parent"), Parent));
		for (int32 i = 0; i < Children.Num(); ++i)
		{
			Fields.Add(STLogging::MakeNestedField(FString::Printf(TEXT("Child%d"), i), Children[i]));
		}
		return Fields;
	}
};
}
