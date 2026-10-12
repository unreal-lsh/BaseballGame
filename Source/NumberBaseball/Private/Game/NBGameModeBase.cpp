#include "Game/NBGameModeBase.h"
#include "NumberBaseball.h"
#include "Game/NBGameStateBase.h"
#include "Library/NBBaseballLibrary.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"
#include "TimerManager.h"

ANBGameModeBase::ANBGameModeBase()
{
	GameStateClass = ANBGameStateBase::StaticClass();
	PlayerStateClass = ANBPlayerState::StaticClass();
	PlayerControllerClass = ANBPlayerController::StaticClass();

	// 채팅으로만 진행하는 게임이라 조종할 폰이 필요 없다.
	DefaultPawnClass = nullptr;
}

void ANBGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANBPlayerState* NewPlayerState = NewPlayer != nullptr ? NewPlayer->GetPlayerState<ANBPlayerState>() : nullptr;
	if (NewPlayerState == nullptr)
	{
		return;
	}

	NewPlayerState->SetPlayerName(FString::Printf(TEXT("Player%d"), ++JoinedPlayerCount));
	Participants.Add(NewPlayerState);

	BroadcastMessage(FString::Printf(TEXT("[시스템] %s 님이 입장했습니다. (현재 %d명)"), *NewPlayerState->GetPlayerName(), Participants.Num()));

	TryStartGame();
}

void ANBGameModeBase::Logout(AController* Exiting)
{
	ANBPlayerState* ExitingState = Exiting != nullptr ? Exiting->GetPlayerState<ANBPlayerState>() : nullptr;
	const int32 RemovedIndex = Participants.IndexOfByKey(ExitingState);

	if (RemovedIndex != INDEX_NONE)
	{
		const bool bWasTurnPlayer = (RemovedIndex == CurrentTurnIndex);
		Participants.RemoveAt(RemovedIndex);

		// 나간 사람이 현재 차례이거나 그 앞 순서였다면, 뒤 사람들이 한 칸씩 당겨졌으므로 인덱스도 당긴다.
		if (RemovedIndex <= CurrentTurnIndex)
		{
			--CurrentTurnIndex;
		}

		BroadcastMessage(FString::Printf(TEXT("[시스템] %s 님이 퇴장했습니다."), *ExitingState->GetPlayerName()));

		const ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
		if (NBGameState != nullptr && NBGameState->GetRoundState() == ENBRoundState::Playing)
		{
			if (Participants.Num() < MinPlayersToStart)
			{
				StopRoundForLackOfPlayers();
			}
			else if (JudgeGame(0) == ENBGameResult::Draw)
			{
				FinishRound(nullptr);
			}
			else if (bUseTurnSystem && bWasTurnPlayer)
			{
				GetWorldTimerManager().ClearTimer(TurnTimerHandle);
				GetWorldTimerManager().ClearTimer(TurnTransitionTimerHandle);
				StartNextTurn();
			}
		}
	}

	Super::Logout(Exiting);
}

void ANBGameModeBase::HandleChatMessage(ANBPlayerController* Sender, const FString& RawMessage)
{
	ANBPlayerState* SenderState = Sender != nullptr ? Sender->GetPlayerState<ANBPlayerState>() : nullptr;
	if (SenderState == nullptr)
	{
		return;
	}

	// 클라이언트가 보낸 값은 믿지 않는다. 공백만 있거나 너무 긴 메시지를 여기서 걸러 낸다.
	FString Message = RawMessage.TrimStartAndEnd();
	if (Message.IsEmpty())
	{
		return;
	}
	if (Message.Len() > MaxChatLength)
	{
		Message.LeftInline(MaxChatLength);
	}

	// 숫자로 시작하면 숫자 야구 입력으로 보고, 그 외에는 일반 채팅으로 모두에게 중계한다.
	const TCHAR FirstCharacter = Message[0];
	const bool bIsGuessAttempt = (FirstCharacter >= TEXT('0') && FirstCharacter <= TEXT('9'));

	if (bIsGuessAttempt)
	{
		HandleGuess(Sender, SenderState, Message);
	}
	else
	{
		BroadcastMessage(FString::Printf(TEXT("%s: %s"), *SenderState->GetPlayerName(), *Message));
	}
}

// ---------------------------------------------------------------------------
// 필수 : 규칙
// ---------------------------------------------------------------------------

