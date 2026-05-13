#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESessionVisibility : uint8
{
	Public       UMETA(DisplayName = "Public"),
	FriendsOnly  UMETA(DisplayName = "Friends Only"),
	Private      UMETA(DisplayName = "Private")
};

USTRUCT(BlueprintType)
struct FFoundSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Index = 0;
	UPROPERTY(BlueprintReadOnly) FString OwnerName;
	UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 PingMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreated, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsFound, int32, NumSessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionJoined, bool, bWasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionDestroyed);

UCLASS(Config=Game)
class TESTSIMU_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(int32 MaxPlayers = 4, ESessionVisibility Visibility = ESessionVisibility::Public);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSession(int32 SearchResultIndex);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool IsSteamAvailable() const;

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool IsInSession() const;

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool IsHosting() const;

	UFUNCTION(BlueprintCallable, Category = "Session")
	FString GetSessionStatus() const;

	UFUNCTION(BlueprintCallable, Category = "Session")
	TArray<FFoundSessionInfo> GetFoundSessions() const;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void OpenInviteUI();

	/** Host travels here after CreateSession completes. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void TravelToLobby();

	/** Host calls this from the Lobby to start the game and bring everyone along. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void StartGame();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnSessionJoined OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnSessionDestroyed OnSessionDestroyed;

	/** Non-dynamic multicast (for Slate widgets that bind via AddSP). */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionsFoundNative, int32);
	FOnSessionsFoundNative OnSessionsFoundNative;

	/** Map ServerTravel'd to right after CreateSession. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Session")
	TSoftObjectPtr<UWorld> LobbyMap;

	/** Gameplay map ServerTravel'd to when the host presses Start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Session")
	TSoftObjectPtr<UWorld> GameMap;

private:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);

	IOnlineSessionPtr GetSessionInterface() const;
	FString GetMapTravelURL(const TSoftObjectPtr<UWorld>& Map, bool bAsListenServer) const;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	bool bIsSearching = false;

	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;
	FDelegateHandle InviteAcceptedDelegateHandle;

	bool bIsHosting = false;

	bool bPendingSessionCreate = false;
	int32 PendingMaxPlayers = 4;
	ESessionVisibility PendingVisibility = ESessionVisibility::Public;

	bool bPendingInviteJoin = false;
	int32 PendingInviteControllerId = 0;
	TSharedPtr<FOnlineSessionSearchResult> PendingInviteResult;

	bool bPendingBrowseJoin = false;
	int32 PendingBrowseJoinIndex = -1;
};
