#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

UCLASS()
class TESTSIMU_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ALobbyPlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_bIsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

	UFUNCTION()
	void OnRep_bIsReady();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetReady(bool bReady);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
