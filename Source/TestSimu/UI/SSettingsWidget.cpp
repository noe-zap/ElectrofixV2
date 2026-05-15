#include "UI/SSettingsWidget.h"
#include "InputMappingContext.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/SOverlay.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Misc/ConfigCacheIni.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"

static const TCHAR* MouseSettingsSection = TEXT("TestSimu.MouseSettings");
static const TCHAR* AudioSettingsSection = TEXT("TestSimu.Audio");
static const TCHAR* LanguageSettingsSection = TEXT("TestSimu.Language");

static FSlateFontInfo MakeFont(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle("Regular", Size);
}

FSlateFontInfo SSettingsWidget::GetTitleFont()  { return MakeFont(36); }
FSlateFontInfo SSettingsWidget::GetButtonFont() { return MakeFont(16); }
FSlateFontInfo SSettingsWidget::GetLabelFont()  { return MakeFont(14); }
FSlateFontInfo SSettingsWidget::GetHeaderFont() { return MakeFont(14); }
FSlateFontInfo SSettingsWidget::GetTabFont()    { return MakeFont(12); }

ULocalPlayer* SSettingsWidget::GetLocalPlayer()
{
	if (!GEngine || !GEngine->GameViewport) return nullptr;
	UGameInstance* GI = GEngine->GameViewport->GetGameInstance();
	if (!GI) return nullptr;
	return GI->GetFirstGamePlayer();
}

UEnhancedInputUserSettings* SSettingsWidget::GetUserSettings()
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return nullptr;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (Subsystem)
	{
		return Subsystem->GetUserSettings();
	}
	return UEnhancedInputUserSettings::LoadOrCreateSettings(LP);
}

