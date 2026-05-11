#include "UI/SLoadingScreenWidget.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SLoadingScreenWidget::Construct(const FArguments& InArgs)
{
	if (InArgs._BackgroundTexture)
	{
		BackgroundBrush.SetResourceObject(InArgs._BackgroundTexture);
		BackgroundBrush.ImageSize = FVector2D(
			InArgs._BackgroundTexture->GetSizeX(),
			InArgs._BackgroundTexture->GetSizeY());
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
	}
	else
	{
		BackgroundBrush.TintColor = FLinearColor::Black;
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
	}

	const int32 FontSize = InArgs._TipFontSize > 0 ? InArgs._TipFontSize : 24;

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage).Image(&BackgroundBrush)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(50.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(InArgs._TipText)
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FontSize))
				.ColorAndOpacity(FLinearColor::White)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(20.f, 0.f, 0.f, 0.f)
			[
				SNew(SCircularThrobber)
				.Radius(20.f)
			]
		]
	];
}
