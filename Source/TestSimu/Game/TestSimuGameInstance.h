#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TestSimuGameInstance.generated.h"

class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TESTSIMU_API UTestSimuGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
	TObjectPtr<UTexture2D> LoadingBackground = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
	FText LoadingTipText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen", meta = (ClampMin = "8", ClampMax = "96"))
	int32 LoadingTipFontSize = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen", meta = (ClampMin = "0.0"))
	float MinimumLoadingScreenDisplayTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
	bool bAllowEngineTickDuringLoad = false;

protected:
	void HandlePreLoadMap(const FString& MapName);

private:
	FString EditorCulture;
};