void SSettingsWidget::Construct(const FArguments& InArgs)
{
	OnBackDelegate = InArgs._OnBack;
	bShowLanguageTab = InArgs._ShowLanguageTab;

	ResolutionOptions.Reset();
	RebindableActions.Reset();
	KeyTexts.Reset();
	TabButtonTexts.Reset();
	LanguageButtonTexts.Reset();

	WindowModeOptions = {
		NSLOCTEXT("TestSimu", "WindowMode_Fullscreen", "Fullscreen"),
		NSLOCTEXT("TestSimu", "WindowMode_WindowedFullscreen", "Windowed Fullscreen"),
		NSLOCTEXT("TestSimu", "WindowMode_Windowed", "Windowed")
	};

	ResolutionValues = { {1280,720}, {1600,900}, {1920,1080}, {2560,1440}, {3840,2160} };
	for (const FIntPoint& Res : ResolutionValues)
	{
		ResolutionOptions.Add(MakeShared<FString>(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y)));
	}

	QualityOptions = {
		NSLOCTEXT("TestSimu", "Quality_Low", "Low"),
		NSLOCTEXT("TestSimu", "Quality_Medium", "Medium"),
		NSLOCTEXT("TestSimu", "Quality_High", "High"),
		NSLOCTEXT("TestSimu", "Quality_Epic", "Epic")
	};

	FPSValues  = { 30.f, 60.f, 120.f, 144.f, 0.f };
	FPSOptions = {
		FText::FromString(TEXT("30")),
		FText::FromString(TEXT("60")),
		FText::FromString(TEXT("120")),
		FText::FromString(TEXT("144")),
		NSLOCTEXT("TestSimu", "FPS_Unlimited", "Unlimited")
	};

	PopulateRebindableActions();
	LoadVideoSettings();
	LoadMouseSettings();
	LoadSoundSettings();

	// --- Controls tab ---
	TSharedRef<SVerticalBox> ControlsContent = SNew(SVerticalBox);

	ControlsContent->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 8.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_MouseHeader", "Mouse")).Font(GetHeaderFont()).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f))
	];

	ControlsContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return bInvertYAxis ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bInvertYAxis = (State == ECheckBoxState::Checked); SaveMouseSettings(); ApplyMouseSettings(); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_InvertYAxis", "Invert Y Axis")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
		]
	];

	ControlsContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return bInvertXAxis ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bInvertXAxis = (State == ECheckBoxState::Checked); SaveMouseSettings(); ApplyMouseSettings(); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_InvertXAxis", "Invert X Axis")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
		]
	];

	ControlsContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_MouseSensitivity", "Mouse Sensitivity")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
		]
		+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
		[
			SAssignNew(SensitivitySlider, SSlider)
			.Value((MouseSensitivity - 0.1f) / 4.9f)
			.OnValueChanged_Lambda([this](float Value)
			{
				MouseSensitivity = 0.1f + Value * 4.9f;
				MouseSensitivity = FMath::RoundToFloat(MouseSensitivity * 10.f) / 10.f;
				if (SensitivityValueText.IsValid())
				{
					SensitivityValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), MouseSensitivity)));
				}
				SaveMouseSettings();
				ApplyMouseSettings();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SAssignNew(SensitivityValueText, STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%.1fx"), MouseSensitivity)))
			.Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
		]
	];

	ControlsContent->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 8.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_KeyBindingsHeader", "Key Bindings")).Font(GetHeaderFont()).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f))
	];

	for (int32 i = 0; i < RebindableActions.Num(); i++)
	{
		TSharedPtr<STextBlock> KeyText;
		ControlsContent->AddSlot().AutoHeight().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(RebindableActions[i].DisplayName)).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f)
			[
				SNew(SButton).HAlign(HAlign_Center)
				.OnClicked_Lambda([this, i]() { StartListening(i); return FReply::Handled(); })
				.ContentPadding(FMargin(20.f, 4.f))
				[
					SAssignNew(KeyText, STextBlock)
					.Text(FText::FromString(RebindableActions[i].CurrentKey.GetDisplayName().ToString()))
					.Font(GetButtonFont()).ColorAndOpacity(FLinearColor::White)
				]
			]
		];
		KeyTexts.Add(KeyText);
	}

	ControlsContent->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f)
	[
		SNew(SButton)
		.OnClicked_Lambda([this]() { ResetToDefaults(); return FReply::Handled(); })
		.ContentPadding(FMargin(20.f, 8.f))
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_ResetToDefaults", "Reset to Defaults")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.8f, 0.3f, 0.3f))
		]
	];

	// --- Video tab ---
	auto MakeCycleRow = [this](const FText& Label, TSharedPtr<STextBlock>& ValueText, const FText& InitialValue, TFunction<void()> OnCycle) -> TSharedRef<SHorizontalBox>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Label).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f)
			[
				SNew(SButton).HAlign(HAlign_Center)
				.OnClicked_Lambda([OnCycle]() { OnCycle(); return FReply::Handled(); })
				.ContentPadding(FMargin(15.f, 4.f))
				[
					SAssignNew(ValueText, STextBlock).Text(InitialValue).Font(GetButtonFont()).ColorAndOpacity(FLinearColor::White)
				]
			];
	};

	TSharedRef<SVerticalBox> VideoContent = SNew(SVerticalBox);

	VideoContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		MakeCycleRow(NSLOCTEXT("TestSimu", "Settings_WindowMode", "Window Mode"), WindowModeText,
			WindowModeOptions[SelectedWindowMode],
			[this]() {
				SelectedWindowMode = (SelectedWindowMode + 1) % WindowModeOptions.Num();
				if (WindowModeText.IsValid()) WindowModeText->SetText(WindowModeOptions[SelectedWindowMode]);
			})
	];

	VideoContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		MakeCycleRow(NSLOCTEXT("TestSimu", "Settings_Resolution", "Resolution"), ResolutionText,
			FText::FromString(*ResolutionOptions[SelectedResolution]),
			[this]() {
				SelectedResolution = (SelectedResolution + 1) % ResolutionOptions.Num();
				if (ResolutionText.IsValid()) ResolutionText->SetText(FText::FromString(*ResolutionOptions[SelectedResolution]));
			})
	];

	VideoContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_VSync", "VSync")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return bVSyncEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bVSyncEnabled = (State == ECheckBoxState::Checked); })
		]
	];

	VideoContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		MakeCycleRow(NSLOCTEXT("TestSimu", "Settings_Quality", "Quality"), QualityText,
			QualityOptions[SelectedQuality],
			[this]() {
				SelectedQuality = (SelectedQuality + 1) % QualityOptions.Num();
				if (QualityText.IsValid()) QualityText->SetText(QualityOptions[SelectedQuality]);
			})
	];

	VideoContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		MakeCycleRow(NSLOCTEXT("TestSimu", "Settings_FPSLimit", "FPS Limit"), FPSText,
			FPSOptions[SelectedFPS],
			[this]() {
				SelectedFPS = (SelectedFPS + 1) % FPSOptions.Num();
				if (FPSText.IsValid()) FPSText->SetText(FPSOptions[SelectedFPS]);
			})
	];

	VideoContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_FOV", "Field of View")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White)
		]
		+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
		[
			SAssignNew(FOVSlider, SSlider)
			.Value((FieldOfView - 60.f) / 60.f)
			.OnValueChanged_Lambda([this](float Value)
			{
				FieldOfView = 60.f + Value * 60.f;
				FieldOfView = FMath::RoundToFloat(FieldOfView);
				if (FOVValueText.IsValid())
				{
					FOVValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), FMath::RoundToInt(FieldOfView))));
				}
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SAssignNew(FOVValueText, STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%d"), FMath::RoundToInt(FieldOfView))))
			.Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
		]
	];

	VideoContent->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 20.f)
	[
		SNew(SButton)
		.OnClicked_Lambda([this]() { ApplyVideoSettings(); return FReply::Handled(); })
		.ContentPadding(FMargin(30.f, 10.f))
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_Apply", "Apply")).Font(GetButtonFont()).ColorAndOpacity(FLinearColor(0.2f, 1.f, 0.2f))
		]
	];

	// --- Sound tab ---
	TSharedRef<SVerticalBox> SoundContent = SNew(SVerticalBox);

	SoundContent->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 8.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_VolumeHeader", "Volume")).Font(GetHeaderFont()).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f))
	];

	SoundContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_MasterVolume", "Master Volume")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White) ]
		+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
		[ SAssignNew(GeneralVolumeSlider, SSlider).Value(GeneralVolume).OnValueChanged_Lambda([this](float Value) { OnGeneralVolumeChanged(Value); }) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[ SAssignNew(GeneralVolumeText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GeneralVolume * 100.f)))).Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)) ]
	];

	SoundContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_MusicVolume", "Music Volume")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White) ]
		+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
		[ SAssignNew(MusicVolumeSlider, SSlider).Value(MusicVolume).OnValueChanged_Lambda([this](float Value) { OnMusicVolumeChanged(Value); }) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[ SAssignNew(MusicVolumeText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(MusicVolume * 100.f)))).Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)) ]
	];

	SoundContent->AddSlot().AutoHeight().Padding(0.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_EffectsVolume", "Effects Volume")).Font(GetLabelFont()).ColorAndOpacity(FLinearColor::White) ]
		+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
		[ SAssignNew(EffectsVolumeSlider, SSlider).Value(EffectsVolume).OnValueChanged_Lambda([this](float Value) { OnEffectsVolumeChanged(Value); }) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[ SAssignNew(EffectsVolumeText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(EffectsVolume * 100.f)))).Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)) ]
	];

	// --- Gameplay tab (placeholder until concrete settings exist) ---
	TSharedRef<SVerticalBox> GameplayContent = SNew(SVerticalBox);
	GameplayContent->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 40.f)
	[
		SNew(STextBlock)
		.Text(NSLOCTEXT("TestSimu", "Settings_GameplayPlaceholder", "Gameplay options coming soon"))
		.Font(GetLabelFont()).ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
	];

	// --- General tab (language) — only shown from the main menu ---
	TSharedPtr<SWidget> GeneralContent;
	if (bShowLanguageTab)
	{
		LanguageCultureCodes = {
			TEXT("en"), TEXT("fr"), TEXT("it"), TEXT("de"), TEXT("es"),
			TEXT("ja"), TEXT("ko"), TEXT("pl"), TEXT("pt-BR"), TEXT("pt"),
			TEXT("ru"), TEXT("zh-Hans"), TEXT("es-419"), TEXT("tr")
		};
		LanguageDisplayNames = {
			MakeShared<FString>(TEXT("English")),
			MakeShared<FString>(TEXT("Français")),
			MakeShared<FString>(TEXT("Italiano")),
			MakeShared<FString>(TEXT("Deutsch")),
			MakeShared<FString>(TEXT("Español")),
			MakeShared<FString>(TEXT("日本語")),
			MakeShared<FString>(TEXT("한국어")),
			MakeShared<FString>(TEXT("Polski")),
			MakeShared<FString>(TEXT("Português (Brasil)")),
			MakeShared<FString>(TEXT("Português")),
			MakeShared<FString>(TEXT("Русский")),
			MakeShared<FString>(TEXT("简体中文")),
			MakeShared<FString>(TEXT("Español (Latinoamérica)")),
			MakeShared<FString>(TEXT("Türkçe"))
		};
		LoadLanguageSetting();

		TSharedRef<SVerticalBox> LangContent = SNew(SVerticalBox);
		LangContent->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 8.f)
		[
			SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_LanguageHeader", "Language")).Font(GetHeaderFont()).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f))
		];

		TSharedRef<SScrollBox> LanguageScrollBox = SNew(SScrollBox);
		for (int32 i = 0; i < LanguageDisplayNames.Num(); i++)
		{
			LanguageScrollBox->AddSlot().Padding(2.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this, i]()
				{
					SelectedLanguage = i;
					SaveLanguageSetting();
					ApplyLanguageSetting();
					for (int32 j = 0; j < LanguageButtonTexts.Num(); j++)
					{
						if (LanguageButtonTexts[j].IsValid())
							LanguageButtonTexts[j]->SetColorAndOpacity(j == i ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor::White);
					}
					return FReply::Handled();
				})
				.ContentPadding(FMargin(20.f, 6.f))
				[
					SAssignNew(LanguageButtonTexts.AddDefaulted_GetRef(), STextBlock)
					.Text(FText::FromString(*LanguageDisplayNames[i]))
					.Font(GetButtonFont())
					.ColorAndOpacity(i == SelectedLanguage ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor::White)
				]
			];
		}

		LangContent->AddSlot().FillHeight(1.f).Padding(0.f, 6.f)
		[ SNew(SBox).MaxDesiredHeight(300.f) [ LanguageScrollBox ] ];

		GeneralContent = LangContent;
	}

	// --- Tab bar ---
	TArray<FText> TabNames;
	TArray<TSharedRef<SWidget>> TabContents;

	if (bShowLanguageTab && GeneralContent.IsValid())
	{
		TabNames.Add(NSLOCTEXT("TestSimu", "Settings_TabGeneral", "General"));
		TabContents.Add(SNew(SScrollBox) + SScrollBox::Slot().Padding(0.f, 0.f, 16.f, 0.f) [ GeneralContent.ToSharedRef() ]);
	}
	TabNames.Add(NSLOCTEXT("TestSimu", "Settings_TabControls", "Controls"));
	TabContents.Add(SNew(SScrollBox) + SScrollBox::Slot().Padding(0.f, 0.f, 16.f, 0.f) [ ControlsContent ]);
	TabNames.Add(NSLOCTEXT("TestSimu", "Settings_TabSound", "Sound"));
	TabContents.Add(SNew(SScrollBox) + SScrollBox::Slot().Padding(0.f, 0.f, 16.f, 0.f) [ SoundContent ]);
	TabNames.Add(NSLOCTEXT("TestSimu", "Settings_TabVideo", "Video"));
	TabContents.Add(SNew(SScrollBox) + SScrollBox::Slot().Padding(0.f, 0.f, 16.f, 0.f) [ VideoContent ]);
	TabNames.Add(NSLOCTEXT("TestSimu", "Settings_TabGameplay", "Gameplay"));
	TabContents.Add(SNew(SScrollBox) + SScrollBox::Slot().Padding(0.f, 0.f, 16.f, 0.f) [ GameplayContent ]);

	TSharedRef<SHorizontalBox> TabBar = SNew(SHorizontalBox);
	for (int32 i = 0; i < TabNames.Num(); i++)
	{
		TSharedPtr<STextBlock> TabText;
		TabBar->AddSlot().FillWidth(1.f).Padding(4.f, 0.f)
		[
			SNew(SButton).HAlign(HAlign_Center)
			.OnClicked_Lambda([this, i]() { SwitchTab(i); return FReply::Handled(); })
			.ContentPadding(FMargin(8.f, 6.f))
			[
				SAssignNew(TabText, STextBlock).Text(TabNames[i])
				.Font(GetTabFont())
				.ColorAndOpacity(i == 0 ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor(0.5f, 0.5f, 0.5f))
			]
		];
		TabButtonTexts.Add(TabText);
	}

	TSharedRef<SWidgetSwitcher> Switcher = SAssignNew(TabSwitcher, SWidgetSwitcher);
	for (const TSharedRef<SWidget>& Content : TabContents)
	{
		Switcher->AddSlot() [ Content ];
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
		[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_Title", "Settings")).Font(GetTitleFont()).ColorAndOpacity(FLinearColor(1.f, 0.8f, 0.2f)) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f) [ TabBar ]

		+ SVerticalBox::Slot().FillHeight(1.f) [ Switcher ]

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(SButton)
			.OnClicked_Lambda([this]() { OnBackDelegate.ExecuteIfBound(); return FReply::Handled(); })
			.ContentPadding(FMargin(40.f, 10.f))
			[ SNew(STextBlock).Text(NSLOCTEXT("TestSimu", "Settings_Back", "Back")).Font(GetButtonFont()).ColorAndOpacity(FLinearColor::White) ]
		]
	];
}

