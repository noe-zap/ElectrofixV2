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
}

void AXRayScanner::ActivateScanner(FVector Location, FRotator Rotation)
{
	bIsScanning = true;
	TargetLocation = Location;
	SetActorLocation(Location);
	SetActorRotation(Rotation);
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);

	// Store initial mouse position so first tick doesn't cause a jump
	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		float MouseX, MouseY;
		PC->GetMousePosition(MouseX, MouseY);
		LastMousePosition = FVector2D(MouseX, MouseY);
	}

	UE_LOG(LogTemp, Log, TEXT("XRayScanner: Activated at (%.1f, %.1f, %.1f)"), Location.X, Location.Y, Location.Z);
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

	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D CurrentMouse(MouseX, MouseY);
	const FVector2D Delta = CurrentMouse - LastMousePosition;
	LastMousePosition = CurrentMouse;

	// Get camera right/up vectors to map screen movement to world movement
	APlayerCameraManager* CamManager = PC->PlayerCameraManager;
	if (!CamManager)
	{
		return;
	}

	const FRotator CamRotation = CamManager->GetCameraRotation();
	const FVector CamRight = FRotationMatrix(CamRotation).GetUnitAxis(EAxis::Y);
	const FVector CamUp = FRotationMatrix(CamRotation).GetUnitAxis(EAxis::Z);

	// Project camera axes onto the horizontal plane (only X/Y, no Z)
	FVector Right2D = FVector(CamRight.X, CamRight.Y, 0.f).GetSafeNormal();
	FVector Up2D = FVector(CamUp.X, CamUp.Y, 0.f).GetSafeNormal();

	// Update target position based on mouse delta (screen Y is inverted relative to world up)
	TargetLocation += (Right2D * Delta.X - Up2D * Delta.Y) * MouseSensitivity;

	// Clamp target to screen bounds
	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);

	FVector2D ScreenPos;
	if (PC->ProjectWorldLocationToScreen(TargetLocation, ScreenPos))
	{
		if (ScreenPos.X < 0.f || ScreenPos.X > ViewportX ||
			ScreenPos.Y < 0.f || ScreenPos.Y > ViewportY)
		{
			// Reject the move — revert target
			TargetLocation -= (Right2D * Delta.X - Up2D * Delta.Y) * MouseSensitivity;
		}
	}

	// Smoothly interpolate actual position toward target (lag/sway effect)
	const FVector CurrentLocation = GetActorLocation();
	const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, MoveInterpSpeed);
	SetActorLocation(NewLocation);
}

void AXRayScanner::PerformScanTrace()
{
	// Start the trace slightly behind the actor so it doesn't begin inside geometry
	const FVector Forward = GetActorForwardVector();
	const FVector TraceStart = GetActorLocation() - Forward * 5.f;
	const FVector TraceEnd = TraceStart + Forward * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		UE_LOG(LogTemp, Verbose, TEXT("XRayScanner: Scan trace hit nothing"));
		return;
	}

	AActor* HitActor = Hit.GetActor();
	UE_LOG(LogTemp, Log, TEXT("XRayScanner: Scan trace hit Actor='%s' Component='%s'"),
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

	// Hit a RepairOrder actor — find first static mesh component with tag XRayScanner
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
