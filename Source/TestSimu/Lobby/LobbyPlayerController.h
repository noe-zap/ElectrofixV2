#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class SLobbyWidget;
class USessionSubsystem;

UCLASS()
class TESTSIMU_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALobbyPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSendChatMessage(const FString& Message);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestStartGame();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void LeaveLobby();

private:
	void CreateLobbyUI();

	TSharedPtr<SLobbyWidget> LobbyWidget;
	TWeakObjectPtr<USessionSubsystem> SessionSubsystem;
};
