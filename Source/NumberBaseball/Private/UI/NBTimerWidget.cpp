#include "UI/NBTimerWidget.h"
#include "Game/NBGameStateBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UNBTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWorld* World = GetWorld();
	const ANBGameStateBase* NBGameState = World != nullptr ? World->GetGameState<ANBGameStateBase>() : nullptr;
	if (NBGameState == nullptr)
	{
		// 클라이언트는 접속 직후 GameState가 아직 복제되지 않았을 수 있다.
		return;
	}

	switch (NBGameState->GetRoundState())
	{
	case ENBRoundState::WaitingForPlayers:
		SetTexts(TEXT("-"), TEXT("다른 플레이어를 기다리는 중..."), 0.0f);
		break;

	case ENBRoundState::Finished:
		SetTexts(TEXT("0"), TEXT("게임 종료! 잠시 후 다시 시작합니다."), 0.0f);
		break;

	case ENBRoundState::Playing:
	{
		const int32 TurnDuration = NBGameState->GetTurnDuration();
		if (TurnDuration <= 0)
		{
			// 턴제를 끈 경우 : 시간 제한 없이 누구나 입력할 수 있다.
			SetTexts(TEXT("-"), TEXT("숫자를 입력하세요!"), 0.0f);
			break;
		}

		const int32 RemainingTime = NBGameState->GetRemainingTurnTime();
		const APlayerState* TurnPlayerState = NBGameState->GetCurrentTurnPlayerState();
		const APlayerController* OwningPlayer = GetOwningPlayer();
		const APlayerState* MyPlayerState = OwningPlayer != nullptr ? OwningPlayer->GetPlayerState<APlayerState>() : nullptr;

		FString TurnMessage;
		if (TurnPlayerState == nullptr)
		{
			TurnMessage = TEXT("다음 턴 준비 중...");
		}
		else if (TurnPlayerState == MyPlayerState)
		{
			TurnMessage = TEXT("내 차례입니다! 숫자를 입력하세요.");
		}
		else
		{
			TurnMessage = FString::Printf(TEXT("%s 님의 차례"), *TurnPlayerState->GetPlayerName());
		}

		SetTexts(FString::FromInt(RemainingTime), TurnMessage, static_cast<float>(RemainingTime) / static_cast<float>(TurnDuration));
		break;
	}

	default:
		break;
	}
}

void UNBTimerWidget::SetTexts(const FString& InTimerText, const FString& InTurnText, float InBarPercent)
{
	if (TimerText != nullptr)
	{
		TimerText->SetText(FText::FromString(InTimerText));
	}
	if (TurnText != nullptr)
	{
		TurnText->SetText(FText::FromString(InTurnText));
	}
	if (TimerBar != nullptr)
	{
		TimerBar->SetPercent(FMath::Clamp(InBarPercent, 0.0f, 1.0f));
	}
}
