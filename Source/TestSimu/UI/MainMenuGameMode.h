#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class SMainMenuWidget;
class USessionSubsystem;

UCLASS()
class TESTSIMU_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TSharedPtr<SMainMenuWidget> GetMainMenuWidget() const { return MainMenuWidget; }

private:
	UFUNCTION()
	void OnSessionCreated(bool bWasSuccessful);

	UFUNCTION()
	void OnSessionJoined(bool bWasSuccessful, const FString& ErrorMessage);

	TSharedPtr<SMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<USessionSubsystem> SessionSubsystem;
};
