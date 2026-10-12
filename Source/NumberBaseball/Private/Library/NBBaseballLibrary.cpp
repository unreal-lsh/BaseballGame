#include "Library/NBBaseballLibrary.h"

void UNBBaseballLibrary::GenerateSecretNumber(TArray<int32>& OutSecretNumber)
{
	// 1~9 후보에서 하나씩 뽑아 제거하므로 중복이 생길 수 없다.
	TArray<int32> Candidates;
	for (int32 Number = 1; Number <= 9; ++Number)
	{
		Candidates.Add(Number);
	}

	OutSecretNumber.Reset();
	for (int32 Count = 0; Count < NumberLength; ++Count)
	{
		const int32 PickedIndex = FMath::RandRange(0, Candidates.Num() - 1);
		OutSecretNumber.Add(Candidates[PickedIndex]);
		Candidates.RemoveAtSwap(PickedIndex);
	}
}

ENBGuessValidation UNBBaseballLibrary::ValidateGuess(const FString& Input, TArray<int32>& OutDigits)
{
	OutDigits.Reset();

	if (Input.IsEmpty())
	{
		return ENBGuessValidation::WrongLength;
	}

	// 1) 문자가 섞여 있는가?
	for (const TCHAR Character : Input)
	{
		if (Character < TEXT('0') || Character > TEXT('9'))
		{
			return ENBGuessValidation::NotNumeric;
		}
	}

	// 2) 3자리인가?
	if (Input.Len() != NumberLength)
	{
		return ENBGuessValidation::WrongLength;
	}

	// 3) 1~9 범위이고 중복이 없는가?
	TArray<int32> Digits;
	for (const TCHAR Character : Input)
	{
		const int32 Digit = static_cast<int32>(Character - TEXT('0'));
		if (Digit == 0)
		{
			return ENBGuessValidation::OutOfRange;
		}
		if (Digits.Contains(Digit))
		{
			return ENBGuessValidation::Duplicated;
		}
		Digits.Add(Digit);
	}

	OutDigits = MoveTemp(Digits);
	return ENBGuessValidation::Valid;
}

void UNBBaseballLibrary::JudgeGuess(const TArray<int32>& SecretNumber, const TArray<int32>& GuessDigits, int32& OutStrikeCount, int32& OutBallCount)
{
	OutStrikeCount = 0;
	OutBallCount = 0;

	if (SecretNumber.Num() != GuessDigits.Num())
	{
		return;
	}

	for (int32 Index = 0; Index < GuessDigits.Num(); ++Index)
	{
		if (GuessDigits[Index] == SecretNumber[Index])
		{
			++OutStrikeCount; // 숫자와 자리가 모두 일치
		}
		else if (SecretNumber.Contains(GuessDigits[Index]))
		{
			++OutBallCount; // 숫자는 있지만 자리가 다름
		}
	}
}

FString UNBBaseballLibrary::MakeResultString(int32 StrikeCount, int32 BallCount)
{
	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}
	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

FString UNBBaseballLibrary::GetValidationMessage(ENBGuessValidation Validation)
{
	switch (Validation)
	{
	case ENBGuessValidation::NotNumeric:
		return TEXT("숫자만 입력해주세요. 다시 입력하세요.");
	case ENBGuessValidation::WrongLength:
		return TEXT("3자리 숫자를 입력해주세요. 다시 입력하세요.");
	case ENBGuessValidation::OutOfRange:
		return TEXT("1~9 사이의 숫자만 사용할 수 있습니다. 다시 입력하세요.");
	case ENBGuessValidation::Duplicated:
		return TEXT("중복되지 않은 숫자를 입력해주세요. 다시 입력하세요.");
	default:
		return FString();
	}
}

FString UNBBaseballLibrary::DigitsToString(const TArray<int32>& Digits)
{
	FString Result;
	for (const int32 Digit : Digits)
	{
		Result.AppendInt(Digit);
	}
	return Result;
}
