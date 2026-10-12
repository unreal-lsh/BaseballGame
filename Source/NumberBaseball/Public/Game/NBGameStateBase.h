#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NBGameStateBase.generated.h"

class ANBPlayerController;
class APlayerState;

/** 라운드 진행 단계 */
UENUM(BlueprintType)
enum class ENBRoundState : uint8
{
	WaitingForPlayers, // 인원이 모이기를 기다리는 중
	Playing,           // 게임 진행 중
	Finished           // 결과 공지 중 (곧 리셋)
};

/**
 * 모든 클라이언트가 함께 봐야 하는 게임 정보를 담는다.
 * - 복제 변수 : 라운드 단계, 현재 턴 플레이어, 남은 턴 시간
 * - NetMulticast RPC : 채팅 한 줄, 결과 공지
 * GameMode는 서버에만 있으므로, 클라이언트에 알릴 내용은 전부 이 클래스를 거친다.
 */
UCLASS()
class NUMBERBASEBALL_API ANBGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 모든 클라이언트의 채팅창에 한 줄을 추가한다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastChatMessage(const FString& Message);

	/** 모든 클라이언트에 공지 위젯을 띄운다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowNotice(const FString& Message, float Duration);

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	ENBRoundState GetRoundState() const { return RoundState; }

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	int32 GetRemainingTurnTime() const { return RemainingTurnTime; }

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	int32 GetTurnDuration() const { return TurnDuration; }

	UFUNCTION(BlueprintPure, Category = "NumberBaseball")
	APlayerState* GetCurrentTurnPlayerState() const { return CurrentTurnPlayerState; }

	// 아래 Setter는 서버(GameMode)에서만 호출한다.
	void SetRoundState(ENBRoundState NewState) { RoundState = NewState; }
	void SetRemainingTurnTime(int32 NewTime) { RemainingTurnTime = FMath::Max(NewTime, 0); }
	void SetTurnDuration(int32 NewDuration) { TurnDuration = FMath::Max(NewDuration, 0); }
	void SetCurrentTurnPlayerState(APlayerState* NewTurnPlayer) { CurrentTurnPlayerState = NewTurnPlayer; }

private:
	/** 이 머신에서 화면을 가진 플레이어 컨트롤러를 찾는다. (데디케이티드 서버에서는 nullptr) */
	ANBPlayerController* FindLocalPlayerController() const;

	UPROPERTY(Replicated)
	ENBRoundState RoundState = ENBRoundState::WaitingForPlayers;

	/** 현재 턴의 남은 시간(초). 서버가 1초마다 줄인다. */
	UPROPERTY(Replicated)
	int32 RemainingTurnTime = 0;

	/** 한 턴의 제한 시간(초). 0이면 턴제를 쓰지 않는다는 뜻. */
	UPROPERTY(Replicated)
	int32 TurnDuration = 0;

	/** 지금 숫자를 입력할 수 있는 플레이어 */
	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> CurrentTurnPlayerState;
};
