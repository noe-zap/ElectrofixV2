#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbientNPCPath.generated.h"

class AAmbientNPCCharacter;
class AAmbientNPCSpawnPoint;
class AAmbientNPCExitPoint;

/**
 * Ambient NPC director. Drop one in the level. It auto-discovers every
 * AAmbientNPCSpawnPoint and AAmbientNPCExitPoint placed in the world at BeginPlay.
 *
 * Every SpawnInterval seconds, the director spawns NPCsPerWave new NPCs (subject to
 * the MaxAlive cap). Each NPC picks a random SpawnPoint to appear at and a random
 * ExitPoint to walk toward. On arrival at the exit, the NPC despawns.
 */
UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AAmbientNPCPath : public AActor
{
	GENERATED_BODY()

public:
	AAmbientNPCPath();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ambient NPC")
	TObjectPtr<USceneComponent> SceneRoot;

	// NPC classes that can spawn. One is chosen at random per NPC.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC|Classes")
	TArray<TSubclassOf<AAmbientNPCCharacter>> NPCClasses;

	// Seconds between spawn waves.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC|Spawning", meta = (ClampMin = "0.1"))
	float SpawnInterval = 5.f;

	// Number of NPCs spawned per wave (limited by MaxAlive).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC|Spawning", meta = (ClampMin = "1"))
	int32 NPCsPerWave = 2;

	// Maximum live NPCs the director keeps alive at once.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient NPC|Spawning", meta = (ClampMin = "1"))
	int32 MaxAlive = 12;

	UFUNCTION(BlueprintCallable, Category = "Ambient NPC")
	void SetSpawnPaused(bool bPaused);

	// Re-scan the world for spawn / exit points. Call this if you spawn new point
	// actors at runtime.
	UFUNCTION(BlueprintCallable, Category = "Ambient NPC")
	void RefreshPoints();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool bSpawnPaused = false;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAmbientNPCSpawnPoint>> SpawnPoints;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAmbientNPCExitPoint>> ExitPoints;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAmbientNPCCharacter>> Alive;

	FTimerHandle SpawnTimer;

	void ScheduleNextWave();
	void SpawnWave();
	void SpawnOne();
	void PurgeStale();
};
