#include "Workshop/WorkshopDevice.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

const FString AWorkshopDevice::BrokenSuffix = TEXT("_broken");
const FName AWorkshopDevice::ScrewTag = FName("Screw");
const FName AWorkshopDevice::ScrewDriverTag = FName("ScrewDriver");
const FName AWorkshopDevice::CoverToolTag = FName("CoverTool");
const FName AWorkshopDevice::CoverTag = FName("Cover");
const FName AWorkshopDevice::CoverToolPosTag = FName("CoverToolPos");
const FName AWorkshopDevice::CoverPosTag = FName("CoverPos");

void AWorkshopDevice::SetupWorkshopCollision(UStaticMeshComponent* Comp)
{
	if (!Comp) return;
	Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
	Comp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Comp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Comp->SetCollisionResponseToChannel(WorkshopChannel, ECR_Block);
}

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
	CoverToolComp = nullptr;
	CoverComp = nullptr;
	CoverToolPosComp = nullptr;
	CoverPosComp = nullptr;
	CoverState = ECoverRemovalState::Inactive;
	CoverToolMoveAlpha = 0.f;
	CoverPullAccumulator = 0.f;
	CoverAnimAlpha = 0.f;

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
		UE_LOG(LogTemp, Warning, TEXT("  Mesh: %s | FirstTag: %s"), *Mesh->GetName(), *FirstTag.ToString());

		if (BrokenPartIds.Contains(FirstTag))
		{
			// Store slot transform keyed by original PartId
			SlotTransforms.Add(FirstTag, Mesh->GetComponentTransform());
			SlotOccupants.Add(FirstTag, Mesh);

			// Rename tag: "HDD" -> "HDD_broken"
			FName BrokenTag = FName(FirstTag.ToString() + BrokenSuffix);
			Mesh->ComponentTags[0] = BrokenTag;

			Mesh->SetOverlayMaterial(BrokenMatOverlay);
			SetupWorkshopCollision(Mesh);
			UE_LOG(LogTemp, Warning, TEXT("  -> Slot registered: %s (renamed to %s)"), *FirstTag.ToString(), *BrokenTag.ToString());
		}

		// Setup collision for screws and screwdriver
		if (Mesh->ComponentTags.Contains(ScrewTag) || Mesh->ComponentTags.Contains(ScrewDriverTag))
		{
			SetupWorkshopCollision(Mesh);
			UE_LOG(LogTemp, Warning, TEXT("  -> Screw/ScrewDriver collision setup: %s"), *Mesh->GetName());
		}
	}

	RegisterScrews();
	UE_LOG(LogTemp, Warning, TEXT("WorkshopDevice::Repair - %d slots, %d screws registered"), SlotTransforms.Num(), ScrewToPartMap.Num());

	if (bHasCover)
	{
		InitCoverPhase();
	}

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
		// Tags have already been renamed to X_broken, so use GetBasePartId
		FName BaseId = GetBasePartId(Mesh);
		if (SlotTransforms.Contains(BaseId))
		{
			FPartBounds PB;
			PB.PartId = BaseId;
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
				break;
			}
		}
	}
}

