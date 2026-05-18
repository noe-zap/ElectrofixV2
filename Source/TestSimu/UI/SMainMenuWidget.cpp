#include "UI/SMainMenuWidget.h"
#include "UI/SSlateServerBrowserWidget.h"
#include "UI/SLobbyWidget.h"
#include "UI/SSettingsWidget.h"
#include "Online/SessionSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWeakWidget.h"
#include "Styling/CoreStyle.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

static FSlateFontInfo MenuFont(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle("Regular", Size);
}

void SMainMenuWidget::Construct(const FArguments& InArgs)
{
	if (GEngine && GEngine->GameViewport)
	{
		UWorld* World = GEngine->GameViewport->GetWorld();
		if (World)
		{
			UGameInstance* GI = World->GetGameInstance();
			if (GI) SessionSubsystem = GI->GetSubsystem<USessionSubsystem>();
		}
	}

	const FSlateFontInfo TitleFont  = MenuFont(36);
	const FSlateFontInfo ButtonFont = MenuFont(18);
	const FSlateFontInfo StatusFont = MenuFont(14);
	const FSlateColor    ButtonTextColor(FLinearColor::White);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.0f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(500.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f))
				.Padding(40.f)
				[
					SAssignNew(ContentSwitcher, SWidgetSwitcher)

					// Slot 0: Main Menu
					+ SWidgetSwitcher::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 40.f)
						[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Title", "TestSimu")).Font(TitleFont).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f)) ]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SMainMenuWidget::OnPlayClicked).ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Play", "Play")).Font(ButtonFont).ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.f, 0.2f))) ]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SMainMenuWidget::OnHostClicked).ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_HostGame", "Host Game")).Font(ButtonFont).ColorAndOpacity(ButtonTextColor) ]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SMainMenuWidget::OnBrowseClicked).ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_BrowseGames", "Browse Games")).Font(ButtonFont).ColorAndOpacity(ButtonTextColor) ]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center)
							.OnClicked_Lambda([this]() { ShowSettings(); return FReply::Handled(); })
							.ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Settings", "Settings")).Font(ButtonFont).ColorAndOpacity(ButtonTextColor) ]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 30.f, 0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SMainMenuWidget::OnQuitClicked).ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Quit", "Quit")).Font(ButtonFont).ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.3f, 0.3f))) ]
						]

						+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Bottom).HAlign(HAlign_Center)
						[
							SAssignNew(StatusText, STextBlock)
							.Text(SessionSubsystem.IsValid() && SessionSubsystem->IsSteamAvailable() ? NSLOCTEXT("TestSimu", "MainMenu_SteamConnected", "Steam Connected") : NSLOCTEXT("TestSimu", "MainMenu_SteamOffline", "Steam Offline - LAN Only"))
							.Font(StatusFont).ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
						]
					]

					// Slot 1: Server Browser
					+ SWidgetSwitcher::Slot()
					[ SAssignNew(ServerBrowserWidget, SSlateServerBrowserWidget).OnBackClicked(FSimpleDelegate::CreateLambda([this]() { ShowMainMenu(); })) ]

					// Slot 2: Host Settings
					+ SWidgetSwitcher::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 30.f)
						[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_HostSettings", "Host Settings")).Font(TitleFont).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f)) ]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f).HAlign(HAlign_Center)
						[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_SessionVisibility", "Session Visibility")).Font(ButtonFont).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)) ]

						+ SVerticalBox::Slot().AutoHeight().Padding(20.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center)
							.OnClicked_Lambda([this]() { OnVisibilitySelected(ESessionVisibility::Public); return FReply::Handled(); })
							.ContentPadding(FMargin(30.f, 10.f))
							[
								SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Public", "Public")).Font(StatusFont)
								.ColorAndOpacity_Lambda([this]() { return SelectedVisibility == ESessionVisibility::Public ? FSlateColor(FLinearColor(0.2f, 1.f, 0.2f)) : FSlateColor(FLinearColor::White); })
							]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(20.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center)
							.OnClicked_Lambda([this]() { OnVisibilitySelected(ESessionVisibility::FriendsOnly); return FReply::Handled(); })
							.ContentPadding(FMargin(30.f, 10.f))
							[
								SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_FriendsOnly", "Friends Only")).Font(StatusFont)
								.ColorAndOpacity_Lambda([this]() { return SelectedVisibility == ESessionVisibility::FriendsOnly ? FSlateColor(FLinearColor(0.2f, 0.6f, 1.f)) : FSlateColor(FLinearColor::White); })
							]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(20.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center)
							.OnClicked_Lambda([this]() { OnVisibilitySelected(ESessionVisibility::Private); return FReply::Handled(); })
							.ContentPadding(FMargin(30.f, 10.f))
							[
								SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Private", "Private")).Font(StatusFont)
								.ColorAndOpacity_Lambda([this]() { return SelectedVisibility == ESessionVisibility::Private ? FSlateColor(FLinearColor(1.f, 0.6f, 0.2f)) : FSlateColor(FLinearColor::White); })
							]
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f) [ SNew(SSpacer) ]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SMainMenuWidget::OnCreateSessionClicked).ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_CreateLobby", "Create Lobby")).Font(ButtonFont).ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.f, 0.2f))) ]
						]

						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f)
						[
							SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SMainMenuWidget::OnHostBackClicked).ContentPadding(FMargin(40.f, 15.f))
							[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "MainMenu_Back", "Back")).Font(ButtonFont).ColorAndOpacity(ButtonTextColor) ]
						]
					]

					// Slot 3: Lobby (shown in-place if the host stays on this map; usually the world ServerTravels away)
					+ SWidgetSwitcher::Slot()
					[ SAssignNew(LobbyWidget, SLobbyWidget) ]
				]
			]
		]
	];
}

