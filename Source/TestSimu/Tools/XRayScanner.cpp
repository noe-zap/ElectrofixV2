#include "XRayScanner.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

AXRayScanner::AXRayScanner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TraceOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TraceOrigin"));
	TraceOrigin->SetupAttachment(RootComponent);
}

void AXRayScanner::ActivateScanner(FVector Location, FRotator Rotation)
{
	bIsScanning = true;
	TargetLocation = Location;
	ActivationOrigin = Location;
	SetActorLocation(Location);
	SetActorRotation(Rotation);
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
}

void AXRayScanner::DeactivateScanner()
{
	if (!bIsScanning)
	{
		return;
	}

	bIsScanning = false;
	RestoreAllMaterials();
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);

	UE_LOG(LogTemp, Log, TEXT("XRayScanner: Deactivated"));
}

void AXRayScanner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsScanning)
	{
		return;
	}

	UpdatePositionFromMouse(DeltaTime);
	PerformScanTrace();
}

void AXRayScanner::UpdatePositionFromMouse(float DeltaTime)
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return;
	}

	float DeltaX, DeltaY;
	PC->GetInputMouseDelta(DeltaX, DeltaY);

	// Only update the target when the mouse is actually moving
	if (!FMath::IsNearlyZero(DeltaX) || !FMath::IsNearlyZero(DeltaY))
	{
		APlayerCameraManager* CamManager = PC->PlayerCameraManager;
		if (CamManager)
		{
			const FRotator CamRotation = CamManager->GetCameraRotation();
			const FVector CamRight = FRotationMatrix(CamRotation).GetUnitAxis(EAxis::Y);
			const FVector CamUp = FRotationMatrix(CamRotation).GetUnitAxis(EAxis::Z);

			// Project camera axes onto the horizontal plane (only X/Y, no Z)
			FVector Right2D = FVector(CamRight.X, CamRight.Y, 0.f).GetSafeNormal();
			FVector Up2D = FVector(CamUp.X, CamUp.Y, 0.f).GetSafeNormal();

			TargetLocation += (Right2D * DeltaX + Up2D * DeltaY) * MouseSensitivity;
		}
	}

	// Clamp target to configurable world-space bounds relative to activation origin
	const FVector ClampedMin = ActivationOrigin + BoundsMin;
	const FVector ClampedMax = ActivationOrigin + BoundsMax;
	TargetLocation.X = FMath::Clamp(TargetLocation.X, ClampedMin.X, ClampedMax.X);
	TargetLocation.Y = FMath::Clamp(TargetLocation.Y, ClampedMin.Y, ClampedMax.Y);
	TargetLocation.Z = FMath::Clamp(TargetLocation.Z, ClampedMin.Z, ClampedMax.Z);

	// Smoothly interpolate actual position toward target (lag/sway effect)
	const FVector CurrentLocation = GetActorLocation();
	FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, MoveInterpSpeed);

	// Clamp the interpolated position too so the actor never visually exceeds bounds
	NewLocation.X = FMath::Clamp(NewLocation.X, ClampedMin.X, ClampedMax.X);
	NewLocation.Y = FMath::Clamp(NewLocation.Y, ClampedMin.Y, ClampedMax.Y);
	NewLocation.Z = FMath::Clamp(NewLocation.Z, ClampedMin.Z, ClampedMax.Z);
	SetActorLocation(NewLocation);
}

void AXRayScanner::PerformScanTrace()
{
	// Trace straight down (-Z) from the configurable TraceOrigin point
	const FVector TraceStart = TraceOrigin->GetComponentLocation();
	const FVector TraceEnd = TraceStart + FVector(0.f, 0.f, -1.f) * TraceDistance;

	// Debug draw the trace line (green = trace, red = hit point)
	//DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, -1.f, 0, 1.f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		UE_LOG(LogTemp, Verbose, TEXT("XRayScanner: Scan trace hit nothing"));
		return;
	}

	AActor* HitActor = Hit.GetActor();
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
			ApplyXRayMaterial(MeshComp, Hit.ImpactPoint);
			return;
		}
	}

	// Hit a RepairOrder actor — find first static mesh component with tag XRayScanner
	if (HitActor->ActorHasTag(RepairOrderTag))
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		HitActor->GetComponents<UStaticMeshComponent>(MeshComponents);

		for (UStaticMeshComponent* MeshComp : MeshComponents)
		{
			if (MeshComp && MeshComp->ComponentHasTag(XRayTag))
			{
				ApplyXRayMaterial(MeshComp, Hit.ImpactPoint);
				return;
			}
		}

	}
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

	// Update sphere mask center to follow the scanner position
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

APlayerController* AXRayScanner::GetPlayerController() const
{
	return GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
}
