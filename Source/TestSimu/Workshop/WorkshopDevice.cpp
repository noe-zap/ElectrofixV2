#include "Workshop/WorkshopDevice.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

const FName AWorkshopDevice::BrokenTag = FName("Broken");
const FName AWorkshopDevice::ScrewTag = FName("Screw");
const FName AWorkshopDevice::ScrewDriverTag = FName("ScrewDriver");

AWorkshopDevice::AWorkshopDevice()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

// ============================================================
// Repair Entry / Exit
// ============================================================

void AWorkshopDevice::Repair(const TArray<FName>& BrokenPartIds, FVector SpawnLocation)
{
	SetActorLocation(SpawnLocation);

	ActiveBrokenPartIds = BrokenPartIds;
	SlotTransforms.Empty();
	SlotOccupants.Empty();
	ScrewToPartMap.Empty();
	PartScrewsMap.Empty();
	DraggedComponent = nullptr;
	ScrewDriverComp = nullptr;
	bHoldingScrewDriver = false;
	ActiveScrew = nullptr;
	bWasLeftMouseDown = false;
	bWasRightMouseDown = false;

	// Register broken part slots
	TArray<UStaticMeshComponent*> AllMeshes;
	GetComponents<UStaticMeshComponent>(AllMeshes);

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::Repair - Found %d meshes, checking %d broken IDs"), AllMeshes.Num(), BrokenPartIds.Num());

	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		if (Mesh->ComponentTags.Num() == 0)
		{
			continue;
		}

		FName FirstTag = Mesh->ComponentTags[0];
		if (BrokenPartIds.Contains(FirstTag))
		{
			SlotTransforms.Add(FirstTag, Mesh->GetComponentTransform());
			SlotOccupants.Add(FirstTag, Mesh);
			Mesh->ComponentTags.AddUnique(BrokenTag);
			Mesh->SetOverlayMaterial(BrokenMatOverlay);
			UE_LOG(LogTemp, Warning, TEXT("  Slot registered: %s (Mesh: %s)"), *FirstTag.ToString(), *Mesh->GetName());
		}
	}

	// Register screws via overlap detection
	RegisterScrews();

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::Repair - %d slots, %d screws registered"), SlotTransforms.Num(), ScrewToPartMap.Num());

	bIsRepairing = true;
	SetActorTickEnabled(true);
}

void AWorkshopDevice::RegisterScrews()
{
	// Collect broken part meshes and their bounds
	struct FPartBounds
	{
		FName PartId;
		FBox Bounds;
	};
	TArray<FPartBounds> PartBoundsList;

	TArray<UStaticMeshComponent*> AllMeshes;
	GetComponents<UStaticMeshComponent>(AllMeshes);

	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		if (Mesh->ComponentTags.Num() == 0)
		{
			continue;
		}
		FName FirstTag = Mesh->ComponentTags[0];
		if (SlotTransforms.Contains(FirstTag))
		{
			FPartBounds PB;
			PB.PartId = FirstTag;
			PB.Bounds = Mesh->Bounds.GetBox();
			PartBoundsList.Add(PB);
		}
	}

	// Find screws and check overlap with parts
	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		if (!Mesh->ComponentTags.Contains(ScrewTag))
		{
			continue;
		}

		FBox ScrewBounds = Mesh->Bounds.GetBox();

		for (const FPartBounds& PB : PartBoundsList)
		{
			if (ScrewBounds.Intersect(PB.Bounds))
			{
				ScrewToPartMap.Add(Mesh, PB.PartId);

				FScrewArray& ScrewArr = PartScrewsMap.FindOrAdd(PB.PartId);
				ScrewArr.Screws.Add(Mesh);

				UE_LOG(LogTemp, Warning, TEXT("  Screw '%s' assigned to part '%s'"), *Mesh->GetName(), *PB.PartId.ToString());
				break;
			}
		}
	}
}

