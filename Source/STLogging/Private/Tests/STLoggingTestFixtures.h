#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "STLogTypes.h"

#define ST_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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
}
