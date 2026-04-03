#include "XRayScanner.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

AXRayScanner::AXRayScanner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AXRayScanner::UseStart_Implementation()
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	bIsScanning = true;

	if (PC->PlayerCameraManager)
	{
		OriginalFOV = PC->PlayerCameraManager->GetFOVAngle();
	}

	SetActorTickEnabled(true);
}

void AXRayScanner::UseStop_Implementation()
{
	if (!bIsScanning)
	{
		return;
	}

	bIsScanning = false;
	RestoreAllMaterials();

	// Tick stays enabled to lerp FOV back to original — disabled once restored
}

void AXRayScanner::OnUnequipped_Implementation()
{
	if (bIsScanning)
	{
		UseStop();
	}

	// Snap FOV back immediately since the tool is being destroyed
	APlayerController* PC = GetOwnerPlayerController();
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->UnlockFOV();
	}

	SetActorTickEnabled(false);

	Super::OnUnequipped_Implementation();
}

void AXRayScanner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	// Determine FOV target
	const float DesiredFOV = bIsScanning ? TargetFOV : OriginalFOV;
	const float CurrentFOV = PC->PlayerCameraManager->GetFOVAngle();
	const float NewFOV = FMath::FInterpTo(CurrentFOV, DesiredFOV, DeltaTime, FOVInterpSpeed);

	PC->PlayerCameraManager->SetFOV(NewFOV);

	if (bIsScanning)
	{
		PerformScanTrace();
	}
	else
	{
		// Done restoring FOV — disable tick
		if (FMath::IsNearlyEqual(NewFOV, OriginalFOV, 0.1f))
		{
			PC->PlayerCameraManager->UnlockFOV();
			SetActorTickEnabled(false);
		}
	}
}

void AXRayScanner::PerformScanTrace()
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		UE_LOG(LogTemp, Verbose, TEXT("XRayScanner: Trace hit nothing"));
		return;
	}

	AActor* HitActor = Hit.GetActor();
	UE_LOG(LogTemp, Log, TEXT("XRayScanner: Trace hit Actor='%s' Component='%s'"),
		HitActor ? *HitActor->GetName() : TEXT("null"),
		Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("null"));

	if (!HitActor)
	{
		return;
	}

	// Direct hit on a tagged static mesh
	if (HitActor->ActorHasTag(XRayTag))
	{
		UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Hit.GetComponent());
		if (MeshComp)
		{
			UE_LOG(LogTemp, Log, TEXT("XRayScanner: Direct hit — applying to '%s'"), *MeshComp->GetName());
			ApplyXRayMaterial(MeshComp, Hit.ImpactPoint);
			return;
		}
	}

	// Hit a RepairOrder actor — find the first static mesh component with tag XRayScanner inside it
	if (HitActor->ActorHasTag(RepairOrderTag))
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		HitActor->GetComponents<UStaticMeshComponent>(MeshComponents);

		for (UStaticMeshComponent* MeshComp : MeshComponents)
		{
			if (MeshComp && MeshComp->ComponentHasTag(XRayTag))
			{
				UE_LOG(LogTemp, Log, TEXT("XRayScanner: RepairOrder hit — applying to component '%s' in '%s'"),
					*MeshComp->GetName(), *HitActor->GetName());
				ApplyXRayMaterial(MeshComp, Hit.ImpactPoint);
				return;
			}
		}

		UE_LOG(LogTemp, Log, TEXT("XRayScanner: RepairOrder '%s' has no component with tag '%s'"),
			*HitActor->GetName(), *XRayTag.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("XRayScanner: Actor '%s' has neither tag '%s' nor '%s'"),
		*HitActor->GetName(), *XRayTag.ToString(), *RepairOrderTag.ToString());
}

void AXRayScanner::ApplyXRayMaterial(UStaticMeshComponent* MeshComp, const FVector& HitLocation)
{
	if (!XRayMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRayScanner: No XRayMaterial assigned!"));
		return;
	}

	if (!AffectedMeshes.Contains(MeshComp))
	{
		// Store original materials
		FMaterialSlotArray OriginalMats;
		const int32 NumMaterials = MeshComp->GetNumMaterials();
		OriginalMats.Materials.Reserve(NumMaterials);

		for (int32 i = 0; i < NumMaterials; ++i)
		{
			OriginalMats.Materials.Add(MeshComp->GetMaterial(i));
		}

		// Create dynamic material instance and apply to all slots
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(XRayMaterial, this);
		ActiveDynamicMaterials.Add(DynMat);

		for (int32 i = 0; i < NumMaterials; ++i)
		{
			MeshComp->SetMaterial(i, DynMat);
		}

		AffectedMeshes.Add(MeshComp, MoveTemp(OriginalMats));
	}

	// Update sphere mask center to follow the hit point
	if (ActiveDynamicMaterials.Num() > 0)
	{
		// Find the dynamic material for this mesh (last added if new, or search)
		for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
		{
			UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(i));
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(HitLocationParamName, FLinearColor(HitLocation.X, HitLocation.Y, HitLocation.Z, 1.f));
				break;
			}
		}
	}
}

void AXRayScanner::RestoreAllMaterials()
{
	for (auto& Pair : AffectedMeshes)
	{
		UStaticMeshComponent* MeshComp = Pair.Key;
		if (!IsValid(MeshComp))
		{
			continue;
		}

		const TArray<TObjectPtr<UMaterialInterface>>& OriginalMats = Pair.Value.Materials;
		for (int32 i = 0; i < OriginalMats.Num(); ++i)
		{
			MeshComp->SetMaterial(i, OriginalMats[i]);
		}
	}

	AffectedMeshes.Empty();
	ActiveDynamicMaterials.Empty();
}

APlayerController* AXRayScanner::GetOwnerPlayerController() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		return Cast<APlayerController>(OwnerPawn->GetController());
	}
	return nullptr;
}
