#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkshopDevice.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartSnappedBack, FName, PartId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPartsRepaired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScrewRemoved, FName, PartId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScrewInserted, FName, PartId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoverRemoved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRepairFinished);

UENUM()
enum class EScrewState : uint8
{
	Snapped,
	Free,
	Attached,
	Screwing
};

UENUM()
enum class ECoverRemovalState : uint8
{
	Inactive,
	ToolAtBase,
	ToolMovingToSnap,
	Pulling,
	Animating,
	Done
};

USTRUCT()
struct FScrewArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Screws;
};

UCLASS()
class TESTSIMU_API AWorkshopDevice : public AActor
{
	GENERATED_BODY()

public:
	AWorkshopDevice();

	UFUNCTION(BlueprintCallable, Category = "Workshop")
	void Repair(const TArray<FName>& BrokenPartIds, FVector SpawnLocation);

	UFUNCTION(BlueprintCallable, Category = "Workshop")
	void StopRepair();

	UFUNCTION(BlueprintCallable, Category = "Workshop")
	UStaticMeshComponent* SpawnNewPart(UStaticMesh* Mesh, FName BrokenPartId);

	UPROPERTY(BlueprintAssignable, Category = "Workshop")
	FOnPartSnappedBack OnPartSnappedBack;

	UPROPERTY(BlueprintAssignable, Category = "Workshop")
	FOnAllPartsRepaired OnAllPartsRepaired;

	UPROPERTY(BlueprintAssignable, Category = "Workshop")
	FOnScrewRemoved OnScrewRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Workshop")
	FOnScrewInserted OnScrewInserted;