void SSettingsWidget::SwitchTab(int32 Index)
{
	ActiveTabIndex = Index;
	if (TabSwitcher.IsValid()) TabSwitcher->SetActiveWidgetIndex(Index);
	for (int32 i = 0; i < TabButtonTexts.Num(); i++)
	{
		if (TabButtonTexts[i].IsValid())
		{
			TabButtonTexts[i]->SetColorAndOpacity(i == Index ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor(0.5f, 0.5f, 0.5f));
		}
	}
}

void SSettingsWidget::PopulateRebindableActions()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings) return;

	UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile();
	if (!Profile) return;

	const TMap<FName, FKeyMappingRow>& Rows = Profile->GetPlayerMappingRows();

	for (const auto& Pair : Rows)
	{
		const FKeyMappingRow& Row = Pair.Value;
		for (const FPlayerKeyMapping& Mapping : Row.Mappings)
		{
			if (Mapping.GetSlot() != EPlayerMappableKeySlot::First) continue;

			FRebindableAction Entry;
			Entry.MappingName = Mapping.GetMappingName();
			Entry.DisplayName = Mapping.GetDisplayName().ToString();
			Entry.DefaultKey = Mapping.GetDefaultKey();
			Entry.CurrentKey = Mapping.GetCurrentKey();
			RebindableActions.Add(Entry);
		}
	}
}

