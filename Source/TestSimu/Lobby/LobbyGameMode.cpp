#include "Lobby/LobbyGameMode.h"
#include "Lobby/LobbyGameState.h"
#include "Lobby/LobbyPlayerState.h"
#include "Lobby/LobbyPlayerController.h"
#include "Online/SessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

ALobbyGameMode::ALobbyGameMode()
{
	GameStateClass = ALobbyGameState::StaticClass();
	PlayerStateClass = ALobbyPlayerState::StaticClass();
	PlayerControllerClass = ALobbyPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ALobbyGameState* LobbyState = GetGameState<ALobbyGameState>();
	if (LobbyState && NewPlayer && NewPlayer->PlayerState)
	{
		FString PlayerName = NewPlayer->PlayerState->GetPlayerName();
		LobbyState->AddChatMessage(TEXT("System"), FString::Printf(TEXT("%s joined the lobby"), *PlayerName));
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	if (!bGameStarting)
	{
		ALobbyGameState* LobbyState = GetGameState<ALobbyGameState>();
		APlayerController* PC = Cast<APlayerController>(Exiting);
		if (LobbyState && PC && PC->PlayerState)
		{
			FString PlayerName = PC->PlayerState->GetPlayerName();
			LobbyState->AddChatMessage(TEXT("System"), FString::Printf(TEXT("%s left the lobby"), *PlayerName));
		}
	}

	Super::Logout(Exiting);
}

void ALobbyGameMode::StartGame()
{
	if (!HasAuthority())
	{
		return;
	}

	bGameStarting = true;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (USessionSubsystem* Sessions = GI->GetSubsystem<USessionSubsystem>())
		{
			Sessions->StartGame();
		}
	}
}
