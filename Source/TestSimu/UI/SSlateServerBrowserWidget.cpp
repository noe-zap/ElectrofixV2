#include "UI/SSlateServerBrowserWidget.h"
#include "Online/SessionSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

static FSlateFontInfo BrowserFont(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle("Regular", Size);
}

void SSlateServerBrowserWidget::Construct(const FArguments& InArgs)
{
	BackClickedDelegate = InArgs._OnBackClicked;

	if (GEngine && GEngine->GameViewport)
	{
		UWorld* World = GEngine->GameViewport->GetWorld();
		if (World)
		{
			UGameInstance* GI = World->GetGameInstance();
			if (GI)
			{
				SessionSubsystem = GI->GetSubsystem<USessionSubsystem>();
			}
		}
	}

	if (SessionSubsystem.IsValid())
	{
		SessionsFoundHandle = SessionSubsystem->OnSessionsFoundNative.AddSP(this, &SSlateServerBrowserWidget::HandleSessionsFound);
	}

	FSlateFontInfo TitleFont = BrowserFont(24);
	FSlateFontInfo ButtonFont = BrowserFont(16);
	FSlateFontInfo StatusFont = BrowserFont(14);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 20.f)
		[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "ServerBrowser_Title", "Server Browser")).Font(TitleFont).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f)) ]

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 10.f)
		[ SAssignNew(StatusText, STextBlock).Text(NSLOCTEXT("TestSimu", "ServerBrowser_ClickRefresh", "Click Refresh to search")).Font(StatusFont).ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f)) ]

		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 10.f)
		[
			SNew(SScrollBox) + SScrollBox::Slot() [ SAssignNew(SessionListBox, SVerticalBox) ]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
			[
				SNew(SButton).OnClicked(this, &SSlateServerBrowserWidget::OnRefreshClicked).ContentPadding(FMargin(30.f, 10.f))
				[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "ServerBrowser_Refresh", "Refresh")).Font(ButtonFont) ]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
			[
				SNew(SButton).OnClicked(this, &SSlateServerBrowserWidget::OnBackClicked).ContentPadding(FMargin(30.f, 10.f))
				[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "ServerBrowser_Back", "Back")).Font(ButtonFont) ]
			]
		]
	];
}

SSlateServerBrowserWidget::~SSlateServerBrowserWidget()
{
	if (SessionSubsystem.IsValid() && SessionsFoundHandle.IsValid())
	{
		SessionSubsystem->OnSessionsFoundNative.Remove(SessionsFoundHandle);
	}
}

FReply SSlateServerBrowserWidget::OnRefreshClicked()
{
	if (StatusText.IsValid()) StatusText->SetText(NSLOCTEXT("TestSimu", "ServerBrowser_Searching", "Searching..."));
	if (SessionListBox.IsValid()) SessionListBox->ClearChildren();
	if (SessionSubsystem.IsValid()) SessionSubsystem->FindSessions();
	return FReply::Handled();
}

void SSlateServerBrowserWidget::HandleSessionsFound(int32 NumSessions)
{
	if (StatusText.IsValid())
	{
		if (NumSessions > 0)
			StatusText->SetText(FText::Format(NSLOCTEXT("TestSimu", "ServerBrowser_FoundSessions", "Found {0} session(s)"), FText::AsNumber(NumSessions)));
		else
			StatusText->SetText(NSLOCTEXT("TestSimu", "ServerBrowser_NoSessions", "No sessions found"));
	}
	RefreshSessionList();
}

FReply SSlateServerBrowserWidget::OnBackClicked()
{
	BackClickedDelegate.ExecuteIfBound();
	return FReply::Handled();
}

FReply SSlateServerBrowserWidget::OnJoinSessionClicked(int32 SessionIndex)
{
	if (StatusText.IsValid()) StatusText->SetText(NSLOCTEXT("TestSimu", "ServerBrowser_Joining", "Joining..."));
	if (SessionSubsystem.IsValid()) SessionSubsystem->JoinSession(SessionIndex);
	return FReply::Handled();
}

void SSlateServerBrowserWidget::RefreshSessionList()
{
	if (!SessionListBox.IsValid() || !SessionSubsystem.IsValid()) return;

	SessionListBox->ClearChildren();

	TArray<FFoundSessionInfo> Sessions = SessionSubsystem->GetFoundSessions();
	FSlateFontInfo SessionFont = BrowserFont(16);
	FSlateFontInfo JoinFont = BrowserFont(14);

	for (int32 i = 0; i < Sessions.Num(); i++)
	{
		const int32 SessionIndex = Sessions[i].Index;
		const FString Label = FString::Printf(TEXT("[%d] %s (%d/%d) - %dms"),
			SessionIndex, *Sessions[i].OwnerName, Sessions[i].CurrentPlayers, Sessions[i].MaxPlayers, Sessions[i].PingMs);

		SessionListBox->AddSlot().AutoHeight().Padding(0.f, 5.f)
		[
			SNew(SBorder).BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.15f, 1.f)).Padding(10.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[ SNew(STextBlock).Text(FText::FromString(Label)).Font(SessionFont).ColorAndOpacity(FLinearColor::White) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.OnClicked_Lambda([this, SessionIndex]() { return OnJoinSessionClicked(SessionIndex); })
					.ContentPadding(FMargin(15.f, 5.f))
					[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "ServerBrowser_Join", "Join")).Font(JoinFont) ]
				]
			]
		];
	}
}
