#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "XRayScanner.generated.h"

USTRUCT()
struct FMaterialSlotArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBrokenPartFound, FName, PartId, int32, TotalFound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllBrokenPartsFound);

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AXRayScanner : public AActor
{
	GENERATED_BODY()

public:
	AXRayScanner();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XRay Scanner")
	TObjectPtr<USceneComponent> TraceOrigin;

	// --- Configuration ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	TObjectPtr<UMaterialInterface> XRayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float TraceDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	FName XRayTag = FName("XRayScanner");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	FName RepairOrderTag = FName("RepairOrder");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner|Material")
	FName HitLocationParamName = FName("ScanCenter");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner|Material")
	FName BaseColorTextureParamName = FName("BaseColorTexture");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float MouseSensitivity = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float MoveInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRay Scanner|Bounds")
	FVector BoundsMin = FVector(-50.f, -50.f, -50.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRay Scanner|Bounds")
	FVector BoundsMax = FVector(50.f, 50.f, 50.f);

	// --- Blueprint API ---

	UFUNCTION(BlueprintCallable, Category = "XRay Scanner")
	void ActivateScanner(FVector Location, FRotator Rotation, const TArray<FName>& InBrokenPartIds);

	UFUNCTION(BlueprintCallable, Category = "XRay Scanner")
	void DeactivateScanner();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XRay Scanner")
	bool IsScanning() const { return bIsScanning; }

	UFUNCTION(BlueprintCallable, Category = "XRay Scanner")
	void SetBrokenPartIds(const TArray<FName>& InBrokenPartIds);

	UPROPERTY(BlueprintAssignable, Category = "XRay Scanner")
	FOnBrokenPartFound OnBrokenPartFound;

	UPROPERTY(BlueprintAssignable, Category = "XRay Scanner")
	FOnAllBrokenPartsFound OnAllBrokenPartsFound;

protected:
	virtual void Tick(float DeltaTime) override;

private:
	bool bIsScanning = false;
	FVector TargetLocation = FVector::ZeroVector;
	FVector ActivationOrigin = FVector::ZeroVector;

	UPROPERTY()
	TArray<FName> BrokenPartIds;

	UPROPERTY()
	TArray<FName> FoundBrokenParts;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMeshComponent>, FMaterialSlotArray> AffectedMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveDynamicMaterials;

	void UpdatePositionFromMouse(float DeltaTime);
	void PerformScanTrace();
	void ApplyXRayMaterial(UStaticMeshComponent* MeshComp, const FVector& HitLocation);
	void RestoreAllMaterials();
	void CheckForBrokenPart(UStaticMeshComponent* MeshComp);
	void OnAllPartsFoundEventDelay();
	void OnAllPartsFoundHideDelay();
	APlayerController* GetPlayerController() const;

	FTimerHandle AllPartsFoundEventTimerHandle;
	FTimerHandle AllPartsFoundHideTimerHandle;
};
