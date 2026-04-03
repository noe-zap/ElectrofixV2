#pragma once

#include "CoreMinimal.h"
#include "ToolsWheelTypes.generated.h"

class ATool;

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
};
