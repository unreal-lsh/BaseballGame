#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NBBaseballLibrary.generated.h"

/** 플레이어가 입력한 숫자에 대한 유효성 검사 결과 */
UENUM(BlueprintType)
enum class ENBGuessValidation : uint8
{
	Valid,
	NotNumeric,   // 숫자가 아닌 문자가 포함됨
	WrongLength,  // 3자리가 아님
	OutOfRange,   // 0이 포함됨 (1~9만 허용)
	Duplicated    // 같은 숫자가 중복됨
};

/**
 * 숫자 야구의 순수 규칙(정답 생성, 입력 검사, S/B 판정)을 모아 둔 함수 라이브러리.
 * 월드나 네트워크 상태에 의존하지 않으므로 어디서든 호출할 수 있다.
 */
UCLASS()
class NUMBERBASEBALL_API UNBBaseballLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 정답의 자릿수 */
	static constexpr int32 NumberLength = 3;

	/** 1~9 사이에서 중복 없이 NumberLength개의 숫자를 뽑는다. */
	UFUNCTION(BlueprintCallable, Category = "NumberBaseball")
	static void GenerateSecretNumber(TArray<int32>& OutSecretNumber);

	/** 입력 문자열이 규칙에 맞는지 검사하고, 맞으면 각 자리 숫자를 OutDigits에 담는다. */
	UFUNCTION(BlueprintCallable, Category = "NumberBaseball")
	static ENBGuessValidation ValidateGuess(const FString& Input, TArray<int32>& OutDigits);

	/** 정답과 입력을 비교해 스트라이크/볼 개수를 센다. */
	UFUNCTION(BlueprintCallable, Category = "NumberBaseball")
	static void JudgeGuess(const TArray<int32>& SecretNumber, const TArray<int32>& GuessDigits, int32& OutStrikeCount, int32& OutBallCount);

	/** 판정 결과를 "1S1B", "OUT" 형태의 문자열로 만든다. */
	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	static FString MakeResultString(int32 StrikeCount, int32 BallCount);

	/** 유효성 검사 실패 사유를 플레이어에게 보여 줄 안내 문구로 바꾼다. */
	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	static FString GetValidationMessage(ENBGuessValidation Validation);

	/** 숫자 배열을 "386" 같은 문자열로 만든다. */
	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	static FString DigitsToString(const TArray<int32>& Digits);
};
