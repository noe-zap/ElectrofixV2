#include "AmbientNPCController.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

AAmbientNPCController::AAmbientNPCController()
{
	PrimaryActorTick.bCanEverTick = false;
	bWantsPlayerState = false;
	bAttachToPawn = false;
}

void AAmbientNPCController::AssignPath(const TArray<FVector>& InWaypoints)
{
	Waypoints = InWaypoints;
	CurrentIndex = 0;
	RetryCount = 0;

	if (Waypoints.Num() == 0)
	{
		DestroyPawnSafe();
		return;
	}

	MoveToCurrent();
}

void AAmbientNPCController::MoveToCurrent()
{
	if (!Waypoints.IsValidIndex(CurrentIndex))
	{
		DestroyPawnSafe();
		return;
	}
	MoveToLocation(Waypoints[CurrentIndex], AcceptanceRadius);
}

void AAmbientNPCController::AdvanceOrDestroy()
{
	++CurrentIndex;
	RetryCount = 0;
	if (CurrentIndex >= Waypoints.Num())
	{
		DestroyPawnSafe();
		return;
	}
	MoveToCurrent();
}

void AAmbientNPCController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (Result.IsSuccess())
	{
		AdvanceOrDestroy();
		return;
	}

	if (++RetryCount > MaxRetriesPerWaypoint)
	{
		// Skip this waypoint and try the next one. If we're at the end, give up.
		AdvanceOrDestroy();
		return;
	}

	MoveToCurrent();
}

void AAmbientNPCController::DestroyPawnSafe()
{
	if (APawn* P = GetPawn())
	{
		P->Destroy();
	}
}
