#include "Online/SessionSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

static const FName GameIdentifierKey(TEXT("GAMEIDENTIFIER"));
static const FString GameIdentifierValue(TEXT("TestSimu"));

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (LobbyMap.IsNull())
	{
		LobbyMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ShopSim/MainMenu/LobbyMap.LobbyMap")));
	}
	if (GameMap.IsNull())
	{
		GameMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/LV_MallOpti.LV_MallOpti")));
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		InviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &USessionSubsystem::OnSessionUserInviteAccepted));
	}

	UE_LOG(LogTemp, Log, TEXT("SessionSubsystem: Steam %s"),
		IsSteamAvailable() ? TEXT("available") : TEXT("unavailable"));
}

void USessionSubsystem::Deinitialize()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteAcceptedDelegateHandle);
	}

	if (IsInSession())
	{
		DestroySession();
	}

	Super::Deinitialize();
}

IOnlineSessionPtr USessionSubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	return OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
}

bool USessionSubsystem::IsSteamAvailable() const
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	return OnlineSub && OnlineSub->GetSubsystemName() == STEAM_SUBSYSTEM;
}

bool USessionSubsystem::IsInSession() const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		return SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
	}
	return false;
}

bool USessionSubsystem::IsHosting() const
{
	return bIsHosting && IsInSession();
}

FString USessionSubsystem::GetSessionStatus() const
{
	FString Status;

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	Status += FString::Printf(TEXT("Online Subsystem: %s\n"),
		OnlineSub ? *OnlineSub->GetSubsystemName().ToString() : TEXT("None"));
	Status += FString::Printf(TEXT("Steam Available: %s\n"), IsSteamAvailable() ? TEXT("Yes") : TEXT("No"));
	Status += FString::Printf(TEXT("In Session: %s\n"), IsInSession() ? TEXT("Yes") : TEXT("No"));
	Status += FString::Printf(TEXT("Is Hosting: %s\n"), IsHosting() ? TEXT("Yes") : TEXT("No"));

	if (IsInSession())
	{
		IOnlineSessionPtr SessionInterface = GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
			if (Session)
			{
				Status += FString::Printf(TEXT("Session State: %s\n"),
					EOnlineSessionState::ToString(Session->SessionState));
				Status += FString::Printf(TEXT("Players: %d/%d\n"),
					Session->RegisteredPlayers.Num(),
					Session->SessionSettings.NumPublicConnections);
			}
		}
	}

	return Status;
}

FString USessionSubsystem::GetMapTravelURL(const TSoftObjectPtr<UWorld>& Map, bool bAsListenServer) const
{
	const FString PathName = Map.ToSoftObjectPath().GetLongPackageName();
	return bAsListenServer ? PathName + TEXT("?listen") : PathName;
}

void USessionSubsystem::CreateSession(int32 MaxPlayers, ESessionVisibility Visibility)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession: No session interface available"));
		OnSessionCreated.Broadcast(false);
		return;
	}

	if (IsInSession())
	{
		bPendingSessionCreate = true;
		PendingMaxPlayers = MaxPlayers;
		PendingVisibility = Visibility;
		DestroySession();
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowInvites = true;

	switch (Visibility)
	{
	case ESessionVisibility::Public:
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bAllowJoinViaPresence = true;
		break;
	case ESessionVisibility::FriendsOnly:
		SessionSettings.bShouldAdvertise = false;
		SessionSettings.bAllowJoinViaPresence = true;
		break;
	case ESessionVisibility::Private:
		SessionSettings.bShouldAdvertise = false;
		SessionSettings.bAllowJoinViaPresence = false;
		break;
	}

	SessionSettings.Set(SETTING_MAPNAME, FString(TEXT("Lobby")), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(GameIdentifierKey, GameIdentifierValue, EOnlineDataAdvertisementType::ViaOnlineService);

	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnCreateSessionComplete));

	bIsHosting = true;

	if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		bIsHosting = false;
		OnSessionCreated.Broadcast(false);
	}
}

void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
	}

	if (!bWasSuccessful)
	{
		bIsHosting = false;
	}

	OnSessionCreated.Broadcast(bWasSuccessful);

	if (bWasSuccessful)
	{
		TravelToLobby();
	}
}

void USessionSubsystem::TravelToLobby()
{
	if (LobbyMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("SessionSubsystem::TravelToLobby: LobbyMap is not set"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FString URL = GetMapTravelURL(LobbyMap, true);
	World->ServerTravel(URL);
}

void USessionSubsystem::StartGame()
{
	if (!IsHosting())
	{
		UE_LOG(LogTemp, Warning, TEXT("SessionSubsystem::StartGame called by non-host"));
		return;
	}

	if (GameMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("SessionSubsystem::StartGame: GameMap is not set"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FString URL = GetMapTravelURL(GameMap, true);
	World->ServerTravel(URL);
}

void USessionSubsystem::FindSessions()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionsFound.Broadcast(0);
		return;
	}

	bIsSearching = true;

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(GameIdentifierKey, GameIdentifierValue, EOnlineComparisonOp::Equals);

	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnFindSessionsComplete));

	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		bIsSearching = false;
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		OnSessionsFound.Broadcast(0);
		OnSessionsFoundNative.Broadcast(0);
	}
}

void USessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	bIsSearching = false;

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
	}

	int32 NumSessions = 0;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		NumSessions = SessionSearch->SearchResults.Num();
	}

	OnSessionsFound.Broadcast(NumSessions);
	OnSessionsFoundNative.Broadcast(NumSessions);
}

