#include "Lobby/LobbyGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"

ALobbyGameState::ALobbyGameState()
{
	bReplicates = true;
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALobbyGameState, ChatMessages);
}

void ALobbyGameState::OnRep_ChatMessages()
{
	if (ChatMessages.Num() > LastChatCount)
	{
		for (int32 i = LastChatCount; i < ChatMessages.Num(); i++)
		{
			OnChatMessageReceived.Broadcast(ChatMessages[i]);
		}
		LastChatCount = ChatMessages.Num();
	}
}

void ALobbyGameState::AddChatMessage(const FString& SenderName, const FString& Message)
{
	if (!HasAuthority())
	{
		return;
	}

	FChatMessage NewMessage;
	NewMessage.SenderName = SenderName;
	NewMessage.Message = Message;
	NewMessage.Timestamp = GetWorld()->GetTimeSeconds();

	ChatMessages.Add(NewMessage);

	while (ChatMessages.Num() > 50)
	{
		ChatMessages.RemoveAt(0);
	}

	OnChatMessageReceived.Broadcast(NewMessage);
	LastChatCount = ChatMessages.Num();
}

TArray<FString> ALobbyGameState::GetPlayerNames() const
{
	TArray<APlayerState*> SortedPlayers;
	for (APlayerState* PS : PlayerArray)
	{
		if (PS)
		{
			SortedPlayers.Add(PS);
		}
	}
	SortedPlayers.Sort([](const APlayerState& A, const APlayerState& B)
	{
		return A.GetPlayerId() < B.GetPlayerId();
	});

	TArray<FString> Names;
	for (APlayerState* PS : SortedPlayers)
	{
		Names.Add(PS->GetPlayerName());
	}
	return Names;
}

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	OnPlayerListChanged.Broadcast();
}

void ALobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	OnPlayerListChanged.Broadcast();
}
