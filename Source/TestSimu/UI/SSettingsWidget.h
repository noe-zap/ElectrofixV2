#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "InputCoreTypes.h"

class UEnhancedInputUserSettings;
class ULocalPlayer;
class SWidgetSwitcher;
class STextBlock;
class SSlider;

class TESTSIMU_API SSettingsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsWidget)
		: _ShowLanguageTab(false)
	{}
		SLATE_EVENT(FSimpleDelegate, OnBack)
		SLATE_ARGUMENT(bool, ShowLanguageTab)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	static void ApplySavedMouseSettings(APlayerController* PC);

private:
	TSharedPtr<SWidgetSwitcher> TabSwitcher;
	int32 ActiveTabIndex = 0;
	TArray<TSharedPtr<STextBlock>> TabButtonTexts;
	void SwitchTab(int32 Index);

	bool bInvertXAxis = false;
	bool bInvertYAxis = false;
	float MouseSensitivity = 0.3f;
	TSharedPtr<SSlider> SensitivitySlider;
	TSharedPtr<STextBlock> SensitivityValueText;

	void LoadMouseSettings();
	void SaveMouseSettings();
	void ApplyMouseSettings();

	struct FRebindableAction
	{
		FName MappingName;
		FString DisplayName;
		FKey DefaultKey;
		FKey CurrentKey;
	};

	TArray<FRebindableAction> RebindableActions;
	int32 ListeningIndex = INDEX_NONE;
	TArray<TSharedPtr<STextBlock>> KeyTexts;

	void PopulateRebindableActions();
	void StartListening(int32 Index);
	void StopListening(FKey NewKey);
	void ResetToDefaults();

	static ULocalPlayer* GetLocalPlayer();
	static UEnhancedInputUserSettings* GetUserSettings();

	float GeneralVolume = 1.0f;
	float MusicVolume = 1.0f;
	float EffectsVolume = 1.0f;
	TSharedPtr<SSlider> GeneralVolumeSlider;
	TSharedPtr<SSlider> MusicVolumeSlider;
	TSharedPtr<SSlider> EffectsVolumeSlider;
	TSharedPtr<STextBlock> GeneralVolumeText;
	TSharedPtr<STextBlock> MusicVolumeText;
	TSharedPtr<STextBlock> EffectsVolumeText;

	void LoadSoundSettings();
	void SaveSoundSettings();
	void OnGeneralVolumeChanged(float Value);
	void OnMusicVolumeChanged(float Value);
	void OnEffectsVolumeChanged(float Value);

	TArray<FText> WindowModeOptions;
	TArray<TSharedPtr<FString>> ResolutionOptions;
	TArray<FIntPoint> ResolutionValues;
	TArray<FText> QualityOptions;
	TArray<FText> FPSOptions;
	TArray<float> FPSValues;

	int32 SelectedWindowMode = 0;
	int32 SelectedResolution = 0;
	bool bVSyncEnabled = false;
	int32 SelectedQuality = 3;
	int32 SelectedFPS = 0;

	TSharedPtr<STextBlock> WindowModeText;
	TSharedPtr<STextBlock> ResolutionText;
	TSharedPtr<STextBlock> QualityText;
	TSharedPtr<STextBlock> FPSText;

	float FieldOfView = 85.f;
	TSharedPtr<SSlider> FOVSlider;
	TSharedPtr<STextBlock> FOVValueText;

	void LoadVideoSettings();
	void ApplyVideoSettings();

	FSimpleDelegate OnBackDelegate;
	bool bShowLanguageTab = false;

	TArray<TSharedPtr<FString>> LanguageDisplayNames;
	TArray<FString> LanguageCultureCodes;
	int32 SelectedLanguage = 0;
	TArray<TSharedPtr<STextBlock>> LanguageButtonTexts;

	void LoadLanguageSetting();
	void SaveLanguageSetting();
	void ApplyLanguageSetting();

	static FSlateFontInfo GetTitleFont();
	static FSlateFontInfo GetButtonFont();
	static FSlateFontInfo GetLabelFont();
	static FSlateFontInfo GetHeaderFont();
	static FSlateFontInfo GetTabFont();
};
