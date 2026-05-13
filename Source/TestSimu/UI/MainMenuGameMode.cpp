#include "UI/MainMenuGameMode.h"
#include "UI/SMainMenuWidget.h"
#include "Online/SessionSubsystem.h"
#include "Widgets/SWeakWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		SessionSubsystem = GI->GetSubsystem<USessionSubsystem>();
		if (SessionSubsystem)
		{
			SessionSubsystem->OnSessionCreated.AddDynamic(this, &AMainMenuGameMode::OnSessionCreated);
			SessionSubsystem->OnSessionJoined.AddDynamic(this, &AMainMenuGameMode::OnSessionJoined);
		}
	}

	SAssignNew(MainMenuWidget, SMainMenuWidget);

	if (GEngine && GEngine->GameViewport && MainMenuWidget.IsValid())
	{
		GEngine->GameViewport->AddViewportWidgetContent(
			SNew(SWeakWidget).PossiblyNullContent(MainMenuWidget.ToSharedRef())
		);

		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
		}
	}
}

void AMainMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SessionSubsystem)
	{
		SessionSubsystem->OnSessionCreated.RemoveDynamic(this, &AMainMenuGameMode::OnSessionCreated);
		SessionSubsystem->OnSessionJoined.RemoveDynamic(this, &AMainMenuGameMode::OnSessionJoined);
	}

	if (GEngine && GEngine->GameViewport && MainMenuWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MainMenuWidget.ToSharedRef());
	}
	MainMenuWidget.Reset();

	Super::EndPlay(EndPlayReason);
}

void AMainMenuGameMode::OnSessionCreated(bool bWasSuccessful)
{
	if (MainMenuWidget.IsValid()) MainMenuWidget->OnSessionCreated(bWasSuccessful);
}

void AMainMenuGameMode::OnSessionJoined(bool bWasSuccessful, const FString& ErrorMessage)
{
	if (MainMenuWidget.IsValid()) MainMenuWidget->OnSessionJoined(bWasSuccessful);
}
