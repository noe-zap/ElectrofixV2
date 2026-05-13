#include "Lobby/LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

ALobbyPlayerState::ALobbyPlayerState()
{
	bReplicates = true;
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
}

void ALobbyPlayerState::OnRep_bIsReady()
{
}

void ALobbyPlayerState::SetReady(bool bReady)
{
	if (HasAuthority())
	{
		bIsReady = bReady;
	}
}
