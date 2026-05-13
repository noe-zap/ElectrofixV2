#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class USessionSubsystem;

class TESTSIMU_API SSlateServerBrowserWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSlateServerBrowserWidget) {}
		SLATE_EVENT(FSimpleDelegate, OnBackClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSlateServerBrowserWidget();

private:
	FReply OnRefreshClicked();
	FReply OnBackClicked();
	FReply OnJoinSessionClicked(int32 SessionIndex);

	void HandleSessionsFound(int32 NumSessions);
	void RefreshSessionList();

	FSimpleDelegate BackClickedDelegate;
	TWeakObjectPtr<USessionSubsystem> SessionSubsystem;

	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SVerticalBox> SessionListBox;

	FDelegateHandle SessionsFoundHandle;
};
