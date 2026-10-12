#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBGameModeBase.generated.h"

class ANBGameStateBase;
class ANBPlayerController;
class ANBPlayerState;

/** JudgeGame이 돌려주는 현재 게임 결과 */
UENUM(BlueprintType)
enum class ENBGameResult : uint8
{
	InProgress,
	Win,
	Draw
};

/**
 * 숫자 야구의 심판. 서버에만 존재하므로 정답과 모든 판정이 여기서만 이루어진다.
 *
 * [필수] 채팅 중계, 정답 생성, 입력 검사, S/B 판정, 승리/무승부 판정, 리셋
 * [도전] 턴 순서, 턴 타이머, 시간 초과 시 기회 소진과 턴 전환
 */
UCLASS()
class NUMBERBASEBALL_API ANBGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANBGameModeBase();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** PlayerController의 Server RPC가 넘겨준 입력을 처리한다. 숫자로 시작하면 숫자 야구 입력, 아니면 일반 채팅이다. */
	void HandleChatMessage(ANBPlayerController* Sender, const FString& RawMessage);

protected:
	// ---------- 필수 : 규칙 ----------

	/** 1~9 중복 없는 3자리 정답을 새로 만든다. */
	void GenerateRandomNumbers();

	/** 입력 숫자를 정답과 비교해 "1S1B", "OUT" 같은 결과 문자열을 돌려준다. */
	FString CheckAnswer(const TArray<int32>& GuessDigits, int32& OutStrikeCount) const;

	/** 방금 나온 스트라이크 수와 모든 플레이어의 남은 기회로 현재 게임 결과를 판단한다. */
	ENBGameResult JudgeGame(int32 StrikeCount) const;

	/** 모든 데이터를 초기화하고 새 정답으로 게임을 다시 시작한다. */
	void ResetGame();

private:
	// ---------- 필수 : 진행 ----------

	/** 인원이 모였으면 잠시 뒤 게임을 시작한다. */
	void TryStartGame();

	/** 숫자 야구 입력 한 건을 처리한다. */
	void HandleGuess(ANBPlayerController* Sender, ANBPlayerState* SenderState, const FString& Input);

	/** 지금 이 플레이어의 숫자 입력을 받아도 되는지 검사한다. 안 되면 OutReason에 이유를 담는다. */
	bool CanAcceptGuess(const ANBPlayerState* SenderState, FString& OutReason) const;

	/** 결과를 공지하고 리셋을 예약한다. Winner가 nullptr이면 무승부다. */
	void FinishRound(const ANBPlayerState* Winner);

	/** 인원 부족으로 진행 중인 게임을 멈추고 대기 상태로 돌아간다. */
	void StopRoundForLackOfPlayers();

	/** 모든 클라이언트의 채팅창에 메시지를 보낸다. */
	void BroadcastMessage(const FString& Message) const;

	// ---------- 도전 : 턴 ----------

	/** 다음 차례 플레이어에게 턴을 넘기고 타이머를 시작한다. */
	void StartNextTurn();

	/** 1초마다 남은 시간을 줄인다. 0이 되면 시간 초과를 처리한다. */
	void TickTurnTimer();

	/** 시간 초과 : 입력하지 않았다면 기회를 1회 소진시키고 턴을 넘긴다. */
	void HandleTurnTimeout();

	/** 현재 턴의 입력을 닫고, 잠시 뒤 다음 턴이 시작되도록 예약한다. */
	void EndCurrentTurn();

	/** 현재 차례 다음부터 돌면서 기회가 남은 플레이어의 인덱스를 찾는다. 없으면 INDEX_NONE. */
	int32 FindNextTurnIndex() const;

	ANBPlayerState* GetCurrentTurnPlayer() const;

	void ClearAllTimers();

	// ---------- 설정 ----------

	/** 게임을 시작하는 데 필요한 최소 인원 */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Rule", meta = (ClampMin = "1"))
	int32 MinPlayersToStart = 2;

	/** 인원이 모인 뒤 게임이 시작되기까지의 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Rule", meta = (ClampMin = "0.1"))
	float StartDelay = 3.0f;

	/** 결과 공지를 보여 준 뒤 리셋하기까지의 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Rule", meta = (ClampMin = "0.1"))
	float RestartDelay = 5.0f;

	/** 채팅 한 줄의 최대 글자 수 */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Rule", meta = (ClampMin = "1"))
	int32 MaxChatLength = 100;

	/** true면 턴제(도전 기능), false면 누구나 아무 때나 입력하는 방식(필수 기능만) */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Turn")
	bool bUseTurnSystem = true;

	/** 한 턴의 제한 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Turn", meta = (ClampMin = "1"))
	int32 TurnDuration = 15;

	/** 턴이 끝난 뒤 다음 턴이 시작되기까지의 시간(초). 이 동안에는 누구의 입력도 받지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "NumberBaseball|Turn", meta = (ClampMin = "0.1"))
	float TurnTransitionDelay = 1.5f;

	// ---------- 상태 (서버 전용) ----------

	/** 서버만 아는 정답. 클라이언트로 복제하지 않는다. */
	TArray<int32> SecretNumber;

	/** 접속 순서대로 담긴 참가자 목록. 턴 순서로도 쓴다. */
	UPROPERTY()
	TArray<TObjectPtr<ANBPlayerState>> Participants;

	/** Participants에서 현재 차례인 플레이어의 인덱스 */
	int32 CurrentTurnIndex = INDEX_NONE;

	/** 현재 턴에 입력을 받는 중인지 여부. 시간이 0초가 되거나 입력이 끝나면 false가 된다. */
	bool bTurnInputOpen = false;

	/** 플레이어 이름(Player1, Player2...)에 붙일 일련번호 */
	int32 JoinedPlayerCount = 0;

	FTimerHandle RoundStartTimerHandle;
	FTimerHandle TurnTimerHandle;
	FTimerHandle TurnTransitionTimerHandle;
};
