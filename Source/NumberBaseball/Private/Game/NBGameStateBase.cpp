#include "Game/NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void ANBGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBGameStateBase, RoundState);
	DOREPLIFETIME(ANBGameStateBase, RemainingTurnTime);
	DOREPLIFETIME(ANBGameStateBase, TurnDuration);
	DOREPLIFETIME(ANBGameStateBase, CurrentTurnPlayerState);
}

void ANBGameStateBase::MulticastChatMessage_Implementation(const FString& Message)
{
	// 서버와 모든 클라이언트에서 실행된다. 각 머신은 자기 화면의 채팅창에만 추가하면 된다.
	if (ANBPlayerController* LocalController = FindLocalPlayerController())
	{
		LocalController->AddChatMessage(Message);
	}
}

void ANBGameStateBase::MulticastShowNotice_Implementation(const FString& Message, float Duration)
{
	if (ANBPlayerController* LocalController = FindLocalPlayerController())
	{
		LocalController->ShowNotice(Message, Duration);
	}
}

ANBPlayerController* ANBGameStateBase::FindLocalPlayerController() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		ANBPlayerController* Controller = Cast<ANBPlayerController>(Iterator->Get());
		if (Controller != nullptr && Controller->IsLocalController())
		{
			return Controller;
		}
	}
	return nullptr;
}
