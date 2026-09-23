#include "STLogContext.h"

void FSTLogContext::SetEntry(FName Key, const FSTLogValue& Entry)
{
	for (TPair<FName, FSTLogValue>& Existing : Entries)
	{
		if (Existing.Key != Key)
		{
			continue;
		}
		Existing.Value = Entry;
		return;
	}
	Entries.Emplace(Key, Entry);
}

TOptional<FString> FSTLogContext::RenderInline(FName Key) const
{
	for (const TPair<FName, FSTLogValue>& Existing : Entries)
	{
		if (Existing.Key != Key)
		{
			continue;
		}
		return STLogging::RenderInline(Existing.Value.BuildFn(Existing.Key.ToString(), Existing.Value.Ptr));
	}
	return {};
}

FString FSTLogContext::BuildDump(const TSet<FName>& ExcludeKeys) const
{
	TArray<FString> Parts;
	for (const TPair<FName, FSTLogValue>& Existing : Entries)
	{
		if (ExcludeKeys.Contains(Existing.Key))
		{
			continue;
		}
		Parts.Add(STLogging::RenderDump(Existing.Value.BuildFn(Existing.Key.ToString(), Existing.Value.Ptr)));
	}
	return FString::Join(Parts, TEXT(", "));
}
