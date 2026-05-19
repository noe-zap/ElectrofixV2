#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbientNPCSpawnPoint.generated.h"

class UBillboardComponent;
class UArrowComponent;

/**
 * A placeable spawn marker for ambient NPCs. Drop one (or many) into the level.
 * The AmbientNPCPath director auto-discovers every instance at BeginPlay and uses
 * them as candidate spawn locations.
 */
UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AAmbientNPCSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AAmbientNPCSpawnPoint();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;

	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
