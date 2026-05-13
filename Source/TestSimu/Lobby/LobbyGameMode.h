#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class TESTSIMU_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void StartGame();

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

private:
	bool bGameStarting = false;
};
