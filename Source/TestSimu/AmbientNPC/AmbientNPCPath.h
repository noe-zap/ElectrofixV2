#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbientNPCPath.generated.h"

class USplineComponent;
class AAmbientNPCCharacter;

/**
 * A single ambient NPC route. Drop one of these in the level, edit the spline to draw the
 * walk route, and assign which NPC blueprints can spawn on it. The actor is its own spawner:
 * it picks a random interval in [MinSpawnInterval, MaxSpawnInterval] and spawns a new NPC at
 * the start of the spline, who then walks to the end and despawns.
 */
UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AAmbientNPCPath : public AActor
{
	GENERATED_BODY()

public:
	AAmbientNPCPath();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ambient NPC Path")
	TObjectPtr<USplineComponent> Spline;

	// NPC classes that can spawn on this path. Random choice each cycle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC Path")
	TArray<TSubclassOf<AAmbientNPCCharacter>> NPCClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC Path", meta = (ClampMin = "0.5"))
	float MinSpawnInterval = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC Path", meta = (ClampMin = "0.5"))
	float MaxSpawnInterval = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC Path", meta = (ClampMin = "1"))
	int32 MaxAlive = 4;

	// Distance between sampled waypoints along the spline (cm). Smaller = more faithful to
	// the spline curvature, larger = cheaper. 300 cm is usually enough.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC Path", meta = (ClampMin = "50"))
	float WaypointSpacing = 300.f;

	UFUNCTION(BlueprintCallable, Category = "Ambient NPC Path")
	void SetSpawnPaused(bool bPaused);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool bSpawnPaused = false;

	UPROPERTY(Transient)
	TArray<FVector> CachedWaypoints;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAmbientNPCCharacter>> Alive;

	FTimerHandle SpawnTimer;

	void BuildWaypoints();
	void ScheduleNextSpawn();
	void TrySpawn();
	void PurgeStale();
};
