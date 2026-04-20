#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialArrow.generated.h"

class UStaticMeshComponent;

// Reserved CustomDepth stencil value used by the tutorial arrow, matched in the post-process
// outline material (M_PP_Outline) to render this mesh as always-visible through walls.
#define TUTORIAL_ARROW_STENCIL 250

UCLASS()
class TESTSIMU_API ATutorialArrow : public AActor
{
	GENERATED_BODY()

public:
	ATutorialArrow();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial|Arrow")
	TObjectPtr<UStaticMeshComponent> ArrowMesh;

	// Vertical bob amplitude (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Arrow")
	float BobAmplitude = 8.f;

	// Vertical bob speed (hz).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Arrow")
	float BobFrequency = 1.5f;

	// Spin speed (deg/s). 0 = no spin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Arrow")
	float SpinSpeed = 45.f;

	// Set a new follow target. Server-only; replicates to clients.
	UFUNCTION(BlueprintCallable, Category = "Tutorial|Arrow")
	void SetTarget(AActor* NewTarget, FVector Offset);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Target)
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Replicated)
	FVector TargetOffset = FVector(0.f, 0.f, 120.f);

	UFUNCTION()
	void OnRep_Target();

	float BobPhase = 0.f;
	float SpinYaw = 0.f;

	// If TargetActor weakptr hasn't resolved yet on a client (late join), retry for a few seconds.
	FTimerHandle TargetResolveTimer;
	float TargetResolveElapsed = 0.f;

	void TryApplyTargetVisibility();
};