void ANBGameModeBase::GenerateRandomNumbers()
{
	UNBBaseballLibrary::GenerateSecretNumber(SecretNumber);

	// 테스트할 때 정답을 확인할 수 있도록 서버 로그에만 남긴다.
	UE_LOG(LogNumberBaseball, Log, TEXT("[Server] 정답 생성: %s"), *UNBBaseballLibrary::DigitsToString(SecretNumber));
}

FString ANBGameModeBase::CheckAnswer(const TArray<int32>& GuessDigits, int32& OutStrikeCount) const
{
	int32 BallCount = 0;
	UNBBaseballLibrary::JudgeGuess(SecretNumber, GuessDigits, OutStrikeCount, BallCount);
	return UNBBaseballLibrary::MakeResultString(OutStrikeCount, BallCount);
}

ENBGameResult ANBGameModeBase::JudgeGame(int32 StrikeCount) const
{
	// 승리 : 3S 달성
	if (StrikeCount == UNBBaseballLibrary::NumberLength)
	{
		return ENBGameResult::Win;
	}

	// 무승부 : 모든 플레이어가 기회를 다 썼다
	if (Participants.Num() == 0)
	{
		return ENBGameResult::InProgress;
	}
	for (const ANBPlayerState* Participant : Participants)
	{
		if (IsValid(Participant) && Participant->HasRemainingChance())
		{
			return ENBGameResult::InProgress;
		}
	}
	return ENBGameResult::Draw;
}

void ANBGameModeBase::ResetGame()
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (NBGameState == nullptr)
	{
		return;
	}

	ClearAllTimers();
	bTurnInputOpen = false;
	CurrentTurnIndex = INDEX_NONE;
	NBGameState->SetCurrentTurnPlayerState(nullptr);
	NBGameState->SetRemainingTurnTime(0);

	// 결과 공지가 떠 있는 동안 인원이 줄었다면 시작하지 않고 기다린다.
	if (Participants.Num() < MinPlayersToStart)
	{
		NBGameState->SetRoundState(ENBRoundState::WaitingForPlayers);
		return;
	}

	GenerateRandomNumbers();

	for (ANBPlayerState* Participant : Participants)
	{
		if (IsValid(Participant))
		{
			Participant->ResetForNewRound();
		}
	}

	NBGameState->SetTurnDuration(bUseTurnSystem ? TurnDuration : 0);
	NBGameState->SetRoundState(ENBRoundState::Playing);

	BroadcastMessage(TEXT("[시스템] 새 게임 시작! 1~9 사이의 중복 없는 3자리 숫자를 맞혀 보세요."));

	if (bUseTurnSystem)
	{
		StartNextTurn();
	}
}

// ---------------------------------------------------------------------------
// 필수 : 진행
// ---------------------------------------------------------------------------

void ANBGameModeBase::TryStartGame()
{
	const ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (NBGameState == nullptr || NBGameState->GetRoundState() != ENBRoundState::WaitingForPlayers)
	{
		return;
	}
	if (Participants.Num() < MinPlayersToStart || GetWorldTimerManager().IsTimerActive(RoundStartTimerHandle))
	{
		return;
	}

	// 방금 접속한 클라이언트가 GameState를 복제받을 시간을 준 뒤 시작한다.
	BroadcastMessage(TEXT("[시스템] 인원이 모였습니다. 곧 게임을 시작합니다."));
	GetWorldTimerManager().SetTimer(RoundStartTimerHandle, this, &ANBGameModeBase::ResetGame, StartDelay, false);
}

void ANBGameModeBase::HandleGuess(ANBPlayerController* Sender, ANBPlayerState* SenderState, const FString& Input)
{
	// 1) 지금 입력을 받을 수 있는 상황인가? (게임 중인지, 기회가 남았는지, 내 차례인지, 시간이 남았는지)
	FString RejectReason;
	if (!CanAcceptGuess(SenderState, RejectReason))
	{
		Sender->ClientReceiveSystemMessage(RejectReason);
		return;
	}

	// 2) 형식이 올바른가? 틀려도 기회는 소진하지 않는다.
	TArray<int32> GuessDigits;
	const ENBGuessValidation Validation = UNBBaseballLibrary::ValidateGuess(Input, GuessDigits);
	if (Validation != ENBGuessValidation::Valid)
	{
		Sender->ClientReceiveSystemMessage(UNBBaseballLibrary::GetValidationMessage(Validation));
		return;
	}

	// 3) 기회를 1회 쓰고 판정한다.
	SenderState->AddGuessCount();
	SenderState->SetHasGuessedThisTurn(true);

	int32 StrikeCount = 0;
	const FString Result = CheckAnswer(GuessDigits, StrikeCount);

	BroadcastMessage(FString::Printf(TEXT("%s: %s → %s %s"),
		*SenderState->GetPlayerName(), *Input, *Result, *SenderState->GetGuessCountString()));

	// 4) 게임 결과를 판단한다.
	switch (JudgeGame(StrikeCount))
	{
	case ENBGameResult::Win:
		FinishRound(SenderState);
		break;

	case ENBGameResult::Draw:
		FinishRound(nullptr);
		break;

	default:
		if (bUseTurnSystem)
		{
			EndCurrentTurn();
		}
		else if (!SenderState->HasRemainingChance())
		{
			Sender->ClientReceiveSystemMessage(TEXT("기회를 모두 사용했습니다. 다른 플레이어의 결과를 기다려 주세요."));
		}
		break;
	}
}