void SSettingsWidget::StartListening(int32 Index)
{
	if (!RebindableActions.IsValidIndex(Index)) return;

	if (ListeningIndex != INDEX_NONE && KeyTexts.IsValidIndex(ListeningIndex) && KeyTexts[ListeningIndex].IsValid())
	{
		KeyTexts[ListeningIndex]->SetText(FText::FromString(RebindableActions[ListeningIndex].CurrentKey.GetDisplayName().ToString()));
	}

	ListeningIndex = Index;
	if (KeyTexts.IsValidIndex(Index) && KeyTexts[Index].IsValid())
	{
		KeyTexts[Index]->SetText(NSLOCTEXT("TestSimu", "Settings_ListeningForKey", "..."));
	}
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}

void SSettingsWidget::StopListening(FKey NewKey)
{
	if (ListeningIndex == INDEX_NONE) return;

	UEnhancedInputUserSettings* UserSettings = GetUserSettings();

	for (int32 i = 0; i < RebindableActions.Num(); i++)
	{
		if (i != ListeningIndex && RebindableActions[i].CurrentKey == NewKey)
		{
			FKey SwappedKey = RebindableActions[ListeningIndex].CurrentKey;
			RebindableActions[i].CurrentKey = SwappedKey;
			if (KeyTexts.IsValidIndex(i) && KeyTexts[i].IsValid())
			{
				KeyTexts[i]->SetText(FText::FromString(SwappedKey.GetDisplayName().ToString()));
			}
			if (UserSettings)
			{
				FMapPlayerKeyArgs SwapArgs;
				SwapArgs.MappingName = RebindableActions[i].MappingName;
				SwapArgs.Slot = EPlayerMappableKeySlot::First;
				SwapArgs.NewKey = SwappedKey;
				FGameplayTagContainer FailureReason;
				UserSettings->MapPlayerKey(SwapArgs, FailureReason);
			}
			break;
		}
	}

	RebindableActions[ListeningIndex].CurrentKey = NewKey;
	if (KeyTexts.IsValidIndex(ListeningIndex) && KeyTexts[ListeningIndex].IsValid())
	{
		KeyTexts[ListeningIndex]->SetText(FText::FromString(NewKey.GetDisplayName().ToString()));
	}

	if (UserSettings)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = RebindableActions[ListeningIndex].MappingName;
		Args.Slot = EPlayerMappableKeySlot::First;
		Args.NewKey = NewKey;
		FGameplayTagContainer FailureReason;
		UserSettings->MapPlayerKey(Args, FailureReason);
		UserSettings->SaveSettings();
	}

	ListeningIndex = INDEX_NONE;
}

