#include "STLogFormat.h"

namespace STLogging
{
void STLOG_FORMAT_STRING_IS_INVALID_check_braces()
{
}

FString BuildLogMessage(const FSTLogFormat& Format, const FSTLogContext& Context)
{
	FString Message;
	TSet<FName> Referenced;

	int32 i = 0;
	while (i < Format.Len)
	{
		const TCHAR C = Format.Str[i];
		const bool bEscapedBrace = (C == TEXT('{') || C == TEXT('}')) && i + 1 < Format.Len && Format.Str[i + 1] == C;
		if (bEscapedBrace)
		{
			Message.AppendChar(C);
			i += 2;
			continue;
		}
		if (C != TEXT('{'))
		{
			Message.AppendChar(C);
			++i;
			continue;
		}

		// FSTLogFormat guarantees a well-formed {Name} or {Name:.N} token from here.
		int32 NameEnd = i + 1;
		while (Format.Str[NameEnd] != TEXT('}') && Format.Str[NameEnd] != TEXT(':'))
		{
			++NameEnd;
		}
		const FString Name(NameEnd - i - 1, Format.Str + i + 1);
		const FName Key(*Name);

		TOptional<int32> Precision;
		int32 End = NameEnd;
		if (Format.Str[NameEnd] == TEXT(':'))
		{
			int32 DigitsStart = NameEnd + 2; // skip ':' and '.'
			End = DigitsStart;
			while (Format.Str[End] != TEXT('}'))
			{
				++End;
			}
			Precision = FCString::Atoi(*FString(End - DigitsStart, Format.Str + DigitsStart));
		}

		const TOptional<FString> Rendered = Context.RenderInline(Key, Precision);
		Message += Rendered.IsSet() ? Rendered.GetValue() : FString::Printf(TEXT("{%s:MISSING}"), *Name);
		Referenced.Add(Key);
		i = End + 1;
	}

	const FString Dump = Context.BuildDump(Referenced);
	if (Dump.IsEmpty())
	{
		return Message;
	}
	if (!Message.IsEmpty() && !FChar::IsWhitespace(Message[Message.Len() - 1]))
	{
		Message.AppendChar(TEXT(' '));
	}
	return Message + TEXT("{") + Dump + TEXT("}");
}
}
