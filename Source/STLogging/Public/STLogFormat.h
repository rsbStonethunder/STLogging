#pragma once

#include "CoreMinimal.h"
#include "STLogContext.h"

namespace STLogging
{
// Only ever called from the failure path of the consteval FSTLogFormat constructor.
// It is not constexpr, so reaching it makes the constant evaluation fail, and the
// compiler error names this function.
STLOGGING_API void STLOG_FORMAT_STRING_IS_INVALID_check_braces();

constexpr bool IsAsciiDigit(TCHAR C)
{
	return C >= TEXT('0') && C <= TEXT('9');
}

// Valid: {Name} and {Name:.N} (non-empty Name, no braces inside, N one or more digits),
// and the escapes {{ and }}.
constexpr bool IsValidFormat(const TCHAR* Str, int32 Len)
{
	int32 i = 0;
	while (i < Len)
	{
		const TCHAR C = Str[i];
		if (C == TEXT('{'))
		{
			if (i + 1 < Len && Str[i + 1] == TEXT('{'))
			{
				i += 2;
				continue;
			}
			int32 j = i + 1;
			while (j < Len && Str[j] != TEXT('}') && Str[j] != TEXT('{') && Str[j] != TEXT(':'))
			{
				++j;
			}
			if (j == i + 1)
			{
				return false;
			}
			if (j < Len && Str[j] == TEXT(':'))
			{
				int32 k = j + 1;
				if (k >= Len || Str[k] != TEXT('.'))
				{
					return false;
				}
				++k;
				const int32 DigitsStart = k;
				while (k < Len && IsAsciiDigit(Str[k]))
				{
					++k;
				}
				if (k == DigitsStart || k >= Len || Str[k] != TEXT('}'))
				{
					return false;
				}
				i = k + 1;
				continue;
			}
			if (j >= Len || Str[j] != TEXT('}'))
			{
				return false;
			}
			i = j + 1;
			continue;
		}
		if (C == TEXT('}'))
		{
			if (i + 1 < Len && Str[i + 1] == TEXT('}'))
			{
				i += 2;
				continue;
			}
			return false;
		}
		++i;
	}
	return true;
}

template <size_t N>
constexpr bool IsValidFormatLiteral(const TCHAR (&Literal)[N])
{
	return IsValidFormat(Literal, static_cast<int32>(N) - 1);
}

// A string literal that has been checked at compile time (like std::format_string).
class FSTLogFormat
{
public:
	template <size_t N>
	consteval FSTLogFormat(const TCHAR (&Literal)[N])
		: Str(Literal)
		, Len(static_cast<int32>(N) - 1)
	{
		if (!IsValidFormat(Literal, static_cast<int32>(N) - 1))
		{
			STLOG_FORMAT_STRING_IS_INVALID_check_braces();
		}
	}

	const TCHAR* Str;
	int32 Len;
};

// Substitutes {Name} tokens from Context, then appends every unreferenced entry as
// " {Name: Value, ...}". See the spec's "Rendering format" section.
STLOGGING_API FString BuildLogMessage(const FSTLogFormat& Format, const FSTLogContext& Context);
}
