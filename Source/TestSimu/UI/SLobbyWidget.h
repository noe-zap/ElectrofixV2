#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class USessionSubsystem;
class ALobbyGameState;
class ALobbyPlayerController;
struct FChatMessage;

class TESTSIMU_API SLobbyWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLobbyWidget)
		: _bIsHost(false)
	{}
		SLATE_ARGUMENT(bool, bIsHost)
		SLATE_EVENT(FSimpleDelegate, OnLeaveClicked)
		SLATE_EVENT(FSimpleDelegate, OnStartGameClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SLobbyWidget();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void SetCanStartGame(bool bCanStart);
	void RefreshLobbyInfo();

private:
	FSimpleDelegate OnLeaveClickedDelegate;
	FSimpleDelegate OnStartGameClickedDelegate;

	FReply OnInviteFriendsClicked();
	FReply OnStartGameClicked();
	FReply OnLeaveClicked();
	FReply OnSendChatClicked();

	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	void RefreshPlayerList();
	void AddChatMessage(const FChatMessage& Message);

	EVisibility GetStartButtonVisibility() const;
	bool IsStartButtonEnabled() const;
	FText GetStartButtonText() const;

	TWeakObjectPtr<USessionSubsystem> SessionSubsystem;
	TWeakObjectPtr<ALobbyGameState> LobbyGameState;
	TWeakObjectPtr<ALobbyPlayerController> LocalPlayerController;

	bool bIsHost = false;
	bool bStartRequested = false;
	int32 LastMessageCount = 0;
	float RefreshTimer = 0.f;
	TArray<FString> CachedPlayerNames;

	TSharedPtr<SVerticalBox> PlayerListBox;
	TSharedPtr<SVerticalBox> ChatBox;
	TSharedPtr<SScrollBox> ChatScrollBox;
	TSharedPtr<SEditableTextBox> ChatInput;
	TSharedPtr<SButton> StartGameButton;
};
