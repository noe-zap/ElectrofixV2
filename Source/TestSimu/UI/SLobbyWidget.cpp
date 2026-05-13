#include "UI/SLobbyWidget.h"
#include "Online/SessionSubsystem.h"
#include "Lobby/LobbyGameState.h"
#include "Lobby/LobbyPlayerController.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

static FSlateFontInfo LobbyFont(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle("Regular", Size);
}

void SLobbyWidget::Construct(const FArguments& InArgs)
{
	bIsHost = InArgs._bIsHost;
	OnLeaveClickedDelegate = InArgs._OnLeaveClicked;
	OnStartGameClickedDelegate = InArgs._OnStartGameClicked;

	if (GEngine && GEngine->GameViewport)
	{
		UWorld* World = GEngine->GameViewport->GetWorld();
		if (World)
		{
			UGameInstance* GI = World->GetGameInstance();
			if (GI) SessionSubsystem = GI->GetSubsystem<USessionSubsystem>();
			LobbyGameState = World->GetGameState<ALobbyGameState>();
			LocalPlayerController = Cast<ALobbyPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
		}
	}

	const FSlateFontInfo TitleFont  = LobbyFont(32);
	const FSlateFontInfo LabelFont  = LobbyFont(16);
	const FSlateFontInfo ChatFont   = LobbyFont(14);
	const FSlateFontInfo ButtonFont = LobbyFont(16);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[ SNew(SBorder).BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.95f)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(700.f).HeightOverride(550.f)
			[
				SNew(SBorder).BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.1f, 0.95f)).Padding(20.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 15.f)
					[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Lobby_Title", "Lobby")).Font(TitleFont).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f)) ]

					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.35f).Padding(0.f, 0.f, 10.f, 0.f)
						[
							SNew(SBorder).BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.12f)).Padding(10.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
								[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Lobby_Players", "Players")).Font(LabelFont).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)) ]
								+ SVerticalBox::Slot().FillHeight(1.f)
								[ SNew(SScrollBox) + SScrollBox::Slot() [ SAssignNew(PlayerListBox, SVerticalBox) ] ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
								[
									SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SLobbyWidget::OnInviteFriendsClicked).ContentPadding(FMargin(15.f, 8.f))
									[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Lobby_InviteFriends", "Invite Friends")).Font(ButtonFont).ColorAndOpacity(FLinearColor(0.2f, 0.6f, 1.f)) ]
								]
							]
						]
						+ SHorizontalBox::Slot().FillWidth(0.65f)
						[
							SNew(SBorder).BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.12f)).Padding(10.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
								[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Lobby_Chat", "Chat")).Font(LabelFont).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)) ]
								+ SVerticalBox::Slot().FillHeight(1.f)
								[ SAssignNew(ChatScrollBox, SScrollBox) + SScrollBox::Slot() [ SAssignNew(ChatBox, SVerticalBox) ] ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().FillWidth(1.f)
									[ SAssignNew(ChatInput, SEditableTextBox).HintText(NSLOCTEXT("TestSimu", "Lobby_ChatHint", "Type a message...")).OnTextCommitted(this, &SLobbyWidget::OnChatTextCommitted) ]
									+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f, 0.f, 0.f)
									[
										SNew(SButton).OnClicked(this, &SLobbyWidget::OnSendChatClicked).ContentPadding(FMargin(10.f, 5.f))
										[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Lobby_Send", "Send")).Font(ChatFont) ]
									]
								]
							]
						]
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 15.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Left)
						[
							SNew(SButton).OnClicked(this, &SLobbyWidget::OnLeaveClicked).ContentPadding(FMargin(30.f, 12.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Lobby_Leave", "Leave")).Font(ButtonFont).ColorAndOpacity(FLinearColor(0.8f, 0.3f, 0.3f)) ]
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right)
						[
							SAssignNew(StartGameButton, SButton)
							.Visibility(this, &SLobbyWidget::GetStartButtonVisibility)
							.IsEnabled(this, &SLobbyWidget::IsStartButtonEnabled)
							.OnClicked(this, &SLobbyWidget::OnStartGameClicked)
							.ContentPadding(FMargin(30.f, 12.f))
							[ SNew(STextBlock).Text(this, &SLobbyWidget::GetStartButtonText).Font(ButtonFont).ColorAndOpacity(FLinearColor(0.2f, 1.f, 0.2f)) ]
						]
					]
				]
			]
		]
	];

	RefreshPlayerList();

	if (LobbyGameState.IsValid())
	{
		for (const FChatMessage& Msg : LobbyGameState->ChatMessages)
		{
			AddChatMessage(Msg);
		}
		LastMessageCount = LobbyGameState->ChatMessages.Num();
	}
}

SLobbyWidget::~SLobbyWidget() {}

void SLobbyWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UWorld* World = nullptr;
	if (GEngine && GEngine->GameViewport) World = GEngine->GameViewport->GetWorld();

	if (!LobbyGameState.IsValid() && World) LobbyGameState = World->GetGameState<ALobbyGameState>();
	if (!LocalPlayerController.IsValid() && World) LocalPlayerController = Cast<ALobbyPlayerController>(UGameplayStatics::GetPlayerController(World, 0));

	RefreshTimer += InDeltaTime;
	if (RefreshTimer > 0.5f)
	{
		RefreshTimer = 0.f;
		RefreshPlayerList();
	}

	if (LobbyGameState.IsValid())
	{
		int32 CurrentCount = LobbyGameState->ChatMessages.Num();
		if (CurrentCount > LastMessageCount)
		{
			for (int32 i = LastMessageCount; i < CurrentCount; i++) AddChatMessage(LobbyGameState->ChatMessages[i]);
			LastMessageCount = CurrentCount;
		}
	}
}

FReply SLobbyWidget::OnInviteFriendsClicked()
{
	if (SessionSubsystem.IsValid()) SessionSubsystem->OpenInviteUI();
	return FReply::Handled();
}

FReply SLobbyWidget::OnStartGameClicked()
{
	if (bStartRequested) return FReply::Handled();
	bStartRequested = true;

	if (OnStartGameClickedDelegate.IsBound())
		OnStartGameClickedDelegate.Execute();
	else if (LocalPlayerController.IsValid())
		LocalPlayerController->ServerRequestStartGame();
	return FReply::Handled();
}

FReply SLobbyWidget::OnLeaveClicked()
{
	if (OnLeaveClickedDelegate.IsBound())
	{
		OnLeaveClickedDelegate.Execute();
	}
	else
	{
		if (SessionSubsystem.IsValid()) SessionSubsystem->DestroySession();
		if (GEngine && GEngine->GameViewport)
		{
			UWorld* World = GEngine->GameViewport->GetWorld();
			if (World) UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuMap")));
		}
	}
	return FReply::Handled();
}

FReply SLobbyWidget::OnSendChatClicked()
{
	if (ChatInput.IsValid())
	{
		FString Message = ChatInput->GetText().ToString();
		if (!Message.IsEmpty())
		{
			if (LocalPlayerController.IsValid()) LocalPlayerController->ServerSendChatMessage(Message);
			ChatInput->SetText(FText::GetEmpty());
		}
	}
	return FReply::Handled();
}

void SLobbyWidget::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter) OnSendChatClicked();
}

void SLobbyWidget::RefreshPlayerList()
{
	if (!PlayerListBox.IsValid() || !LobbyGameState.IsValid()) return;

	TArray<FString> Names = LobbyGameState->GetPlayerNames();
	if (Names == CachedPlayerNames) return;
	CachedPlayerNames = Names;

	PlayerListBox->ClearChildren();

	const FSlateFontInfo ChatFont = LobbyFont(14);
	for (int32 i = 0; i < Names.Num(); i++)
	{
		FText DisplayText = (i == 0)
			? FText::FromString(FString::Printf(TEXT("%s (Host)"), *Names[i]))
			: FText::FromString(Names[i]);

		PlayerListBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(STextBlock).Text(DisplayText).Font(ChatFont)
			.ColorAndOpacity(i == 0 ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor::White)
		];
	}
}

void SLobbyWidget::AddChatMessage(const FChatMessage& Message)
{
	if (!ChatBox.IsValid()) return;

	FLinearColor NameColor = FLinearColor(0.4f, 0.8f, 1.f);
	if (Message.SenderName == TEXT("System")) NameColor = FLinearColor(0.7f, 0.7f, 0.7f);

	const FSlateFontInfo ChatFont = LobbyFont(14);
	ChatBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[ SNew(STextBlock).Text(FText::FromString(Message.SenderName + TEXT(": "))).Font(ChatFont).ColorAndOpacity(NameColor) ]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[ SNew(STextBlock).Text(FText::FromString(Message.Message)).Font(ChatFont).ColorAndOpacity(FLinearColor::White).AutoWrapText(true) ]
	];

	if (ChatScrollBox.IsValid()) ChatScrollBox->ScrollToEnd();
}

EVisibility SLobbyWidget::GetStartButtonVisibility() const { return bIsHost ? EVisibility::Visible : EVisibility::Collapsed; }
bool SLobbyWidget::IsStartButtonEnabled() const { return bIsHost && !bStartRequested; }
FText SLobbyWidget::GetStartButtonText() const { return bStartRequested ? NSLOCTEXT("TestSimu", "Lobby_Loading", "Loading...") : NSLOCTEXT("TestSimu", "Lobby_StartGame", "Start Game"); }

void SLobbyWidget::SetCanStartGame(bool bCanStart) { bIsHost = bCanStart; }
void SLobbyWidget::RefreshLobbyInfo() { RefreshPlayerList(); }
