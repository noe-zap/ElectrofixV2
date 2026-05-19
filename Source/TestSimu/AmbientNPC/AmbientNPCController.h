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

	// Send the pawn toward a single world-space destination. On arrival the pawn destroys
	// itself. The navmesh handles the actual pathing so movement is continuous (no
	// per-segment braking).
	UFUNCTION()
	void AssignDestination(const FVector& InDestination);

protected:
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	FVector Destination = FVector::ZeroVector;
	int32 RetryCount = 0;

	static constexpr int32 MaxRetries = 3;
	static constexpr float AcceptanceRadius = 80.f;

	void DestroyPawnSafe();
};
