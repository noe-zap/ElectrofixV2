#include "Lobby/LobbyPlayerController.h"
#include "Lobby/LobbyGameState.h"
#include "Lobby/LobbyGameMode.h"
#include "UI/SLobbyWidget.h"
#include "Online/SessionSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWeakWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

ALobbyPlayerController::ALobbyPlayerController()
{
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		SetShowMouseCursor(true);

		UGameInstance* GI = GetGameInstance();
		if (GI)
		{
			SessionSubsystem = GI->GetSubsystem<USessionSubsystem>();
		}

		CreateLobbyUI();
	}
}

void ALobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine && GEngine->GameViewport && LobbyWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LobbyWidget.ToSharedRef());
	}
	LobbyWidget.Reset();

	if (IsLocalController())
	{
		FInputModeGameOnly GameInput;
		SetInputMode(GameInput);
		SetShowMouseCursor(false);
	}

	Super::EndPlay(EndPlayReason);
}

void ALobbyPlayerController::CreateLobbyUI()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	bool bIsHost = false;
	if (SessionSubsystem.IsValid())
	{
		bIsHost = SessionSubsystem->IsHosting();
	}

	SAssignNew(LobbyWidget, SLobbyWidget)
		.bIsHost(bIsHost);

	GEngine->GameViewport->AddViewportWidgetContent(
		SNew(SWeakWidget).PossiblyNullContent(LobbyWidget.ToSharedRef())
	);
}

bool ALobbyPlayerController::ServerSendChatMessage_Validate(const FString& Message)
{
	return !Message.IsEmpty() && Message.Len() <= 500;
}

void ALobbyPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	ALobbyGameState* LobbyState = GetWorld()->GetGameState<ALobbyGameState>();
	if (LobbyState && PlayerState)
	{
		LobbyState->AddChatMessage(PlayerState->GetPlayerName(), Message);
	}
}

bool ALobbyPlayerController::ServerRequestStartGame_Validate()
{
	return true;
}

void ALobbyPlayerController::ServerRequestStartGame_Implementation()
{
	ALobbyGameMode* LobbyMode = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
	if (LobbyMode)
	{
		APlayerState* PS = PlayerState;
		if (PS && GetWorld()->GetGameState())
		{
			const auto& Players = GetWorld()->GetGameState()->PlayerArray;
			if (Players.Num() > 0 && Players[0] == PS)
			{
				LobbyMode->StartGame();
			}
		}
	}
}

void ALobbyPlayerController::LeaveLobby()
{
	if (!IsLocalController())
	{
		return;
	}

	if (SessionSubsystem.IsValid())
	{
		SessionSubsystem->DestroySession();
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenuMap")));
}
