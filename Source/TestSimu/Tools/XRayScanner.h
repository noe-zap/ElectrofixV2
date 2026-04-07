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

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AXRayScanner : public AActor
{
	GENERATED_BODY()

public:
	AXRayScanner();

	// --- Components ---

	/** The point from which the line trace originates. Position it in Blueprint. */
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

	/** Material parameter name for the sphere mask center position */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner|Material")
	FName HitLocationParamName = FName("ScanCenter");

	/** How fast the scanner moves with the mouse */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float MouseSensitivity = 0.1f;

	/** How fast the scanner interpolates to the target position (lower = more lag/sway) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float MoveInterpSpeed = 8.f;

	/** Min bounds offset relative to the activation location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRay Scanner|Bounds")
	FVector BoundsMin = FVector(-50.f, -50.f, -50.f);

	/** Max bounds offset relative to the activation location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRay Scanner|Bounds")
	FVector BoundsMax = FVector(50.f, 50.f, 50.f);

	// --- Blueprint API ---

	/** Call this from Blueprint to activate the scanner at a given location and rotation. */
	UFUNCTION(BlueprintCallable, Category = "XRay Scanner")
	void ActivateScanner(FVector Location, FRotator Rotation);

	/** Call this from Blueprint to deactivate. Restores all materials and stops ticking. */
	UFUNCTION(BlueprintCallable, Category = "XRay Scanner")
	void DeactivateScanner();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "XRay Scanner")
	bool IsScanning() const { return bIsScanning; }

protected:
	virtual void Tick(float DeltaTime) override;

private:
	bool bIsScanning = false;
	FVector TargetLocation = FVector::ZeroVector;
	FVector ActivationOrigin = FVector::ZeroVector;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMeshComponent>, FMaterialSlotArray> AffectedMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveDynamicMaterials;

	void UpdatePositionFromMouse(float DeltaTime);
	void PerformScanTrace();
	void ApplyXRayMaterial(UStaticMeshComponent* MeshComp, const FVector& HitLocation);
	void RestoreAllMaterials();
	APlayerController* GetPlayerController() const;
};
