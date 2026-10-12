#include "Player/NBPlayerState.h"
#include "Net/UnrealNetwork.h"

void ANBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBPlayerState, CurrentGuessCount);
	DOREPLIFETIME(ANBPlayerState, MaxGuessCount);
}

FString ANBPlayerState::GetGuessCountString() const
{
	return FString::Printf(TEXT("[%d / %d]"), CurrentGuessCount, MaxGuessCount);
}

void ANBPlayerState::AddGuessCount()
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentGuessCount = FMath::Min(CurrentGuessCount + 1, MaxGuessCount);
}

void ANBPlayerState::ResetForNewRound()
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentGuessCount = 0;
	bHasGuessedThisTurn = false;
}
