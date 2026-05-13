#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SenderName;

	UPROPERTY(BlueprintReadOnly)
	FString Message;

	UPROPERTY(BlueprintReadOnly)
	float Timestamp = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&, NewMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerListChanged);

UCLASS()
class TESTSIMU_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALobbyGameState();

	UPROPERTY(ReplicatedUsing = OnRep_ChatMessages, BlueprintReadOnly, Category = "Lobby")
	TArray<FChatMessage> ChatMessages;

	UFUNCTION()
	void OnRep_ChatMessages();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void AddChatMessage(const FString& SenderName, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	TArray<FString> GetPlayerNames() const;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnChatMessageReceived OnChatMessageReceived;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnPlayerListChanged OnPlayerListChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

private:
	int32 LastChatCount = 0;
};
