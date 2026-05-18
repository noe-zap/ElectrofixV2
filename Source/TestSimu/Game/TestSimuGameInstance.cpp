#include "Game/TestSimuGameInstance.h"

#include "MoviePlayer.h"
#include "UI/SLoadingScreenWidget.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/ConfigCacheIni.h"

// Must stay in sync with LanguageCultureCodes in SSettingsWidget.cpp
static const TArray<FString> TestSimuSupportedCultures = {
	TEXT("en"), TEXT("fr"), TEXT("it"), TEXT("de"), TEXT("es"),
	TEXT("ja"), TEXT("ko"), TEXT("pl"), TEXT("pt-BR"), TEXT("pt"),
	TEXT("ru"), TEXT("zh-Hans-CN"), TEXT("es-419"), TEXT("tr")
};

static FString FindMatchingSupportedCulture(const FString& InCulture)
{
	for (const FString& Culture : TestSimuSupportedCultures)
	{
		if (InCulture.Equals(Culture, ESearchCase::IgnoreCase))
		{
			return Culture;
		}
	}

	FString BaseLang;
	if (InCulture.Split(TEXT("-"), &BaseLang, nullptr))
	{
		for (const FString& Culture : TestSimuSupportedCultures)
		{
			if (Culture.Equals(BaseLang, ESearchCase::IgnoreCase))
			{
				return Culture;
			}
		}
	}

	return FString();
}

void UTestSimuGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UTestSimuGameInstance::HandlePreLoadMap);

	if (GIsEditor)
	{
		EditorCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	}

	FString TargetCulture;
	FString SavedCulture;
	if (GConfig->GetString(TEXT("TestSimu.Language"), TEXT("Culture"), SavedCulture, GGameUserSettingsIni) && !SavedCulture.IsEmpty())
	{
		TargetCulture = SavedCulture;
	}
	else
	{
		const FString OSCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
		TargetCulture = FindMatchingSupportedCulture(OSCulture);
		if (TargetCulture.IsEmpty())
		{
			TargetCulture = TEXT("en");
		}

		GConfig->SetString(TEXT("TestSimu.Language"), TEXT("Culture"), *TargetCulture, GGameUserSettingsIni);
	}

	GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *TargetCulture, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);

	FInternationalization::Get().SetCurrentLanguageAndLocale(TargetCulture);
}

void UTestSimuGameInstance::Shutdown()
{
	if (GIsEditor && !EditorCulture.IsEmpty())
	{
		FInternationalization::Get().SetCurrentLanguageAndLocale(EditorCulture);
	}

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
