#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CleanableActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API ACleanableActor : public AActor
{
	GENERATED_BODY()

public:
	ACleanableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cleanable")
	UStaticMeshComponent* Mesh;

	// Identifies which cleaning tool can clean this actor (e.g. "Sweep", "Scrub").
	// Auto-applied to Mesh ComponentTags so the cleaning tool's line trace finds it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleanable")
	FName CleaningTag = NAME_None;

	// Offset applied to the anchor transform when spawning. Use it to lift the stain off the
	// floor when the mesh pivot isn't on the bottom of the mesh.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleanable")
	FVector SpawnOffset = FVector::ZeroVector;

	// Scalar parameter name in the material that drives visibility (1 = fully dirty, 0 = clean).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleanable")
	FName OpacityParameterName = FName("Opacity");

	UPROPERTY(ReplicatedUsing = OnRep_DirtAmount, BlueprintReadOnly, Category = "Cleanable")
	float DirtAmount = 1.f;

	// Server-only. Reduces dirt; destroys actor when fully cleaned.
	UFUNCTION(BlueprintCallable, Category = "Cleanable")
	void ApplyClean(float Amount);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UFUNCTION()
	void OnRep_DirtAmount();

	void ApplyOpacityToMaterial();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynMat;
};