TArray<FFoundSessionInfo> USessionSubsystem::GetFoundSessions() const
{
	TArray<FFoundSessionInfo> Out;
	if (!SessionSearch.IsValid())
	{
		return Out;
	}

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];
		FFoundSessionInfo Info;
		Info.Index = i;
		Info.OwnerName = Result.Session.OwningUserName;
		Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
		Info.PingMs = Result.PingInMs;
		Out.Add(Info);
	}
	return Out;
}

void USessionSubsystem::OpenInviteUI()
{
	if (!IsInSession())
	{
		return;
	}

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (!OnlineSub)
	{
		return;
	}

	IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
	if (!ExternalUI.IsValid())
	{
		return;
	}

	ExternalUI->ShowInviteUI(0, NAME_GameSession);
}

void USessionSubsystem::OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		OnSessionJoined.Broadcast(false, TEXT("Invalid invite"));
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionJoined.Broadcast(false, TEXT("No session interface"));
		return;
	}

	if (IsInSession())
	{
		bPendingInviteJoin = true;
		PendingInviteControllerId = ControllerId;
		PendingInviteResult = MakeShared<FOnlineSessionSearchResult>(InviteResult);
		DestroySession();
		return;
	}

	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete));

	bIsHosting = false;

	if (!SessionInterface->JoinSession(ControllerId, NAME_GameSession, InviteResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		OnSessionJoined.Broadcast(false, TEXT("Failed to join from invite"));
	}
}

void USessionSubsystem::JoinSession(int32 SearchResultIndex)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionJoined.Broadcast(false, TEXT("No session interface"));
		return;
	}

	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		OnSessionJoined.Broadcast(false, TEXT("Invalid session index"));
		return;
	}

	if (IsInSession())
	{
		bPendingBrowseJoin = true;
		PendingBrowseJoinIndex = SearchResultIndex;
		DestroySession();
		return;
	}

	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete));

	bIsHosting = false;

	if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SearchResultIndex]))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		OnSessionJoined.Broadcast(false, TEXT("Failed to initiate join"));
	}
}

void USessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
	}

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectString;
		if (SessionInterface.IsValid() && SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			if (ConnectString.IsEmpty())
			{
				OnSessionJoined.Broadcast(false, TEXT("Empty connect string"));
				return;
			}

			APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
			if (PC)
			{
				PC->ClientTravel(ConnectString, TRAVEL_Absolute);
				OnSessionJoined.Broadcast(true, TEXT(""));
			}
			else
			{
				OnSessionJoined.Broadcast(false, TEXT("No local player controller"));
			}
		}
		else
		{
			OnSessionJoined.Broadcast(false, TEXT("Failed to get connect string"));
		}
	}
	else
	{
		FString ErrorMsg;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:           ErrorMsg = TEXT("Session is full"); break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:     ErrorMsg = TEXT("Session no longer exists"); break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: ErrorMsg = TEXT("Could not retrieve address"); break;
		case EOnJoinSessionCompleteResult::AlreadyInSession:        ErrorMsg = TEXT("Already in a session"); break;
		default:                                                    ErrorMsg = TEXT("Unknown error"); break;
		}
		OnSessionJoined.Broadcast(false, ErrorMsg);
	}
}

void USessionSubsystem::DestroySession()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !IsInSession())
	{
		return;
	}

	DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnDestroySessionComplete));

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	}
}

void USessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	}

	if (bWasSuccessful)
	{
		bIsHosting = false;
	}

	OnSessionDestroyed.Broadcast();

	if (bPendingBrowseJoin)
	{
		bPendingBrowseJoin = false;
		int32 IndexToJoin = PendingBrowseJoinIndex;
		PendingBrowseJoinIndex = -1;

		if (SessionSearch.IsValid() && SessionSearch->SearchResults.IsValidIndex(IndexToJoin))
		{
			IOnlineSessionPtr Iface = GetSessionInterface();
			if (Iface.IsValid())
			{
				JoinSessionDelegateHandle = Iface->AddOnJoinSessionCompleteDelegate_Handle(
					FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete));

				bIsHosting = false;
				if (!Iface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[IndexToJoin]))
				{
					Iface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
					OnSessionJoined.Broadcast(false, TEXT("Failed to join after destroy"));
				}
			}
		}
		else
		{
			OnSessionJoined.Broadcast(false, TEXT("Search results expired"));
		}
		return;
	}

	if (bPendingInviteJoin)
	{
		bPendingInviteJoin = false;

		IOnlineSessionPtr Iface = GetSessionInterface();
		if (Iface.IsValid() && PendingInviteResult.IsValid() && PendingInviteResult->IsValid())
		{
			JoinSessionDelegateHandle = Iface->AddOnJoinSessionCompleteDelegate_Handle(
				FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete));

			bIsHosting = false;
			if (!Iface->JoinSession(PendingInviteControllerId, NAME_GameSession, *PendingInviteResult))
			{
				Iface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
				OnSessionJoined.Broadcast(false, TEXT("Failed to join from invite after destroy"));
			}
		}
		PendingInviteResult.Reset();
		return;
	}

	if (bPendingSessionCreate)
	{
		bPendingSessionCreate = false;
		CreateSession(PendingMaxPlayers, PendingVisibility);
	}
}
