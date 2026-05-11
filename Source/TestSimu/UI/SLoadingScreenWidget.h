#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class UTexture2D;

class SLoadingScreenWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadingScreenWidget) {}
		SLATE_ARGUMENT(UTexture2D*, BackgroundTexture)
		SLATE_ARGUMENT(FText, TipText)
		SLATE_ARGUMENT(int32, TipFontSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FSlateBrush BackgroundBrush;
};
