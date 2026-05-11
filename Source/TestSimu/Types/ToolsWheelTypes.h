#pragma once

#include "CoreMinimal.h"
#include "ToolsWheelTypes.generated.h"

class ATool;

UENUM(BlueprintType)
enum class EToolType : uint8
{
	None   UMETA(DisplayName = "None"),
	Broom  UMETA(DisplayName = "Broom"),
	Sponge UMETA(DisplayName = "Sponge")
};

USTRUCT(BlueprintType)
struct FToolSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Slot")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Slot")
	TSubclassOf<ATool> ToolClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Slot")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Slot")
	FName AttachSocketName = FName("hand_r");
};