FReply SSettingsWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (ListeningIndex != INDEX_NONE)
	{
		FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Escape)
		{
			if (KeyTexts.IsValidIndex(ListeningIndex) && KeyTexts[ListeningIndex].IsValid())
			{
				KeyTexts[ListeningIndex]->SetText(FText::FromString(RebindableActions[ListeningIndex].CurrentKey.GetDisplayName().ToString()));
			}
			ListeningIndex = INDEX_NONE;
			return FReply::Handled();
		}
		StopListening(Key);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SSettingsWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (ListeningIndex != INDEX_NONE)
	{
		FKey Key = MouseEvent.GetEffectingButton();
		if (Key == EKeys::LeftMouseButton) return FReply::Unhandled();
		StopListening(Key);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SSettingsWidget::ResetToDefaults()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings) return;

	UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile();
	if (!Profile) return;

	Profile->ResetToDefault();
	UserSettings->SaveSettings();

	for (int32 i = 0; i < RebindableActions.Num(); i++)
	{
		RebindableActions[i].CurrentKey = RebindableActions[i].DefaultKey;
		if (KeyTexts.IsValidIndex(i) && KeyTexts[i].IsValid())
		{
			KeyTexts[i]->SetText(FText::FromString(RebindableActions[i].DefaultKey.GetDisplayName().ToString()));
		}
	}
}

