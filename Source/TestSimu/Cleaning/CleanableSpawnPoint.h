#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CleanableSpawnPoint.generated.h"

class UBillboardComponent;
class UArrowComponent;
class ACleanableActor;

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API ACleanableSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ACleanableSpawnPoint();

	// Restricts which cleanables may spawn here. Matches against ACleanableActor::CleaningTag.
	// Leave NAME_None for a wildcard point that accepts any cleanable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanable Spawn Point")
	FName CleaningTag = NAME_None;

	// The cleanable currently occupying this slot (server only). Cleared automatically when
	// the cleanable is destroyed because TWeakObjectPtr nulls itself.
	UPROPERTY(Transient)
	TWeakObjectPtr<ACleanableActor> CurrentCleanable;

	bool IsOccupied() const { return CurrentCleanable.IsValid(); }

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;

	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