void AWorkshopDevice::StopRepair()
{
	TArray<UStaticMeshComponent*> AllMeshes;
	GetComponents<UStaticMeshComponent>(AllMeshes);
	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		Mesh->ComponentTags.Remove(BrokenTag);
		Mesh->SetOverlayMaterial(nullptr);
		Mesh->SetSimulatePhysics(false);
	}

	if (bHoldingScrewDriver)
	{
		DropScrewDriver();
	}

	ActiveBrokenPartIds.Empty();
	SlotTransforms.Empty();
	SlotOccupants.Empty();
	ScrewToPartMap.Empty();
	PartScrewsMap.Empty();
	DraggedComponent = nullptr;
	ScrewDriverComp = nullptr;
	ActiveScrew = nullptr;
	bWasLeftMouseDown = false;
	bWasRightMouseDown = false;
	bIsRepairing = false;
	SetActorTickEnabled(false);
}

// ============================================================
// Tick
// ============================================================

void AWorkshopDevice::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsRepairing)
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	const bool bLeftMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	const bool bLeftMousePressed = bLeftMouseDown && !bWasLeftMouseDown;
	const bool bLeftMouseReleased = !bLeftMouseDown && bWasLeftMouseDown;
	bWasLeftMouseDown = bLeftMouseDown;

	const bool bRightMouseDown = PC->IsInputKeyDown(EKeys::RightMouseButton);
	const bool bRightMousePressed = bRightMouseDown && !bWasRightMouseDown;
	bWasRightMouseDown = bRightMouseDown;

	if (bHoldingScrewDriver)
	{
		// --- ScrewDriver Mode ---
		UpdateScrewDriverPosition(DeltaTime);

		if (ActiveScrew)
		{
			if (bLeftMouseDown)
			{
				UpdateUnscrew(DeltaTime);
			}
			else
			{
				// Released before timer finished
				CancelUnscrew();
			}
		}
		else if (bLeftMousePressed)
		{
			TryStartUnscrew();
		}

		if (bRightMousePressed)
		{
			if (ActiveScrew)
			{
				CancelUnscrew();
			}
			DropScrewDriver();
		}
	}
	else
	{
		// --- Part Drag Mode ---
		if (bLeftMousePressed && !DraggedComponent)
		{
			TryGrabPart();
		}

		if (bLeftMouseDown && DraggedComponent)
		{
			UpdateDrag(DeltaTime);
		}

		if (bLeftMouseReleased && DraggedComponent)
		{
			ReleasePart();
		}
	}
}

// ============================================================
// Part Drag
// ============================================================

void AWorkshopDevice::TryGrabPart()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	FVector TraceEnd = WorldLocation + WorldDirection * DragTraceDistance;

	if (bDebugTrace)
	{
		DrawDebugLine(GetWorld(), WorldLocation, TraceEnd, FColor::Green, false, 2.f, 0, 1.f);
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, Params);

	if (bDebugTrace && bHit)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 12, FColor::Red, false, 2.f);
		UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::TryGrabPart - Hit: %s, Component: %s"),
			*Hit.GetActor()->GetName(), *Hit.GetComponent()->GetName());
	}

	if (!bHit)
	{
		return;
	}

	UStaticMeshComponent* HitMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
	if (!HitMesh || HitMesh->ComponentTags.Num() == 0)
	{
		return;
	}

	// Check if clicking on the screwdriver
	if (HitMesh->ComponentTags.Contains(ScrewDriverTag))
	{
		ScrewDriverComp = HitMesh;
		ScrewDriverOriginalTransform = HitMesh->GetComponentTransform();
		TryPickUpScrewDriver();
		return;
	}

	FName PartId = HitMesh->ComponentTags[0];
	if (!ActiveBrokenPartIds.Contains(PartId))
	{
		if (bDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("  First tag '%s' not in ActiveBrokenPartIds"), *PartId.ToString());
		}
		return;
	}

	// Check if part is locked by screws
	if (IsPartLockedByScrews(PartId))
	{
		UE_LOG(LogTemp, Warning, TEXT("  Part '%s' is locked by screws - cannot drag"), *PartId.ToString());
		return;
	}

	DraggedComponent = HitMesh;

	// Clear slot if this mesh was occupying one
	if (TObjectPtr<UStaticMeshComponent>* Occupant = SlotOccupants.Find(PartId))
	{
		if (*Occupant == DraggedComponent)
		{
			*Occupant = nullptr;
		}
	}

	if (DraggedComponent->IsSimulatingPhysics())
	{
		DraggedComponent->SetSimulatePhysics(false);
	}

	DraggedComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	FVector LiftedPos = DraggedComponent->GetComponentLocation();
	if (FTransform* SlotTransform = SlotTransforms.Find(PartId))
	{
		LiftedPos.Z = SlotTransform->GetLocation().Z + DragLiftHeight;
	}
	DraggedComponent->SetWorldLocation(LiftedPos);
	DragTargetLocation = LiftedPos;

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::TryGrabPart - Grabbed: %s (PartId: %s)"),
		*DraggedComponent->GetName(), *PartId.ToString());
}