UStaticMeshComponent* AWorkshopDevice::SpawnNewPart(UStaticMesh* Mesh, FName BrokenPartId)
{
	if (!Mesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* NewComp = NewObject<UStaticMeshComponent>(this);
	NewComp->SetStaticMesh(Mesh);
	NewComp->ComponentTags.Add(BrokenPartId);
	NewComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	NewComp->RegisterComponent();

	SetupWorkshopCollision(NewComp);

	// Match scale from the broken part's slot
	FTransform* SlotTransform = SlotTransforms.Find(BrokenPartId);
	FVector SpawnLoc = GetActorLocation() + GetActorRotation().RotateVector(SpawnOffset);
	SpawnLoc.Z += SpawnHeightOffset;

	NewComp->SetWorldLocation(SpawnLoc);
	if (SlotTransform)
	{
		NewComp->SetWorldScale3D(SlotTransform->GetScale3D());
	}

	NewComp->SetSimulatePhysics(true);

	return NewComp;
}

void AWorkshopDevice::StopRepair()
{
	// Reset cover to original transforms if mid-phase
	if (CoverState != ECoverRemovalState::Inactive && CoverState != ECoverRemovalState::Done)
	{
		if (CoverToolComp) CoverToolComp->SetWorldTransform(CoverToolOriginalTransform);
		if (CoverComp) CoverComp->SetWorldTransform(CoverOriginalTransform);
	}
	CoverState = ECoverRemovalState::Inactive;
	CoverToolComp = nullptr;
	CoverComp = nullptr;
	CoverToolPosComp = nullptr;
	CoverPosComp = nullptr;
	CoverToolMoveAlpha = 0.f;
	CoverPullAccumulator = 0.f;
	CoverAnimAlpha = 0.f;

	if (bHoldingScrewDriver)
	{
		ReleaseScrewDriver();
	}
	if (ActiveScrew)
	{
		CancelUnscrew();
	}

	TArray<UStaticMeshComponent*> AllMeshes;
	GetComponents<UStaticMeshComponent>(AllMeshes);
	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		Mesh->SetOverlayMaterial(nullptr);
		Mesh->SetSimulatePhysics(false);
	}

	ActiveBrokenPartIds.Empty();
	SlotTransforms.Empty();
	SlotOccupants.Empty();
	ScrewToPartMap.Empty();
	PartScrewsMap.Empty();
	DraggedComponent = nullptr;
	ScrewDriverComp = nullptr;
	ActiveScrew = nullptr;
	if (HoveredComponent)
	{
		HoveredComponent->SetRenderCustomDepth(false);
		HoveredComponent = nullptr;
	}
	bWasLeftMouseDown = false;
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

	// --- Cover Phase (runs before everything else) ---
	if (CoverState != ECoverRemovalState::Inactive && CoverState != ECoverRemovalState::Done)
	{
		UpdateHover();
		UpdateCoverPhase(DeltaTime);
		return;
	}

	UpdateHover();

	const bool bLeftMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	const bool bLeftMousePressed = bLeftMouseDown && !bWasLeftMouseDown;
	const bool bLeftMouseReleased = !bLeftMouseDown && bWasLeftMouseDown;
	bWasLeftMouseDown = bLeftMouseDown;

	if (bHoldingScrewDriver)
	{
		// --- ScrewDriver Mode ---
		if (ActiveScrew)
		{
			// Currently unscrewing
			if (bLeftMouseDown)
			{
				UpdateUnscrew(DeltaTime);
			}
			else
			{
				// Released — cancel unscrew
				CancelUnscrew();
			}
		}
		else
		{
			// Move screwdriver and snap to nearby screws
			UpdateScrewDriver(DeltaTime);

			if (bLeftMousePressed)
			{
				// Check if snapped to a screw
				UStaticMeshComponent* NearestScrew = FindNearestScrew();
				if (NearestScrew)
				{
					// Start unscrewing
					StartUnscrew(NearestScrew);
				}
				else
				{
					// Not near a screw — drop screwdriver
					ReleaseScrewDriver();
				}
			}
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
// Hover
// ============================================================

void AWorkshopDevice::UpdateHover()
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
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;

	UStaticMeshComponent* NewHovered = nullptr;

	if (GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, WorkshopChannel, Params))
	{
		UStaticMeshComponent* HitMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
		if (HitMesh && HitMesh->ComponentTags.Num() > 0)
		{
			bool bCanInteract = false;

			if (CoverState == ECoverRemovalState::ToolAtBase)
			{
				// During cover phase, only meshes with CoverTool tag are interactable
				bCanInteract = HitMesh->ComponentTags.Contains(CoverToolTag);
			}
			else if (CoverState != ECoverRemovalState::Inactive && CoverState != ECoverRemovalState::Done)
			{
				// During other cover states, nothing is hoverable
				bCanInteract = false;
			}
			else if (bHoldingScrewDriver)
			{
				// While holding screwdriver, only screws are interactable
				bCanInteract = ScrewToPartMap.Contains(HitMesh);
			}
			else
			{
				// Not holding screwdriver: screwdriver, unlocked parts, and new parts
				if (HitMesh->ComponentTags.Contains(ScrewDriverTag))
				{
					bCanInteract = true;
				}
				else
				{
					FName BaseId = GetBasePartId(HitMesh);
					if (ActiveBrokenPartIds.Contains(BaseId) && !IsPartLockedByScrews(BaseId))
					{
						bCanInteract = true;
					}
				}
			}

			if (bCanInteract)
			{
				NewHovered = HitMesh;
			}
		}
	}

	if (NewHovered != HoveredComponent)
	{
		if (HoveredComponent)
		{
			HoveredComponent->SetRenderCustomDepth(false);
		}

		if (NewHovered)
		{
			NewHovered->SetRenderCustomDepth(true);
		}

		HoveredComponent = NewHovered;
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

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, WorkshopChannel, Params);

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - Line trace hit NOTHING on WorkshopChannel"));
		return;
	}

	if (bDebugTrace)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 12, FColor::Red, false, 2.f);
	}

	UStaticMeshComponent* HitMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
	if (!HitMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - Hit component is not a StaticMeshComponent: %s"), *Hit.GetComponent()->GetName());
		return;
	}
	if (HitMesh->ComponentTags.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - Hit mesh '%s' has NO tags"), *HitMesh->GetName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - Hit: %s | Tags[0]: %s | CollisionEnabled: %d"),
		*HitMesh->GetName(), *HitMesh->ComponentTags[0].ToString(), (int32)HitMesh->GetCollisionEnabled());

	// Check if clicking on the screwdriver
	if (HitMesh->ComponentTags.Contains(ScrewDriverTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - Clicked ScrewDriver"));
		ScrewDriverComp = HitMesh;
		ScrewDriverOriginalTransform = HitMesh->GetComponentTransform();
		if (ScrewDriverComp->IsSimulatingPhysics())
		{
			ScrewDriverComp->SetSimulatePhysics(false);
		}
		ScrewDriverComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		FVector LiftedPos = ScrewDriverComp->GetComponentLocation();
		LiftedPos.Z += ScrewDriverPickupLift;
		ScrewDriverComp->SetWorldLocation(LiftedPos);
		ScrewDriverDragTarget = LiftedPos;
		bHoldingScrewDriver = true;

		// Rotate 90° on pitch to point downward (ready to unscrew)
		FRotator ReadyRotation = ScrewDriverComp->GetComponentRotation();
		ReadyRotation.Pitch -= 90.f;
		ScrewDriverComp->SetWorldRotation(ReadyRotation);

		UE_LOG(LogTemp, Warning, TEXT("TryGrabScrewDriver - PICKED UP: %s"), *ScrewDriverComp->GetName());
		return;
	}

	// Get base part id (strips _broken suffix if present)
	FName PartId = GetBasePartId(HitMesh);
	if (!ActiveBrokenPartIds.Contains(PartId))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - PartId '%s' not in ActiveBrokenPartIds"), *PartId.ToString());
		return;
	}

	// Can't grab parts that still have screws
	if (IsPartLockedByScrews(PartId))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - Part '%s' is locked by screws"), *PartId.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("TryGrabPart - GRABBED: %s (PartId: %s)"), *HitMesh->GetName(), *PartId.ToString());
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

	FName PartId = GetBasePartId(DraggedComponent);
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
	FName PartId = GetBasePartId(DraggedComponent);

	FTransform* SlotTransform = SlotTransforms.Find(PartId);
	if (!SlotTransform)
	{
		DraggedComponent->SetSimulatePhysics(true);
		DraggedComponent = nullptr;
		return;
	}

	float Distance = FVector::Dist(DraggedComponent->GetComponentLocation(), SlotTransform->GetLocation());

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

	if (AreAllSlotsRepaired())
	{
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
	if ((*Occupant)->ComponentTags.Num() == 0)
	{
		return false;
	}
	return !IsBrokenTag((*Occupant)->ComponentTags[0]);
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

FName AWorkshopDevice::GetBasePartId(UStaticMeshComponent* Comp) const
{
	if (!Comp || Comp->ComponentTags.Num() == 0)
	{
		return NAME_None;
	}

	FString TagStr = Comp->ComponentTags[0].ToString();
	if (TagStr.EndsWith(BrokenSuffix))
	{
		TagStr.LeftChopInline(BrokenSuffix.Len());
	}
	return FName(*TagStr);
}

bool AWorkshopDevice::IsBrokenTag(const FName& Tag) const
{
	return Tag.ToString().EndsWith(BrokenSuffix);
}

// ============================================================
// Cover Pull
// ============================================================

void AWorkshopDevice::InitCoverPhase()
{
	// Cover, CoverToolPos, CoverPos are child components of the WorkshopDevice
	TArray<UStaticMeshComponent*> AllMeshes;
	GetComponents<UStaticMeshComponent>(AllMeshes);
	for (UStaticMeshComponent* Mesh : AllMeshes)
	{
		if (Mesh->ComponentTags.Contains(CoverTag))
		{
			CoverComp = Mesh;
		}
	}

	TArray<USceneComponent*> AllSceneComps;
	GetComponents<USceneComponent>(AllSceneComps);
	for (USceneComponent* SC : AllSceneComps)
	{
		if (SC->ComponentTags.Contains(CoverToolPosTag))
		{
			CoverToolPosComp = SC;
		}
		else if (SC->ComponentTags.Contains(CoverPosTag))
		{
			CoverPosComp = SC;
		}
	}

	// CoverTool is a separate actor (like the ScrewDriver) — found via raycast on click
	if (CoverComp && CoverToolPosComp && CoverPosComp)
	{
		CoverOriginalTransform = CoverComp->GetComponentTransform();
		CoverState = ECoverRemovalState::ToolAtBase;
		UE_LOG(LogTemp, Warning, TEXT("CoverPhase - Initialized. Waiting for tool click. (CoverTool found via raycast)"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CoverPhase - Missing child components. Cover:%d ToolPos:%d CoverPos:%d"),
			CoverComp != nullptr, CoverToolPosComp != nullptr, CoverPosComp != nullptr);
		CoverState = ECoverRemovalState::Done;
	}
}

void AWorkshopDevice::UpdateCoverPhase(float DeltaTime)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	const bool bLeftMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	const bool bLeftMousePressed = bLeftMouseDown && !bWasLeftMouseDown;
	bWasLeftMouseDown = bLeftMouseDown;

	switch (CoverState)
	{
	case ECoverRemovalState::ToolAtBase:
		if (bLeftMousePressed)
		{
			FVector WorldLocation, WorldDirection;
			if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
			{
				FVector TraceEnd = WorldLocation + WorldDirection * DragTraceDistance;
				FHitResult Hit;
				FCollisionQueryParams Params;
				Params.bTraceComplex = true;

				if (GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, WorkshopChannel, Params))
				{
					UStaticMeshComponent* HitMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
					if (HitMesh && HitMesh->ComponentTags.Contains(CoverToolTag))
					{
						// Found the CoverTool via raycast (separate actor, like ScrewDriver)
						CoverToolComp = HitMesh;
						CoverToolOriginalTransform = CoverToolComp->GetComponentTransform();
						CoverToolMoveAlpha = 0.f;
						CoverState = ECoverRemovalState::ToolMovingToSnap;
						UE_LOG(LogTemp, Warning, TEXT("CoverPhase - Tool clicked, moving to snap position."));
					}
				}
			}
		}
		break;

	case ECoverRemovalState::ToolMovingToSnap:
		UpdateCoverToolMoving(DeltaTime);
		break;

	case ECoverRemovalState::Pulling:
		UpdateCoverPulling(DeltaTime);
		break;

	case ECoverRemovalState::Animating:
		UpdateCoverAnimating(DeltaTime);
		break;

	default:
		break;
	}
}

