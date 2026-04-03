#pragma once

#include "CoreMinimal.h"
#include "Tools/Tool.h"
#include "XRayScanner.generated.h"

USTRUCT()
struct FMaterialSlotArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API AXRayScanner : public ATool
{
	GENERATED_BODY()

public:
	AXRayScanner();

	// --- Configuration ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	TObjectPtr<UMaterialInterface> XRayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float TraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float TargetFOV = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	float FOVInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	FName XRayTag = FName("XRayScanner");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner")
	FName RepairOrderTag = FName("RepairOrder");

	/** Material parameter name for the sphere mask center position */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "XRay Scanner|Material")
	FName HitLocationParamName = FName("ScanCenter");

protected:
	virtual void UseStart_Implementation() override;
	virtual void UseStop_Implementation() override;
	virtual void OnUnequipped_Implementation() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bIsScanning = false;
	float OriginalFOV = 90.f;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMeshComponent>, FMaterialSlotArray> AffectedMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveDynamicMaterials;

	void PerformScanTrace();
	void ApplyXRayMaterial(UStaticMeshComponent* MeshComp, const FVector& HitLocation);
	void RestoreAllMaterials();
	APlayerController* GetOwnerPlayerController() const;
};
