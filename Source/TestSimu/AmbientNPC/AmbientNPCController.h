#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AmbientNPCController.generated.h"

UCLASS()
class TESTSIMU_API AAmbientNPCController : public AAIController
{
	GENERATED_BODY()

public:
	AAmbientNPCController();

	// Hand the controller an ordered list of world-space waypoints to walk.
	// On reaching the last one (or running out of retries), the pawn is destroyed.
	UFUNCTION()
	void AssignPath(const TArray<FVector>& InWaypoints);

protected:
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	UPROPERTY(Transient)
	TArray<FVector> Waypoints;

	int32 CurrentIndex = 0;
	int32 RetryCount = 0;

	static constexpr int32 MaxRetriesPerWaypoint = 2;
	static constexpr float AcceptanceRadius = 80.f;

	void MoveToCurrent();
	void AdvanceOrDestroy();
	void DestroyPawnSafe();
};