bool ANBGameModeBase::CanAcceptGuess(const ANBPlayerState* SenderState, FString& OutReason) const
{
	const ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (NBGameState == nullptr || NBGameState->GetRoundState() != ENBRoundState::Playing)
	{
		OutReason = TEXT("지금은 게임이 진행 중이 아닙니다.");
		return false;
	}

	if (!SenderState->HasRemainingChance())
	{
		OutReason = FString::Printf(TEXT("기회를 모두 사용해 더 이상 입력할 수 없습니다. %s"), *SenderState->GetGuessCountString());
		return false;
	}

	if (!bUseTurnSystem)
	{
		return true;
	}

	// 여기부터는 턴제(도전 기능) 전용 검사
	const ANBPlayerState* TurnPlayer = GetCurrentTurnPlayer();
	if (TurnPlayer != SenderState && !bTurnInputOpen)
	{
		OutReason = TEXT("다음 턴을 준비 중입니다. 잠시만 기다려 주세요.");
		return false;
	}
	if (TurnPlayer != SenderState)
	{
		OutReason = TurnPlayer != nullptr
			? FString::Printf(TEXT("지금은 %s 님의 차례입니다."), *TurnPlayer->GetPlayerName())
			: FString(TEXT("아직 차례가 정해지지 않았습니다."));
		return false;
	}

	if (SenderState->HasGuessedThisTurn())
	{
		OutReason = TEXT("이번 턴에는 이미 입력했습니다.");
		return false;
	}

	if (!bTurnInputOpen || NBGameState->GetRemainingTurnTime() <= 0)
	{
		OutReason = TEXT("시간이 종료되어 입력할 수 없습니다.");
		return false;
	}

	return true;
}

void ANBGameModeBase::FinishRound(const ANBPlayerState* Winner)
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (NBGameState == nullptr)
	{
		return;
	}

	ClearAllTimers();
	bTurnInputOpen = false;
	NBGameState->SetRoundState(ENBRoundState::Finished);
	NBGameState->SetCurrentTurnPlayerState(nullptr);
	NBGameState->SetRemainingTurnTime(0);

	const FString SecretString = UNBBaseballLibrary::DigitsToString(SecretNumber);
	const FString NoticeMessage = Winner != nullptr
		? FString::Printf(TEXT("%s 승리! (정답: %s)"), *Winner->GetPlayerName(), *SecretString)
		: FString::Printf(TEXT("무승부! (정답: %s)"), *SecretString);

	// 공지 위젯으로 결과를 알리고, 채팅창에도 기록을 남긴다.
	NBGameState->MulticastShowNotice(NoticeMessage, RestartDelay);
	BroadcastMessage(FString::Printf(TEXT("[결과] %s 잠시 후 새 게임이 시작됩니다."), *NoticeMessage));

	GetWorldTimerManager().SetTimer(RoundStartTimerHandle, this, &ANBGameModeBase::ResetGame, RestartDelay, false);
}

void ANBGameModeBase::StopRoundForLackOfPlayers()
{
	ClearAllTimers();
	bTurnInputOpen = false;
	CurrentTurnIndex = INDEX_NONE;

	if (ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>())
	{
		NBGameState->SetRoundState(ENBRoundState::WaitingForPlayers);
		NBGameState->SetCurrentTurnPlayerState(nullptr);
		NBGameState->SetRemainingTurnTime(0);
	}

	BroadcastMessage(TEXT("[시스템] 인원이 부족해 게임을 중단합니다. 다른 플레이어를 기다립니다."));
}

void ANBGameModeBase::BroadcastMessage(const FString& Message) const
{
	UE_LOG(LogNumberBaseball, Log, TEXT("[Chat] %s"), *Message);

	if (ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>())
	{
		NBGameState->MulticastChatMessage(Message);
	}
}

