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
};
