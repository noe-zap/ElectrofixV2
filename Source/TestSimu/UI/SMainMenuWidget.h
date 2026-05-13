#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Online/SessionSubsystem.h"

class USessionSubsystem;
class SSlateServerBrowserWidget;
class SLobbyWidget;
class SSettingsWidget;
class SWidgetSwitcher;
class STextBlock;

class TESTSIMU_API SMainMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainMenuWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FReply OnPlayClicked();
	FReply OnHostClicked();
	FReply OnBrowseClicked();
	FReply OnQuitClicked();

	FReply OnCreateSessionClicked();
	FReply OnHostBackClicked();
	void OnVisibilitySelected(ESessionVisibility NewVisibility);

	void OnSessionCreated(bool bWasSuccessful);
	void OnSessionJoined(bool bWasSuccessful);

	void ShowMainMenu();
	void ShowHostSettings();
	void ShowServerBrowser();
	void ShowLobby(bool bAsHost);
	void ShowSettings();
	void HideSettings();

private:
	TWeakObjectPtr<USessionSubsystem> SessionSubsystem;

	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SWidgetSwitcher> ContentSwitcher;
	TSharedPtr<SSlateServerBrowserWidget> ServerBrowserWidget;
	TSharedPtr<SLobbyWidget> LobbyWidget;
	TSharedPtr<SSettingsWidget> SettingsWidget;
	TSharedPtr<SWidget> SettingsViewportHost;

	ESessionVisibility SelectedVisibility = ESessionVisibility::Public;
};
