#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/TutorialTypes.h"
#include "TutorialSequence.generated.h"

UCLASS(BlueprintType)
class TESTSIMU_API UTutorialSequence : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Ordered list of tutorial steps. Play from index 0 upward.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TArray<FTutorialStepData> Steps;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
