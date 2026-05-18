#include "AmbientNPCCharacter.h"
#include "AmbientNPCController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AAmbientNPCCharacter::AAmbientNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AAmbientNPCController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(5.f);
	SetMinNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(8000.f * 8000.f);
	SetCanBeDamaged(false);

	bUseControllerRotationYaw = false;

	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		M->bEnableUpdateRateOptimizations = true;
		M->bComponentUseFixedSkelBounds = true;
		M->CastShadow = false;
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCapsuleComponent* C = GetCapsuleComponent())
	{
		C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = 180.f;
		CMC->bOrientRotationToMovement = true;
		CMC->bRunPhysicsWithNoController = false;
		CMC->NetworkSmoothingMode = ENetworkSmoothingMode::Linear;
	}
}
