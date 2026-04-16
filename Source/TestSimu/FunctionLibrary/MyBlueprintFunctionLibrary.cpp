// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/MyBlueprintFunctionLibrary.h"

FText UMyBlueprintFunctionLibrary::MinutesToTimeText(int32 Minutes)
{
	const int32 Hours = Minutes / 60;
	const int32 Mins = Minutes % 60;

	if (Hours >= 1)
	{
		return FText::FromString(FString::Printf(TEXT("%dh%02d"), Hours, Mins));
	}

	return FText::FromString(FString::Printf(TEXT("%d min"), Mins));
}

FText UMyBlueprintFunctionLibrary::OverTimeToTimeText(int32 Minutes)
{
	const int32 Absolute = FMath::Abs(Minutes);
	const int32 Hours = Absolute / 60;
	const int32 Mins = Absolute % 60;

	if (Hours >= 1)
	{
		return FText::FromString(FString::Printf(TEXT("+%dh%02d"), Hours, Mins));
	}

	return FText::FromString(FString::Printf(TEXT("+%d min"), Mins));
}

TArray<FName> UMyBlueprintFunctionLibrary::GetRandomUniqueElements(const TArray<FName>& Names)
{
	if (Names.Num() == 0)
	{
		return TArray<FName>();
	}

	const int32 Count = FMath::RandRange(1, Names.Num());

	TArray<FName> Shuffled = Names;
	for (int32 i = Shuffled.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Shuffled.Swap(i, j);
	}

	Shuffled.SetNum(Count);
	return Shuffled;
}

