#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NBPlayerState.generated.h"

/**
 * 플레이어 한 명의 시도 횟수를 관리한다.
 * 값은 서버에서만 바뀌고, 복제(Replication)를 통해 모든 클라이언트가 같은 값을 본다.
 */
UCLASS()
class NUMBERBASEBALL_API ANBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	int32 GetCurrentGuessCount() const { return CurrentGuessCount; }

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	int32 GetMaxGuessCount() const { return MaxGuessCount; }

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	int32 GetRemainingGuessCount() const { return FMath::Max(MaxGuessCount - CurrentGuessCount, 0); }

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	bool HasRemainingChance() const { return CurrentGuessCount < MaxGuessCount; }

	/** "[현재 시도 횟수 / 최대 시도 횟수]" 형태의 문자열 */
	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	FString GetGuessCountString() const;

	/** 시도 횟수를 1 올린다. (서버 전용) */
	void AddGuessCount();

	/** 새 라운드를 위해 시도 횟수와 턴 활동 기록을 초기화한다. (서버 전용) */
	void ResetForNewRound();

	bool HasGuessedThisTurn() const { return bHasGuessedThisTurn; }
	void SetHasGuessedThisTurn(bool bInHasGuessed) { bHasGuessedThisTurn = bInHasGuessed; }

private:
	/** 현재 시도 횟수 */
	UPROPERTY(Replicated)
	int32 CurrentGuessCount = 0;

	/** 최대 시도 횟수 */
	UPROPERTY(Replicated, EditDefaultsOnly, Category = "NumberBaseball")
	int32 MaxGuessCount = 3;

	/** 이번 턴에 숫자를 입력했는지 여부. 서버의 턴 판정에만 쓰므로 복제하지 않는다. */
	bool bHasGuessedThisTurn = false;
};
