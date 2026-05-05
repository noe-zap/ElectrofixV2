#include "CleaningTool.h"
#include "Cleaning/CleanableActor.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ACleaningTool::ACleaningTool()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACleaningTool::UseStart_Implementation()
{
	Multicast_PlayCleaningMontage();
	CleanTick();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CleanHoldTimer, this, &ACleaningTool::CleanTick, CleanInterval, true);
	}
}

void ACleaningTool::UseStop_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CleanHoldTimer);
	}

	Multicast_StopCleaningMontage();
}

void ACleaningTool::CleanTick()
{
	FVector Start, End;
	if (!ComputeAimRay(Start, End))
	{
		return;
	}

	if (HasAuthority())
	{
		DoCleanAt(Start, End);
	}
	else
	{
		ServerCleanAt(Start, End);
	}
}

bool ACleaningTool::ComputeAimRay(FVector& OutStart, FVector& OutEnd) const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (PC == nullptr)
	{
		return false;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return false;
	}

	FVector WorldLoc;
	FVector WorldDir;
	if (!PC->DeprojectScreenPositionToWorld(ViewportX * 0.5f, ViewportY * 0.5f, WorldLoc, WorldDir))
	{
		return false;
	}

	OutStart = WorldLoc;
	OutEnd = WorldLoc + WorldDir * CleanRange;
	return true;
}

void ACleaningTool::ServerCleanAt_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End)
{
	DoCleanAt(Start, End);
}

void ACleaningTool::DoCleanAt(const FVector& Start, const FVector& End)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CleaningTrace), false, this);
	Params.AddIgnoredActor(OwnerPawn);

	TArray<FHitResult> Hits;
	bool bAnyHit = false;
	if (CleanTraceRadius > 0.f)
	{
		bAnyHit = GetWorld()->SweepMultiByChannel(
			Hits, Start, End, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(CleanTraceRadius), Params);
	}
	else
	{
		bAnyHit = GetWorld()->LineTraceMultiByChannel(Hits, Start, End, ECC_Visibility, Params);
	}

	ACleanableActor* TargetCleanable = nullptr;
	FVector ImpactForDebug = End;
	for (const FHitResult& H : Hits)
	{
		ACleanableActor* Cleanable = Cast<ACleanableActor>(H.GetActor());
		if (Cleanable == nullptr)
		{
			continue;
		}

		UPrimitiveComponent* HitComp = H.GetComponent();
		if (HitComp == nullptr)
		{
			continue;
		}

		if (!CleaningTag.IsNone() && !HitComp->ComponentHasTag(CleaningTag))
		{
			continue;
		}

		TargetCleanable = Cleanable;
		ImpactForDebug = H.ImpactPoint;
		break;
	}

	if (UWorld* World = GetWorld())
	{
		const FColor LineColor = TargetCleanable ? FColor::Green : (bAnyHit ? FColor::Yellow : FColor::Red);
		DrawDebugLine(World, Start, TargetCleanable ? ImpactForDebug : End, LineColor, false, 2.f, 0, 1.f);
	}

	if (CleaningTag.IsNone())
	{
		return;
	}

	if (TargetCleanable == nullptr)
	{
		return;
	}

	TargetCleanable->ApplyClean(CleanAmountPerClick);
}

void ACleaningTool::Multicast_PlayCleaningMontage_Implementation()
{
	if (CleaningMontage == nullptr)
	{
		return;
	}

	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = OwningActor->FindComponentByClass<USkeletalMeshComponent>();
	if (OwnerMesh == nullptr)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Play(CleaningMontage);
	}
}

void ACleaningTool::Multicast_StopCleaningMontage_Implementation()
{
	if (CleaningMontage == nullptr)
	{
		return;
	}

	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = OwningActor->FindComponentByClass<USkeletalMeshComponent>();
	if (OwnerMesh == nullptr)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(CleaningMontage->BlendOut.GetBlendTime(), CleaningMontage);
	}
}
