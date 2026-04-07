#include "XRayScanner.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

AXRayScanner::AXRayScanner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TraceOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TraceOrigin"));
	TraceOrigin->SetupAttachment(RootComponent);
}

void AXRayScanner::ActivateScanner(FVector Location, FRotator Rotation, const TArray<FName>& InBrokenPartIds)
{
	SetBrokenPartIds(InBrokenPartIds);
	bIsScanning = true;
	TargetLocation = Location;
	ActivationOrigin = Location;
	SetActorLocation(Location);
	SetActorRotation(Rotation);
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
}

void AXRayScanner::SetBrokenPartIds(const TArray<FName>& InBrokenPartIds)
{
	BrokenPartIds = InBrokenPartIds;
	FoundBrokenParts.Empty();

	FString IdsStr;
	for (const FName& Id : BrokenPartIds)
	{
		IdsStr += Id.ToString() + TEXT(", ");
	}
	UE_LOG(LogTemp, Log, TEXT("XRayScanner: SetBrokenPartIds called with %d parts: [%s]"), BrokenPartIds.Num(), *IdsStr);
}

void AXRayScanner::DeactivateScanner()
{
	if (!bIsScanning)
	{
		return;
	}

	bIsScanning = false;
	FoundBrokenParts.Empty();
	GetWorldTimerManager().ClearTimer(AllPartsFoundEventTimerHandle);
	GetWorldTimerManager().ClearTimer(AllPartsFoundHideTimerHandle);
	RestoreAllMaterials();
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
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

	NewLocation.X = FMath::Clamp(NewLocation.X, ClampedMin.X, ClampedMax.X);
	NewLocation.Y = FMath::Clamp(NewLocation.Y, ClampedMin.Y, ClampedMax.Y);
	NewLocation.Z = FMath::Clamp(NewLocation.Z, ClampedMin.Z, ClampedMax.Z);
	SetActorLocation(NewLocation);
}

void AXRayScanner::PerformScanTrace()
{
	const FVector TraceStart = TraceOrigin->GetComponentLocation();
	const FVector TraceEnd = TraceStart + FVector(0.f, 0.f, -1.f) * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		UE_LOG(LogTemp, Verbose, TEXT("XRayScanner: Line trace missed (no hit)"));
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRayScanner: Hit something but no valid actor"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("XRayScanner: Hit Actor: %s | Component: %s | Distance: %.1f"),
		*HitActor->GetName(),
		Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("None"),
		Hit.Distance);

	if (HitActor->ActorHasTag(XRayTag))
	{
		UE_LOG(LogTemp, Log, TEXT("XRayScanner: Actor has XRay tag"));
		UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Hit.GetComponent());
		if (MeshComp)
		{
			ApplyXRayMaterial(MeshComp, Hit.ImpactPoint);
			CheckForBrokenPart(MeshComp);
			return;
		}
	}

	if (HitActor->ActorHasTag(RepairOrderTag))
	{
		UE_LOG(LogTemp, Log, TEXT("XRayScanner: Actor has RepairOrder tag"));
		TArray<UStaticMeshComponent*> MeshComponents;
		HitActor->GetComponents<UStaticMeshComponent>(MeshComponents);

		const FVector HitPoint = Hit.ImpactPoint;

		int32 FoundCount = 0;
		for (UStaticMeshComponent* MeshComp : MeshComponents)
		{
			if (MeshComp && MeshComp->ComponentHasTag(XRayTag))
			{
				UE_LOG(LogTemp, Log, TEXT("XRayScanner: Found XRay-tagged mesh component: %s"), *MeshComp->GetName());
				ApplyXRayMaterial(MeshComp, HitPoint);
				FoundCount++;
			}

			// Check if hit point XY is inside this mesh's XY bounds for broken part detection
			// (ignore Z since trace hits the top surface but parts are underneath)
			if (MeshComp && BrokenPartIds.Num() > 0)
			{
				const FBoxSphereBounds MeshBounds = MeshComp->Bounds;
				const FVector MeshMin = MeshBounds.Origin - MeshBounds.BoxExtent;
				const FVector MeshMax = MeshBounds.Origin + MeshBounds.BoxExtent;

				const bool bInsideXY = HitPoint.X >= MeshMin.X && HitPoint.X <= MeshMax.X
					&& HitPoint.Y >= MeshMin.Y && HitPoint.Y <= MeshMax.Y;

				if (bInsideXY)
				{
					UE_LOG(LogTemp, Log, TEXT("XRayScanner: Hit point is over mesh: %s"), *MeshComp->GetName());
					CheckForBrokenPart(MeshComp);
				}
			}
		}
		if (FoundCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("XRayScanner: RepairOrder actor had no mesh components with XRay tag"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("XRayScanner: Applied XRay to %d mesh components"), FoundCount);
		}
	}
}

void AXRayScanner::ApplyXRayMaterial(UStaticMeshComponent* MeshComp, const FVector& HitLocation)
{
	if (!XRayMaterial)
	{
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

		// Grab BaseColorTexture from the original material and pass it to the XRay material
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UTexture* Tex = nullptr;
			if (OriginalMats.Materials[i] && OriginalMats.Materials[i]->GetTextureParameterValue(BaseColorTextureParamName, Tex) && Tex)
			{
				DynMat->SetTextureParameterValue(BaseColorTextureParamName, Tex);
				break;
			}
		}

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

void AXRayScanner::CheckForBrokenPart(UStaticMeshComponent* MeshComp)
{
	if (BrokenPartIds.Num() == 0 || !MeshComp)
	{
		return;
	}

	for (const FName& Tag : MeshComp->ComponentTags)
	{
		if (BrokenPartIds.Contains(Tag) && !FoundBrokenParts.Contains(Tag))
		{
			FoundBrokenParts.Add(Tag);
			UE_LOG(LogTemp, Log, TEXT("XRayScanner: Broken part found: %s (%d/%d)"),
				*Tag.ToString(), FoundBrokenParts.Num(), BrokenPartIds.Num());
			OnBrokenPartFound.Broadcast(Tag, FoundBrokenParts.Num());

			if (FoundBrokenParts.Num() >= BrokenPartIds.Num())
			{
				UE_LOG(LogTemp, Log, TEXT("XRayScanner: All broken parts scanned!"));
				bIsScanning = false;
				SetActorTickEnabled(false);
				GetWorldTimerManager().SetTimer(AllPartsFoundEventTimerHandle, this,
					&AXRayScanner::OnAllPartsFoundEventDelay, 1.f, false);
				GetWorldTimerManager().SetTimer(AllPartsFoundHideTimerHandle, this,
					&AXRayScanner::OnAllPartsFoundHideDelay, 2.f, false);
				return;
			}
		}
	}
}

void AXRayScanner::OnAllPartsFoundEventDelay()
{
	OnAllBrokenPartsFound.Broadcast();

	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

void AXRayScanner::OnAllPartsFoundHideDelay()
{
	RestoreAllMaterials();
	FoundBrokenParts.Empty();
	SetActorHiddenInGame(true);
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
