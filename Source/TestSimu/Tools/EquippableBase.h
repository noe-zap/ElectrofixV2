#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquippableBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AEquippableBase : public AActor
{
	GENERATED_BODY()

public:
	AEquippableBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
