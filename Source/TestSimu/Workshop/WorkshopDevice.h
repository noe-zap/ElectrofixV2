#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkshopDevice.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartSnappedBack, FName, PartId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPartsRepaired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScrewRemoved, FName, PartId);

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

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Parts")
	float SpawnHeightOffset = 30.f;

	// --- Part Drag Settings ---
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
	float ScrewEjectImpulse = 200.f;

	// --- Debug ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Debug")
	bool bDebugTrace = false;

private:
	// --- Part Drag ---
	void TryGrabPart();
	void UpdateDrag(float DeltaTime);
	void ReleasePart();
	void SnapToSlot(UStaticMeshComponent* Comp, FName PartId);
	bool IsSlotRepaired(FName PartId) const;
	bool AreAllSlotsRepaired() const;
	bool IsPartLockedByScrews(FName PartId) const;
	FName GetPartId(UStaticMeshComponent* Comp) const;

	// --- Screw Driver ---
	void TryPickUpScrewDriver();
	void DropScrewDriver();
	void UpdateScrewDriverPosition(float DeltaTime);

	// --- Screw Unscrewing ---
	void TryStartUnscrew();
	void UpdateUnscrew(float DeltaTime);
	void FinishUnscrew();
	void CancelUnscrew();
	void RegisterScrews();

	// --- Collision ---
	static constexpr ECollisionChannel WorkshopChannel = ECC_GameTraceChannel1;
	void SetupWorkshopCollision(UStaticMeshComponent* Comp);

	// --- Tags ---
	static const FName BrokenTag;
	static const FName ScrewTag;
	static const FName ScrewDriverTag;

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
	FVector ScrewDriverTargetLocation = FVector::ZeroVector;
	FTransform ScrewDriverOriginalTransform;

	// --- Unscrew State ---
	UPROPERTY()
	UStaticMeshComponent* ActiveScrew = nullptr;

	float UnscrewTimer = 0.f;
	FVector ScrewStartLocation = FVector::ZeroVector;
	FRotator ScrewStartRotation = FRotator::ZeroRotator;

	// --- Input ---
	bool bWasLeftMouseDown = false;
	bool bWasRightMouseDown = false;
	bool bIsRepairing = false;
};