FReply SMainMenuWidget::OnPlayClicked()
{
	// "Play" defaults to Host flow as a shortcut.
	return OnHostClicked();
}

FReply SMainMenuWidget::OnHostClicked()
{
	ShowHostSettings();
	return FReply::Handled();
}

void SMainMenuWidget::OnSessionCreated(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// SessionSubsystem already triggers ServerTravel to the configured LobbyMap.
		if (StatusText.IsValid()) StatusText->SetText(NSLOCTEXT("TestSimu", "MainMenu_SessionCreated", "Session created"));
	}
	else
	{
		if (StatusText.IsValid()) StatusText->SetText(NSLOCTEXT("TestSimu", "MainMenu_FailedCreateSession", "Failed to create session"));
		ShowMainMenu();
	}
}

void SMainMenuWidget::OnSessionJoined(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		if (StatusText.IsValid()) StatusText->SetText(NSLOCTEXT("TestSimu", "MainMenu_FailedJoinSession", "Failed to join session"));
		ShowMainMenu();
	}
}

FReply SMainMenuWidget::OnBrowseClicked()
{
	ShowServerBrowser();
	return FReply::Handled();
}

FReply SMainMenuWidget::OnQuitClicked()
{
	if (GEngine && GEngine->GameViewport)
	{
		UWorld* World = GEngine->GameViewport->GetWorld();
		if (World) UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, false);
	}
	return FReply::Handled();
}

FReply SMainMenuWidget::OnCreateSessionClicked()
{
	if (StatusText.IsValid()) StatusText->SetText(NSLOCTEXT("TestSimu", "MainMenu_CreatingLobby", "Creating lobby..."));
	if (SessionSubsystem.IsValid()) SessionSubsystem->CreateSession(4, SelectedVisibility);
	return FReply::Handled();
}

FReply SMainMenuWidget::OnHostBackClicked()
{
	ShowMainMenu();
	return FReply::Handled();
}

void SMainMenuWidget::OnVisibilitySelected(ESessionVisibility NewVisibility)
{
	SelectedVisibility = NewVisibility;
}

void SMainMenuWidget::ShowMainMenu() { if (ContentSwitcher.IsValid()) ContentSwitcher->SetActiveWidgetIndex(0); }
void SMainMenuWidget::ShowServerBrowser() { if (ContentSwitcher.IsValid()) ContentSwitcher->SetActiveWidgetIndex(1); }
void SMainMenuWidget::ShowHostSettings() { if (ContentSwitcher.IsValid()) ContentSwitcher->SetActiveWidgetIndex(2); }

void SMainMenuWidget::ShowLobby(bool bAsHost)
{
	if (LobbyWidget.IsValid()) { LobbyWidget->SetCanStartGame(bAsHost); LobbyWidget->RefreshLobbyInfo(); }
	if (ContentSwitcher.IsValid()) ContentSwitcher->SetActiveWidgetIndex(3);
}

void SMainMenuWidget::ShowSettings()
{
	if (!GEngine || !GEngine->GameViewport) return;

	SettingsWidget = SNew(SSettingsWidget)
		.ShowLanguageTab(true)
		.OnBack(FSimpleDelegate::CreateLambda([this]() { HideSettings(); }));

	SettingsViewportHost =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.0f)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(900.f).HeightOverride(700.f)
			[
				SNew(SBorder).BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.1f, 1.0f)).Padding(40.f)
				[ SettingsWidget.ToSharedRef() ]
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(SettingsViewportHost.ToSharedRef(), 100);
}

void SMainMenuWidget::HideSettings()
{
	if (GEngine && GEngine->GameViewport && SettingsViewportHost.IsValid())
		GEngine->GameViewport->RemoveViewportWidgetContent(SettingsViewportHost.ToSharedRef());
	SettingsViewportHost.Reset();
	SettingsWidget.Reset();
}
