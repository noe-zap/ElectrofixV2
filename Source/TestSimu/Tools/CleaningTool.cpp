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

DEFINE_LOG_CATEGORY_STATIC(LogCleaning, Log, All);

ACleaningTool::ACleaningTool()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACleaningTool::UseStart_Implementation()
{
	UE_LOG(LogCleaning, Log, TEXT("[Tool] UseStart called (HasAuthority=%s)"),
		HasAuthority() ? TEXT("yes") : TEXT("no"));

	FVector Start, End;
	if (!ComputeAimRay(Start, End))
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Tool] Could not compute aim ray (no local PC / no viewport)."));
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
	UE_LOG(LogCleaning, Log, TEXT("[Tool] ServerCleanAt RPC received."));
	DoCleanAt(Start, End);
}

void ACleaningTool::DoCleanAt(const FVector& Start, const FVector& End)
{
	UE_LOG(LogCleaning, Log, TEXT("[Tool] DoCleanAt — tool=%s owner=%s tag='%s' Start=%s End=%s"),
		*GetName(),
		GetOwner() ? *GetOwner()->GetName() : TEXT("null"),
		*CleaningTag.ToString(),
		*Start.ToCompactString(), *End.ToCompactString());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Tool] No owner pawn — aborting."));
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

	Multicast_PlayCleaningMontage();

	// Walk every hit along the ray and pick the first valid cleanable target.
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
			UE_LOG(LogCleaning, Verbose, TEXT("[Tool] Skipping cleanable %s: component '%s' lacks tag '%s'"),
				*Cleanable->GetName(), *HitComp->GetName(), *CleaningTag.ToString());
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
		UE_LOG(LogCleaning, Warning, TEXT("[Tool] CleaningTag is None on this tool — fill it in the BP."));
		return;
	}

	if (TargetCleanable == nullptr)
	{
		FString HitDump;
		for (const FHitResult& H : Hits)
		{
			HitDump += FString::Printf(TEXT(" [%s/%s @%s]"),
				H.GetActor() ? *H.GetActor()->GetName() : TEXT("?"),
				H.GetComponent() ? *H.GetComponent()->GetName() : TEXT("?"),
				*H.ImpactPoint.ToCompactString());
		}
		UE_LOG(LogCleaning, Log, TEXT("[Tool] No cleanable along ray. %d hits:%s"), Hits.Num(), *HitDump);
		return;
	}

	UE_LOG(LogCleaning, Log, TEXT("[Tool] Applying clean %.2f to %s (current dirt=%.2f)."),
		CleanAmountPerClick, *TargetCleanable->GetName(), TargetCleanable->DirtAmount);
	TargetCleanable->ApplyClean(CleanAmountPerClick);
}

void ACleaningTool::Multicast_PlayCleaningMontage_Implementation()
{
	if (CleaningMontage == nullptr)
	{
		UE_LOG(LogCleaning, Verbose, TEXT("[Tool] Multicast: no montage assigned."));
		return;
	}

	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Tool] Multicast: no owner actor."));
		return;
	}

	USkeletalMeshComponent* OwnerMesh = OwningActor->FindComponentByClass<USkeletalMeshComponent>();
	if (OwnerMesh == nullptr)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Tool] Multicast: owner has no skeletal mesh."));
		return;
	}

	if (UAnimInstance* AnimInstance = OwnerMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Play(CleaningMontage);
	}
	else
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Tool] Multicast: skeletal mesh has no AnimInstance."));
	}
}