void AWorkshopDevice::UpdateCoverToolMoving(float DeltaTime)
{
	CoverToolMoveAlpha = FMath::Clamp(CoverToolMoveAlpha + DeltaTime * CoverToolMoveSpeed, 0.f, 1.f);

	// Smoothstep: t^2 * (3 - 2t)
	float T = CoverToolMoveAlpha;
	float Smooth = T * T * (3.f - 2.f * T);

	FVector Start = CoverToolOriginalTransform.GetLocation();
	FVector End = CoverToolPosComp->GetComponentLocation();
	FVector Pos = FMath::Lerp(Start, End, Smooth);
	Pos.Z += FMath::Sin(Smooth * PI) * CoverToolArcHeight;

	CoverToolComp->SetWorldLocation(Pos);

	FQuat StartQuat = CoverToolOriginalTransform.GetRotation();
	FQuat EndQuat = CoverToolPosComp->GetComponentQuat();
	CoverToolComp->SetWorldRotation(FQuat::Slerp(StartQuat, EndQuat, Smooth));

	if (CoverToolMoveAlpha >= 1.f)
	{
		CoverToolComp->SetWorldLocation(End);
		CoverToolComp->SetWorldRotation(EndQuat.Rotator());
		CoverPullAccumulator = 0.f;
		CoverState = ECoverRemovalState::Pulling;
		UE_LOG(LogTemp, Warning, TEXT("CoverPhase - Tool snapped. Begin pulling."));
	}
}