	UPROPERTY(BlueprintAssignable, Category = "Workshop")
	FOnCoverRemoved OnCoverRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Workshop")
	FOnRepairFinished OnRepairFinished;

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float SpawnHeightOffset = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	FVector SpawnOffset = FVector(-30.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	UMaterialInterface* BrokenMatOverlay = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float SnapDistanceThreshold = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float DragTraceDistance = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float DragInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float DragScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float DragLiftHeight = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float MaxDragDistance = 0.f;

	// --- Screw Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float UnscrewDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewRotationSpeed = 720.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewLiftDistance = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewSnapDistance = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewDriverHeightAboveScrew = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewDriverPickupLift = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewPickupDistance = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewSlotSnapDistance = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float ScrewInDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	float MaxScrewDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Screws")
	FVector ScrewAttachOffset = FVector(0.f, 0.f, 0.f);

	// --- Cover Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover")
	bool bHasCover = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverPullThreshold = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverToolMoveSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverAnimSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverToolArcHeight = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverPullRotationScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverPullVisualScale = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Cover", meta = (EditCondition = "bHasCover"))
	float CoverPullMaxOffset = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float ResetAnimSpeed = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Debug")
	bool bDebugTrace = false;

private:
	// --- Cover Pull ---
	void InitCoverPhase();
	void UpdateCoverPhase(float DeltaTime);
	void UpdateCoverToolMoving(float DeltaTime);
	void UpdateCoverPulling(float DeltaTime);
	void UpdateCoverAnimating(float DeltaTime);
	void FinishCoverRemoval();

	// --- Part Drag ---
	void TryGrabPart();
	void UpdateDrag(float DeltaTime);
	void ReleasePart();
	void SnapToSlot(UStaticMeshComponent* Comp, FName PartId);
	bool IsSlotRepaired(FName PartId) const;
	bool AreAllSlotsRepaired() const;
	bool IsPartLockedByScrews(FName PartId) const;
	FName GetBasePartId(UStaticMeshComponent* Comp) const;
	bool IsBrokenTag(const FName& Tag) const;

	// --- ScrewDriver ---
	void TryGrabScrewDriver();
	void UpdateScrewDriver(float DeltaTime);
	void ReleaseScrewDriver();

	// --- Screws ---
	void RegisterScrews();
	UStaticMeshComponent* FindNearestScrew() const;
	void StartUnscrew(UStaticMeshComponent* Screw);
	void UpdateUnscrew(float DeltaTime);
	void FinishUnscrew();
	void CancelUnscrew();

	// --- Screw Pickup & Insertion ---
	UStaticMeshComponent* FindNearestFreeScrew() const;
	void AttachScrewToDriver(UStaticMeshComponent* Screw);
	void DetachScrewFromDriver();
	UStaticMeshComponent* FindNearestEmptyScrewSlot() const;
	void StartScrewIn(UStaticMeshComponent* SlotScrew);
	void UpdateScrewIn(float DeltaTime);
	void FinishScrewIn();
	void CancelScrewIn();
	bool AreAllScrewsInserted() const;
	void ConstrainFreeScrews();

	// --- Reset Animation ---
	void StartResetAnimation();
	void UpdateResetAnimation(float DeltaTime);
	void FinishResetAnimation();

	// --- Collision ---
	static constexpr ECollisionChannel WorkshopChannel = ECC_GameTraceChannel1;
	void SetupWorkshopCollision(UStaticMeshComponent* Comp);

	// --- Tags ---
	static const FString BrokenSuffix;
	static const FName ScrewTag;
	static const FName ScrewDriverTag;
	static const FName CoverToolTag;
	static const FName CoverTag;
	static const FName CoverToolPosTag;
	static const FName CoverPosTag;

	// --- Part State ---
	UPROPERTY()
	TArray<FName> ActiveBrokenPartIds;

	UPROPERTY()
	TMap<FName, FTransform> SlotTransforms;

	UPROPERTY()
	TMap<FName, TObjectPtr<UStaticMeshComponent>> SlotOccupants;

	UPROPERTY()
	UStaticMeshComponent* DraggedComponent = nullptr;

	FVector DragTargetLocation = FVector::ZeroVector;

	// --- Screw State ---
	UPROPERTY()
	TMap<TObjectPtr<UStaticMeshComponent>, FName> ScrewToPartMap;

	UPROPERTY()
	TMap<FName, FScrewArray> PartScrewsMap;

	// --- ScrewDriver State ---
	UPROPERTY()
	UStaticMeshComponent* ScrewDriverComp = nullptr;

	bool bHoldingScrewDriver = false;
	FTransform ScrewDriverOriginalTransform;
	FVector ScrewDriverDragTarget = FVector::ZeroVector;

	// --- Unscrew State ---
	UPROPERTY()
	UStaticMeshComponent* ActiveScrew = nullptr;

	float UnscrewTimer = 0.f;
	FVector ScrewStartLocation = FVector::ZeroVector;
	FRotator ScrewStartRotation = FRotator::ZeroRotator;

	// --- Hover ---
	void UpdateHover();

	UPROPERTY()
	UStaticMeshComponent* HoveredComponent = nullptr;

	// --- Cover State ---
	ECoverRemovalState CoverState = ECoverRemovalState::Inactive;

	UPROPERTY()
	UStaticMeshComponent* CoverToolComp = nullptr;

	UPROPERTY()
	UStaticMeshComponent* CoverComp = nullptr;

	UPROPERTY()
	USceneComponent* CoverToolPosComp = nullptr;

	UPROPERTY()
	USceneComponent* CoverPosComp = nullptr;

	FTransform CoverToolOriginalTransform;
	FTransform CoverOriginalTransform;
	float CoverToolMoveAlpha = 0.f;
	float CoverPullAccumulator = 0.f;
	float CoverAnimAlpha = 0.f;

	FVector CoverAnimStartPos = FVector::ZeroVector;
	FQuat CoverAnimStartQuat = FQuat::Identity;
	FVector ToolAnimStartPos = FVector::ZeroVector;
	FQuat ToolAnimStartQuat = FQuat::Identity;

	// --- Reset Animation State ---
	bool bResettingAfterRepair = false;
	float ResetAnimAlpha = 0.f;
	FVector ResetCoverStartPos = FVector::ZeroVector;
	FQuat ResetCoverStartQuat = FQuat::Identity;
	FVector ResetCoverToolStartPos = FVector::ZeroVector;
	FQuat ResetCoverToolStartQuat = FQuat::Identity;
	FVector ResetScrewDriverStartPos = FVector::ZeroVector;
	FQuat ResetScrewDriverStartQuat = FQuat::Identity;

	// --- Input ---
	bool bWasLeftMouseDown = false;
	bool bIsRepairing = false;
};
