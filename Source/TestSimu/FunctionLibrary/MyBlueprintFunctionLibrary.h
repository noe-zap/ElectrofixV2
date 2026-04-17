// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TESTSIMU_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Utilities")
	static FText MinutesToTimeText(int32 Minutes);

	UFUNCTION(BlueprintPure, Category = "Utilities")
	static FText OverTimeToTimeText(int32 Minutes);

	UFUNCTION(BlueprintPure, Category = "Utilities")
	static TArray<FName> GetRandomUniqueElements(const TArray<FName>& Names);

	UFUNCTION(BlueprintPure, Category = "Shop")
	static float CalculateXPFromOvertime(float OvertimeCost, float BaseXP = 20.0f, float MaxOvertimeCost = 60.0f);
};