void SSettingsWidget::LoadVideoSettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return;

	switch (Settings->GetFullscreenMode())
	{
		case EWindowMode::Fullscreen:         SelectedWindowMode = 0; break;
		case EWindowMode::WindowedFullscreen: SelectedWindowMode = 1; break;
		case EWindowMode::Windowed:           SelectedWindowMode = 2; break;
		default:                              SelectedWindowMode = 0; break;
	}

	FIntPoint CurrentRes = Settings->GetScreenResolution();
	SelectedResolution = 0;
	for (int32 i = 0; i < ResolutionValues.Num(); i++)
	{
		if (ResolutionValues[i] == CurrentRes) { SelectedResolution = i; break; }
	}

	bVSyncEnabled = Settings->IsVSyncEnabled();

	int32 QualityLevel = Settings->GetOverallScalabilityLevel();
	SelectedQuality = (QualityLevel >= 0) ? FMath::Clamp(QualityLevel, 0, QualityOptions.Num() - 1) : 3;

	float FrameRateLimit = Settings->GetFrameRateLimit();
	SelectedFPS = FPSValues.Num() - 1;
	for (int32 i = 0; i < FPSValues.Num(); i++)
	{
		if (FPSValues[i] > 0.f && FMath::IsNearlyEqual(FPSValues[i], FrameRateLimit, 1.0f))
		{
			SelectedFPS = i; break;
		}
	}

	FString FOVValue;
	if (GConfig->GetString(MouseSettingsSection, TEXT("FieldOfView"), FOVValue, GGameUserSettingsIni))
	{
		FieldOfView = FMath::Clamp(FCString::Atof(*FOVValue), 60.f, 120.f);
	}
}