void AWorkshopDevice::UpdateDrag(float DeltaTime)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	float MouseX, MouseY;
	PC->GetInputMouseDelta(MouseX, MouseY);

	if (MouseX != 0.f || MouseY != 0.f)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		FVector CamForward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
		CamForward.Z = 0.f;
		CamForward.Normalize();

		DragTargetLocation += CamRight * MouseX * DragScale + CamForward * MouseY * DragScale;
	}

	FName PartId = GetPartId(DraggedComponent);
	if (FTransform* SlotTransform = SlotTransforms.Find(PartId))
	{
		DragTargetLocation.Z = SlotTransform->GetLocation().Z + DragLiftHeight;

		if (MaxDragDistance > 0.f)
		{
			FVector2D Target2D(DragTargetLocation.X, DragTargetLocation.Y);
			FVector2D Origin2D(SlotTransform->GetLocation().X, SlotTransform->GetLocation().Y);
			float Dist2D = FVector2D::Distance(Target2D, Origin2D);
			if (Dist2D > MaxDragDistance)
			{
				FVector2D Dir = (Target2D - Origin2D).GetSafeNormal();
				FVector2D Clamped = Origin2D + Dir * MaxDragDistance;
				DragTargetLocation.X = Clamped.X;
				DragTargetLocation.Y = Clamped.Y;
			}
		}
	}

	FVector CurrentLoc = DraggedComponent->GetComponentLocation();
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, DragTargetLocation, DeltaTime, DragInterpSpeed);
	DraggedComponent->SetWorldLocation(NewLoc);
}

void AWorkshopDevice::ReleasePart()
{
	FName PartId = GetPartId(DraggedComponent);

	FTransform* SlotTransform = SlotTransforms.Find(PartId);
	if (!SlotTransform)
	{
		DraggedComponent->SetSimulatePhysics(true);
		DraggedComponent = nullptr;
		return;
	}

	float Distance = FVector::Dist(DraggedComponent->GetComponentLocation(), SlotTransform->GetLocation());

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::ReleasePart - %s (PartId: %s), Distance: %.1f, Threshold: %.1f"),
		*DraggedComponent->GetName(), *PartId.ToString(), Distance, SnapDistanceThreshold);

	TObjectPtr<UStaticMeshComponent>* CurrentOccupant = SlotOccupants.Find(PartId);
	bool bSlotFree = !CurrentOccupant || !(*CurrentOccupant);

	if (Distance <= SnapDistanceThreshold && bSlotFree)
	{
		SnapToSlot(DraggedComponent, PartId);
	}
	else
	{
		DraggedComponent->SetSimulatePhysics(true);
	}

	DraggedComponent = nullptr;
}

