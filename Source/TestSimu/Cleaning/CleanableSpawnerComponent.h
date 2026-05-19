#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CleanableSpawnerComponent.generated.h"

class ACleanableActor;
class ACleanableSpawnPoint;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESTSIMU_API UCleanableSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCleanableSpawnerComponent();

	// Cleanable classes that can spawn. Random choice each tick.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanable Spawner")
	TArray<TSubclassOf<ACleanableActor>> CleanableClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanable Spawner", meta = (ClampMin = "1.0"))
	float MinSpawnInterval = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanable Spawner", meta = (ClampMin = "1.0"))
	float MaxSpawnInterval = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cleanable Spawner", meta = (ClampMin = "1"))
	int32 MaxAlive = 12;

	void RegisterSpawnPoint(ACleanableSpawnPoint* Point);
	void UnregisterSpawnPoint(ACleanableSpawnPoint* Point);

	// Server-authoritative: immediately spawn up to Count cleanables at free spawn points whose
	// CleaningTag matches Tag. Respects MaxAlive. Returns the number actually spawned. Use for
	// scripted moments (tutorial step completion, events) where dust should appear now rather
	// than waiting for the random ambient timer.
	UFUNCTION(BlueprintCallable, Category = "Cleanable Spawner")
	int32 ForceSpawnAtTag(FName Tag, int32 Count = 1);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACleanableSpawnPoint>> Anchors;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACleanableActor>> Alive;

	FTimerHandle SpawnTimer;

	void ScheduleNextSpawn();
	void TrySpawn();
	void PurgeStale();
	bool IsAnchorOccupied(const ACleanableSpawnPoint* Anchor) const;

	// Picks a compatible cleanable class for the anchor and spawns it. Returns the spawned actor
	// or nullptr if no compatible class exists or the spawn fails.
	ACleanableActor* SpawnAtAnchor(ACleanableSpawnPoint* Anchor);
};
