#include "AmbientNPCController.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

AAmbientNPCController::AAmbientNPCController()
{
	PrimaryActorTick.bCanEverTick = false;
	bWantsPlayerState = false;
	bAttachToPawn = false;
}

void AAmbientNPCController::AssignDestination(const FVector& InDestination)
{
	Destination = InDestination;
	RetryCount = 0;
	MoveToLocation(Destination, AcceptanceRadius);
}

void AAmbientNPCController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (Result.IsSuccess())
	{
		DestroyPawnSafe();
		return;
	}

	if (++RetryCount > MaxRetries)
	{
		DestroyPawnSafe();
		return;
	}

	MoveToLocation(Destination, AcceptanceRadius);
}

void AAmbientNPCController::DestroyPawnSafe()
{
	if (APawn* P = GetPawn())
	{
		P->Destroy();
	}
}