void AWorkshopDevice::SnapToSlot(UStaticMeshComponent* Comp, FName PartId)
{
	FTransform SlotTransformValue = SlotTransforms[PartId];
	Comp->SetSimulatePhysics(false);
	Comp->SetWorldTransform(SlotTransformValue);
	Comp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	SlotOccupants[PartId] = Comp;

	OnPartSnappedBack.Broadcast(PartId);

	bool bSlotRepaired = IsSlotRepaired(PartId);
	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::SnapToSlot - %s snapped to slot %s (Repaired: %s)"),
		*Comp->GetName(), *PartId.ToString(), bSlotRepaired ? TEXT("YES") : TEXT("NO"));

	if (AreAllSlotsRepaired())
	{
		UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice - All slots repaired!"));
		OnAllPartsRepaired.Broadcast();
	}
}

bool AWorkshopDevice::IsPartLockedByScrews(FName PartId) const
{
	const FScrewArray* ScrewArr = PartScrewsMap.Find(PartId);
	return ScrewArr && ScrewArr->Screws.Num() > 0;
}

bool AWorkshopDevice::IsSlotRepaired(FName PartId) const
{
	const TObjectPtr<UStaticMeshComponent>* Occupant = SlotOccupants.Find(PartId);
	if (!Occupant || !*Occupant)
	{
		return false;
	}
	return !(*Occupant)->ComponentTags.Contains(BrokenTag);
}

bool AWorkshopDevice::AreAllSlotsRepaired() const
{
	for (const auto& Pair : SlotOccupants)
	{
		if (!IsSlotRepaired(Pair.Key))
		{
			return false;
		}
	}
	return true;
}

FName AWorkshopDevice::GetPartId(UStaticMeshComponent* Comp) const
{
	if (Comp && Comp->ComponentTags.Num() > 0)
	{
		return Comp->ComponentTags[0];
	}
	return NAME_None;
}

// ============================================================
// ScrewDriver
// ============================================================

void AWorkshopDevice::TryPickUpScrewDriver()
{
	if (!ScrewDriverComp)
	{
		return;
	}

	bHoldingScrewDriver = true;
	ScrewDriverComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	ScrewDriverTargetLocation = ScrewDriverComp->GetComponentLocation();

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice - ScrewDriver picked up"));
}

void AWorkshopDevice::DropScrewDriver()
{
	if (!ScrewDriverComp)
	{
		return;
	}

	bHoldingScrewDriver = false;

	// Return screwdriver to original position
	ScrewDriverComp->SetWorldTransform(ScrewDriverOriginalTransform);

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice - ScrewDriver dropped"));
}

void AWorkshopDevice::UpdateScrewDriverPosition(float DeltaTime)
{
	if (!ScrewDriverComp || ActiveScrew)
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	float MouseX, MouseY;
	PC->GetInputMouseDelta(MouseX, MouseY);

	if (MouseX != 0.f || MouseY != 0.f)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		FVector CamForward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
		CamForward.Z = 0.f;
		CamForward.Normalize();

		ScrewDriverTargetLocation += CamRight * MouseX * DragScale + CamForward * MouseY * DragScale;
	}

	FVector CurrentLoc = ScrewDriverComp->GetComponentLocation();
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, ScrewDriverTargetLocation, DeltaTime, DragInterpSpeed);
	ScrewDriverComp->SetWorldLocation(NewLoc);
}

// ============================================================
// Screw Unscrewing
// ============================================================

