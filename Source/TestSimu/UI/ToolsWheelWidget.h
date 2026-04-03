#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/ToolsWheelTypes.h"
#include "ToolsWheelWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TESTSIMU_API UToolsWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Tools Wheel")
	TArray<FToolSlotInfo> Slots;

	UPROPERTY(BlueprintReadOnly, Category = "Tools Wheel")
	int32 HoveredIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	float WheelRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	float InnerRadius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	float IconSize = 48.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	FLinearColor BackgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	FLinearColor SectorColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	FLinearColor HighlightColor = FLinearColor(0.8f, 0.5f, 0.1f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	FLinearColor LineColor = FLinearColor(1.f, 1.f, 1.f, 0.3f);

	// Outer highlight arc on hovered slot (GTA style)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	FLinearColor HoverOutlineColor = FLinearColor(1.f, 0.6f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Style")
	float HoverOutlineThickness = 4.f;

	// Text style
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Text")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Text")
	int32 TextFontSize = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Text")
	FString FontTypeFace = TEXT("Regular");

	UFUNCTION(BlueprintCallable, Category = "Tools Wheel")
	void UpdateSelection(FVector2D MousePosition, FVector2D ScreenCenter);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tools Wheel")
	int32 GetHoveredIndex() const { return HoveredIndex; }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