void AWorkshopDevice::UpdateCoverPulling(float DeltaTime)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	const bool bLeftMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);

	if (bLeftMouseDown)
	{
		float MouseX, MouseY;
		PC->GetInputMouseDelta(MouseX, MouseY);

		// Negative MouseY in UE = mouse moving down on screen = pulling
		CoverPullAccumulator = FMath::Max(0.f, CoverPullAccumulator - MouseY);

		float RotAngle = CoverPullAccumulator * CoverPullRotationScale;

		// Rotate cover tool on Pitch
		FRotator ToolRot = CoverToolPosComp->GetComponentRotation();
		ToolRot.Pitch -= RotAngle;
		CoverToolComp->SetWorldRotation(ToolRot);
		CoverToolComp->SetWorldLocation(CoverToolPosComp->GetComponentLocation());

		// Rotate cover on Pitch (half rate for subtlety)
		FRotator CoverRot = CoverOriginalTransform.GetRotation().Rotator();
		CoverRot.Pitch += RotAngle * 0.5f;
		CoverComp->SetWorldRotation(CoverRot);

		if (CoverPullAccumulator >= CoverPullThreshold)
		{
			// Save current transforms for animation start
			CoverAnimStartPos = CoverComp->GetComponentLocation();
			CoverAnimStartQuat = CoverComp->GetComponentQuat();
			ToolAnimStartPos = CoverToolComp->GetComponentLocation();
			ToolAnimStartQuat = CoverToolComp->GetComponentQuat();
			CoverAnimAlpha = 0.f;
			CoverState = ECoverRemovalState::Animating;
			UE_LOG(LogTemp, Warning, TEXT("CoverPhase - Pull threshold reached. Animating removal."));
		}
	}
}