void SSettingsWidget::ApplyVideoSettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return;

	static const EWindowMode::Type WindowModes[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
	Settings->SetFullscreenMode(WindowModes[FMath::Clamp(SelectedWindowMode, 0, 2)]);

	if (ResolutionValues.IsValidIndex(SelectedResolution))
		Settings->SetScreenResolution(ResolutionValues[SelectedResolution]);

	Settings->SetVSyncEnabled(bVSyncEnabled);
	Settings->SetOverallScalabilityLevel(SelectedQuality);

	if (FPSValues.IsValidIndex(SelectedFPS))
		Settings->SetFrameRateLimit(FPSValues[SelectedFPS]);

	Settings->ApplySettings(true);
	Settings->SaveSettings();

	GConfig->SetString(MouseSettingsSection, TEXT("FieldOfView"), *FString::Printf(TEXT("%.0f"), FieldOfView), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);

	ULocalPlayer* LP = GetLocalPlayer();
	if (LP)
	{
		APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->SetFOV(FieldOfView);
		}
	}
}

void SSettingsWidget::LoadMouseSettings()
{
	FString Value;
	if (GConfig->GetString(MouseSettingsSection, TEXT("InvertXAxis"), Value, GGameUserSettingsIni))
		bInvertXAxis = Value.ToBool();
	if (GConfig->GetString(MouseSettingsSection, TEXT("InvertYAxis"), Value, GGameUserSettingsIni))
		bInvertYAxis = Value.ToBool();
	if (GConfig->GetString(MouseSettingsSection, TEXT("MouseSensitivity"), Value, GGameUserSettingsIni))
	{
		MouseSensitivity = FMath::Clamp(FCString::Atof(*Value), 0.1f, 5.0f);
	}
}