// ---------------------------------------------------------------------------
// 도전 : 턴
// ---------------------------------------------------------------------------

void ANBGameModeBase::StartNextTurn()
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (NBGameState == nullptr || NBGameState->GetRoundState() != ENBRoundState::Playing)
	{
		return;
	}

	const int32 NextTurnIndex = FindNextTurnIndex();
	if (NextTurnIndex == INDEX_NONE)
	{
		// 기회가 남은 사람이 아무도 없다.
		FinishRound(nullptr);
		return;
	}

	CurrentTurnIndex = NextTurnIndex;
	ANBPlayerState* TurnPlayer = Participants[CurrentTurnIndex];

	// 활동 체크 변수를 초기화하고 입력을 연다.
	TurnPlayer->SetHasGuessedThisTurn(false);
	bTurnInputOpen = true;

	NBGameState->SetCurrentTurnPlayerState(TurnPlayer);
	NBGameState->SetRemainingTurnTime(TurnDuration);

	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ANBGameModeBase::TickTurnTimer, 1.0f, true);

	BroadcastMessage(FString::Printf(TEXT("[턴] %s 님의 차례입니다. (%d초) %s"),
		*TurnPlayer->GetPlayerName(), TurnDuration, *TurnPlayer->GetGuessCountString()));
}

void ANBGameModeBase::TickTurnTimer()
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (NBGameState == nullptr)
	{
		return;
	}

	// 남은 시간은 서버가 관리하고, 복제를 통해 모든 클라이언트의 타이머 위젯에 반영된다.
	const int32 NewRemainingTime = NBGameState->GetRemainingTurnTime() - 1;
	NBGameState->SetRemainingTurnTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		HandleTurnTimeout();
	}
}

void ANBGameModeBase::HandleTurnTimeout()
{
	ANBPlayerState* TurnPlayer = GetCurrentTurnPlayer();

	// 이번 턴에 숫자를 입력하지 않았다면 기회를 1회 소진시킨다.
	if (TurnPlayer != nullptr && !TurnPlayer->HasGuessedThisTurn())
	{
		TurnPlayer->AddGuessCount();
		BroadcastMessage(FString::Printf(TEXT("[시간 초과] %s 님이 입력하지 못해 기회가 1회 소진됩니다. %s"),
			*TurnPlayer->GetPlayerName(), *TurnPlayer->GetGuessCountString()));
	}

	if (JudgeGame(0) == ENBGameResult::Draw)
	{
		FinishRound(nullptr);
	}
	else
	{
		EndCurrentTurn();
	}
}

void ANBGameModeBase::EndCurrentTurn()
{
	// 입력을 닫고 타이머를 멈춘다. 이 시점부터 다음 턴이 시작될 때까지는 누구의 숫자도 받지 않는다.
	bTurnInputOpen = false;
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	// 화면에는 "다음 턴 준비 중"으로 보이도록 턴 표시를 비운다. (서버의 CurrentTurnIndex는 그대로 둔다)
	if (ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>())
	{
		NBGameState->SetCurrentTurnPlayerState(nullptr);
		NBGameState->SetRemainingTurnTime(0);
	}

	GetWorldTimerManager().SetTimer(TurnTransitionTimerHandle, this, &ANBGameModeBase::StartNextTurn, TurnTransitionDelay, false);
}

int32 ANBGameModeBase::FindNextTurnIndex() const
{
	const int32 ParticipantCount = Participants.Num();
	for (int32 Offset = 1; Offset <= ParticipantCount; ++Offset)
	{
		// CurrentTurnIndex가 INDEX_NONE(-1)이어도 0번부터 찾도록 음수를 보정한다.
		const int32 CandidateIndex = ((CurrentTurnIndex + Offset) % ParticipantCount + ParticipantCount) % ParticipantCount;
		const ANBPlayerState* Candidate = Participants[CandidateIndex];
		if (IsValid(Candidate) && Candidate->HasRemainingChance())
		{
			return CandidateIndex;
		}
	}
	return INDEX_NONE;
}

ANBPlayerState* ANBGameModeBase::GetCurrentTurnPlayer() const
{
	return Participants.IsValidIndex(CurrentTurnIndex) ? Participants[CurrentTurnIndex].Get() : nullptr;
}

void ANBGameModeBase::ClearAllTimers()
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(RoundStartTimerHandle);
	TimerManager.ClearTimer(TurnTimerHandle);
	TimerManager.ClearTimer(TurnTransitionTimerHandle);
}