void AWorkshopDevice::UpdateCoverAnimating(float DeltaTime)
{
	CoverAnimAlpha = FMath::Clamp(CoverAnimAlpha + DeltaTime * CoverAnimSpeed, 0.f, 1.f);

	// Smoothstep: t^2 * (3 - 2t)
	float T = CoverAnimAlpha;
	float Smooth = T * T * (3.f - 2.f * T);

	// Cover: fly from current to CoverPos with arc
	if (CoverComp && CoverPosComp)
	{
		FVector CoverEnd = CoverPosComp->GetComponentLocation();
		FVector CoverPos = FMath::Lerp(CoverAnimStartPos, CoverEnd, Smooth);
		CoverPos.Z += FMath::Sin(Smooth * PI) * CoverToolArcHeight;
		CoverComp->SetWorldLocation(CoverPos);

		FQuat CoverEndQuat = CoverPosComp->GetComponentQuat();
		CoverComp->SetWorldRotation(FQuat::Slerp(CoverAnimStartQuat, CoverEndQuat, Smooth));
	}

	// CoverTool: return to original position with arc
	if (CoverToolComp)
	{
		FVector ToolEnd = CoverToolOriginalTransform.GetLocation();
		FVector ToolPos = FMath::Lerp(ToolAnimStartPos, ToolEnd, Smooth);
		ToolPos.Z += FMath::Sin(Smooth * PI) * CoverToolArcHeight * 0.5f;
		CoverToolComp->SetWorldLocation(ToolPos);

		CoverToolComp->SetWorldRotation(FQuat::Slerp(ToolAnimStartQuat, CoverToolOriginalTransform.GetRotation(), Smooth));
	}

	if (CoverAnimAlpha >= 1.f)
	{
		FinishCoverRemoval();
	}
}

void AWorkshopDevice::FinishCoverRemoval()
{
	// Snap to final positions
	if (CoverComp && CoverPosComp)
	{
		CoverComp->SetWorldLocation(CoverPosComp->GetComponentLocation());
		CoverComp->SetWorldRotation(CoverPosComp->GetComponentRotation());
	}
	if (CoverToolComp)
	{
		CoverToolComp->SetWorldTransform(CoverToolOriginalTransform);
		CoverToolComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	CoverState = ECoverRemovalState::Done;
	OnCoverRemoved.Broadcast();
	UE_LOG(LogTemp, Warning, TEXT("CoverPhase - Complete. Transitioning to screw/drag flow."));
}

// ============================================================
// ScrewDriver
// ============================================================

void AWorkshopDevice::TryGrabScrewDriver()
{
	// Handled inline in TryGrabPart via line trace hit
}

void AWorkshopDevice::UpdateScrewDriver(float DeltaTime)
{
	if (!ScrewDriverComp)
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	// Project mouse onto a horizontal plane at screwdriver height
	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		float PlaneZ = ScrewDriverDragTarget.Z;
		if (!FMath::IsNearlyZero(WorldDirection.Z))
		{
			float T = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
			if (T > 0.f)
			{
				FVector PlaneHit = WorldLocation + WorldDirection * T;
				PlaneHit.Z = PlaneZ;
				ScrewDriverDragTarget = PlaneHit;
			}
		}
	}

	// Snap above nearest screw if close enough, otherwise follow mouse
	UStaticMeshComponent* NearestScrew = FindNearestScrew();
	FVector TargetLoc;
	if (NearestScrew)
	{
		TargetLoc = NearestScrew->GetComponentLocation();
		TargetLoc.Z += ScrewDriverHeightAboveScrew;
	}
	else
	{
		TargetLoc = ScrewDriverDragTarget;
	}

	// Smooth transition to target
	FVector CurrentLoc = ScrewDriverComp->GetComponentLocation();
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, DragInterpSpeed);
	ScrewDriverComp->SetWorldLocation(NewLoc);
}