void AWorkshopDevice::TryStartUnscrew()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	FVector TraceEnd = WorldLocation + WorldDirection * DragTraceDistance;

	if (bDebugTrace)
	{
		DrawDebugLine(GetWorld(), WorldLocation, TraceEnd, FColor::Cyan, false, 2.f, 0, 1.f);
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;

	if (!GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		return;
	}

	UStaticMeshComponent* HitMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
	if (!HitMesh || !ScrewToPartMap.Contains(HitMesh))
	{
		if (bDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::TryStartUnscrew - Hit is not a registered screw"));
		}
		return;
	}

	ActiveScrew = HitMesh;
	UnscrewTimer = 0.f;
	ScrewStartLocation = ActiveScrew->GetComponentLocation();
	ScrewStartRotation = ActiveScrew->GetComponentRotation();

	// Move screwdriver to screw position
	if (ScrewDriverComp)
	{
		ScrewDriverTargetLocation = ScrewStartLocation;
		ScrewDriverComp->SetWorldLocation(ScrewStartLocation);
	}

	FName* PartId = ScrewToPartMap.Find(ActiveScrew);
	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::TryStartUnscrew - Unscrewing '%s' from part '%s'"),
		*ActiveScrew->GetName(), PartId ? *PartId->ToString() : TEXT("unknown"));
}

void AWorkshopDevice::UpdateUnscrew(float DeltaTime)
{
	if (!ActiveScrew)
	{
		return;
	}

	UnscrewTimer += DeltaTime;
	float Alpha = FMath::Clamp(UnscrewTimer / UnscrewDuration, 0.f, 1.f);

	// Rotate screw around local Z
	float RotationAngle = UnscrewTimer * ScrewRotationSpeed;
	FRotator NewRotation = ScrewStartRotation;
	NewRotation.Yaw += RotationAngle;

	// Lift screw upward
	FVector NewLocation = ScrewStartLocation;
	NewLocation.Z += Alpha * ScrewLiftDistance;

	ActiveScrew->SetWorldLocationAndRotation(NewLocation, NewRotation);

	// Rotate screwdriver too
	if (ScrewDriverComp)
	{
		FRotator DriverRot = ScrewDriverComp->GetComponentRotation();
		DriverRot.Yaw = ScrewStartRotation.Yaw + RotationAngle;
		ScrewDriverComp->SetWorldLocationAndRotation(NewLocation, DriverRot);
	}

	if (UnscrewTimer >= UnscrewDuration)
	{
		FinishUnscrew();
	}
}

void AWorkshopDevice::FinishUnscrew()
{
	if (!ActiveScrew)
	{
		return;
	}

	FName* PartIdPtr = ScrewToPartMap.Find(ActiveScrew);
	FName PartId = PartIdPtr ? *PartIdPtr : NAME_None;

	// Detach and eject screw
	ActiveScrew->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	ActiveScrew->SetSimulatePhysics(true);

	FVector EjectDir = FVector::UpVector;
	ActiveScrew->AddImpulse(EjectDir * ScrewEjectImpulse, NAME_None, true);

	// Remove screw from maps
	ScrewToPartMap.Remove(ActiveScrew);
	if (FScrewArray* ScrewArr = PartScrewsMap.Find(PartId))
	{
		ScrewArr->Screws.Remove(ActiveScrew);
		UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::FinishUnscrew - Screw removed from part '%s' (%d screws remaining)"),
			*PartId.ToString(), ScrewArr->Screws.Num());
	}

	OnScrewRemoved.Broadcast(PartId);

	// Move screwdriver back to free movement
	if (ScrewDriverComp)
	{
		ScrewDriverTargetLocation = ScrewDriverComp->GetComponentLocation();
	}

	ActiveScrew = nullptr;
	UnscrewTimer = 0.f;
}

void AWorkshopDevice::CancelUnscrew()
{
	if (!ActiveScrew)
	{
		return;
	}

	// Reset screw to original position
	ActiveScrew->SetWorldLocationAndRotation(ScrewStartLocation, ScrewStartRotation);

	// Move screwdriver back to free movement
	if (ScrewDriverComp)
	{
		ScrewDriverTargetLocation = ScrewDriverComp->GetComponentLocation();
	}

	ActiveScrew = nullptr;
	UnscrewTimer = 0.f;

	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::CancelUnscrew - Unscrew cancelled"));
}
