#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbientNPCExitPoint.generated.h"

class UBillboardComponent;
class UArrowComponent;

/**
 * A placeable exit marker for ambient NPCs. Drop one (or many) into the level.
 * Each spawned NPC picks one of these at random as its destination, then despawns
 * on arrival.
 */
UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AAmbientNPCExitPoint : public AActor
{
	GENERATED_BODY()

public:
	AAmbientNPCExitPoint();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;

	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