void AWorkshopDevice::ReleaseScrewDriver()
{
	if (!ScrewDriverComp)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ReleaseScrewDriver - DROPPED"));
	bHoldingScrewDriver = false;
	SetupWorkshopCollision(ScrewDriverComp);
	ScrewDriverComp->SetSimulatePhysics(true);
}

// ============================================================
// Screws
// ============================================================

UStaticMeshComponent* AWorkshopDevice::FindNearestScrew() const
{
	if (!ScrewDriverComp)
	{
		return nullptr;
	}

	FVector DriverLoc = ScrewDriverComp->GetComponentLocation();
	UStaticMeshComponent* Nearest = nullptr;
	float NearestDist = ScrewSnapDistance;

	for (const auto& Pair : ScrewToPartMap)
	{
		UStaticMeshComponent* Screw = Pair.Key;
		float Dist = FVector::Dist(DriverLoc, Screw->GetComponentLocation());
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			Nearest = Screw;
		}
	}

	return Nearest;
}

void AWorkshopDevice::StartUnscrew(UStaticMeshComponent* Screw)
{
	ActiveScrew = Screw;
	UnscrewTimer = 0.f;
	ScrewStartLocation = Screw->GetComponentLocation();
	ScrewStartRotation = Screw->GetComponentRotation();

	// Snap screwdriver above the screw
	FVector AboveScrew = ScrewStartLocation;
	AboveScrew.Z += ScrewDriverHeightAboveScrew;
	ScrewDriverComp->SetWorldLocation(AboveScrew);
	ScrewDriverDragTarget = AboveScrew;
}

void AWorkshopDevice::UpdateUnscrew(float DeltaTime)
{
	if (!ActiveScrew || !ScrewDriverComp)
	{
		return;
	}

	UnscrewTimer += DeltaTime;
	float Alpha = FMath::Clamp(UnscrewTimer / UnscrewDuration, 0.f, 1.f);

	// Rotate screw
	FRotator NewRotation = ScrewStartRotation;
	NewRotation.Yaw += UnscrewTimer * ScrewRotationSpeed;

	// Lift screw upward
	FVector NewLocation = ScrewStartLocation;
	NewLocation.Z += Alpha * ScrewLiftDistance;

	ActiveScrew->SetWorldLocationAndRotation(NewLocation, NewRotation);

	// Keep screwdriver above the screw (smooth)
	FVector DriverTarget = NewLocation;
	DriverTarget.Z += ScrewDriverHeightAboveScrew;
	FVector DriverCurrent = ScrewDriverComp->GetComponentLocation();
	ScrewDriverComp->SetWorldLocation(FMath::VInterpTo(DriverCurrent, DriverTarget, DeltaTime, DragInterpSpeed));

	// Rotate screwdriver with the screw
	FRotator DriverRot = ScrewDriverComp->GetComponentRotation();
	DriverRot.Yaw = NewRotation.Yaw;
	ScrewDriverComp->SetWorldRotation(DriverRot);

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

	// Hide the screw
	ActiveScrew->SetVisibility(false);
	ActiveScrew->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Remove from maps
	ScrewToPartMap.Remove(ActiveScrew);
	if (FScrewArray* ScrewArr = PartScrewsMap.Find(PartId))
	{
		ScrewArr->Screws.Remove(ActiveScrew);
	}

	OnScrewRemoved.Broadcast(PartId);

	// Reset screwdriver to free movement
	ScrewDriverDragTarget = ScrewDriverComp->GetComponentLocation();
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

	ActiveScrew = nullptr;
	UnscrewTimer = 0.f;
}
