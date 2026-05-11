#include "Game/TestSimuGameInstance.h"

#include "MoviePlayer.h"
#include "UI/SLoadingScreenWidget.h"

void UTestSimuGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UTestSimuGameInstance::HandlePreLoadMap);
}

void UTestSimuGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);

	Super::Shutdown();
}

void UTestSimuGameInstance::HandlePreLoadMap(const FString& MapName)
{
	if (!IsMoviePlayerEnabled())
	{
		return;
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.bMoviesAreSkippable = false;
	LoadingScreen.bWaitForManualStop = false;
	LoadingScreen.bAllowEngineTick = bAllowEngineTickDuringLoad;
	LoadingScreen.MinimumLoadingScreenDisplayTime = MinimumLoadingScreenDisplayTime;
	LoadingScreen.WidgetLoadingScreen = SNew(SLoadingScreenWidget)
		.BackgroundTexture(LoadingBackground)
		.TipText(LoadingTipText)
		.TipFontSize(LoadingTipFontSize);

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}