void SSettingsWidget::SaveMouseSettings()
{
	GConfig->SetString(MouseSettingsSection, TEXT("InvertXAxis"), bInvertXAxis ? TEXT("true") : TEXT("false"), GGameUserSettingsIni);
	GConfig->SetString(MouseSettingsSection, TEXT("InvertYAxis"), bInvertYAxis ? TEXT("true") : TEXT("false"), GGameUserSettingsIni);
	GConfig->SetString(MouseSettingsSection, TEXT("MouseSensitivity"), *FString::Printf(TEXT("%.2f"), MouseSensitivity), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void SSettingsWidget::ApplyMouseSettings()
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (LP)
	{
		APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
		if (PC) ApplySavedMouseSettings(PC);
	}
}

void SSettingsWidget::ApplySavedMouseSettings(APlayerController* PC)
{
	if (!PC) return;

	FString Value;
	bool bInvertX = false;
	bool bInvertY = false;
	float Sensitivity = 0.3f;

	if (GConfig->GetString(MouseSettingsSection, TEXT("InvertXAxis"), Value, GGameUserSettingsIni))
		bInvertX = Value.ToBool();
	if (GConfig->GetString(MouseSettingsSection, TEXT("InvertYAxis"), Value, GGameUserSettingsIni))
		bInvertY = Value.ToBool();
	if (GConfig->GetString(MouseSettingsSection, TEXT("MouseSensitivity"), Value, GGameUserSettingsIni))
		Sensitivity = FMath::Clamp(FCString::Atof(*Value), 0.1f, 5.0f);

	const float BaseYaw = 2.5f;
	const float BasePitch = -2.5f;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	PC->InputYawScale_DEPRECATED = BaseYaw * Sensitivity * (bInvertX ? -1.f : 1.f);
	PC->InputPitchScale_DEPRECATED = BasePitch * Sensitivity * (bInvertY ? -1.f : 1.f);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void SSettingsWidget::LoadSoundSettings()
{
	FString Value;
	if (GConfig->GetString(AudioSettingsSection, TEXT("MasterVolume"), Value, GGameUserSettingsIni))
		GeneralVolume = FMath::Clamp(FCString::Atof(*Value), 0.f, 1.f);
	if (GConfig->GetString(AudioSettingsSection, TEXT("MusicVolume"), Value, GGameUserSettingsIni))
		MusicVolume = FMath::Clamp(FCString::Atof(*Value), 0.f, 1.f);
	if (GConfig->GetString(AudioSettingsSection, TEXT("EffectsVolume"), Value, GGameUserSettingsIni))
		EffectsVolume = FMath::Clamp(FCString::Atof(*Value), 0.f, 1.f);
}

void SSettingsWidget::SaveSoundSettings()
{
	GConfig->SetString(AudioSettingsSection, TEXT("MasterVolume"), *FString::Printf(TEXT("%.3f"), GeneralVolume), GGameUserSettingsIni);
	GConfig->SetString(AudioSettingsSection, TEXT("MusicVolume"), *FString::Printf(TEXT("%.3f"), MusicVolume), GGameUserSettingsIni);
	GConfig->SetString(AudioSettingsSection, TEXT("EffectsVolume"), *FString::Printf(TEXT("%.3f"), EffectsVolume), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void SSettingsWidget::OnGeneralVolumeChanged(float Value)
{
	GeneralVolume = Value;
	if (GeneralVolumeText.IsValid())
		GeneralVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GeneralVolume * 100.f))));
	SaveSoundSettings();
}

void SSettingsWidget::OnMusicVolumeChanged(float Value)
{
	MusicVolume = Value;
	if (MusicVolumeText.IsValid())
		MusicVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(MusicVolume * 100.f))));
	SaveSoundSettings();
}

void SSettingsWidget::OnEffectsVolumeChanged(float Value)
{
	EffectsVolume = Value;
	if (EffectsVolumeText.IsValid())
		EffectsVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(EffectsVolume * 100.f))));
	SaveSoundSettings();
}

void SSettingsWidget::LoadLanguageSetting()
{
	FString SavedCulture;
	if (GConfig->GetString(LanguageSettingsSection, TEXT("Culture"), SavedCulture, GGameUserSettingsIni))
	{
		for (int32 i = 0; i < LanguageCultureCodes.Num(); i++)
		{
			if (LanguageCultureCodes[i] == SavedCulture)
			{
				SelectedLanguage = i;
				return;
			}
		}
	}

	const FString CurrentCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	for (int32 i = 0; i < LanguageCultureCodes.Num(); i++)
	{
		if (CurrentCulture.StartsWith(LanguageCultureCodes[i]))
		{
			SelectedLanguage = i;
			return;
		}
	}
	SelectedLanguage = 0;
}

void SSettingsWidget::SaveLanguageSetting()
{
	if (LanguageCultureCodes.IsValidIndex(SelectedLanguage))
	{
		GConfig->SetString(LanguageSettingsSection, TEXT("Culture"), *LanguageCultureCodes[SelectedLanguage], GGameUserSettingsIni);
		GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *LanguageCultureCodes[SelectedLanguage], GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void SSettingsWidget::ApplyLanguageSetting()
{
	if (LanguageCultureCodes.IsValidIndex(SelectedLanguage))
	{
		FInternationalization::Get().SetCurrentCulture(LanguageCultureCodes[SelectedLanguage]);
	}
}
