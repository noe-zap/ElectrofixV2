// Velocity-driven footstep playback for replicated Characters.
// Ticks locally on every machine that has audio and reads the locally-observed
// (replicated) velocity, so coop clients hear each other's footsteps without
// any RPC. See /home/noe/.claude/plans/hi-i-would-like-silly-fern.md for the
// design rationale.
//
// TODO(future): for tighter foot-plant sync, replace the velocity-driven trigger
// with a UAnimNotify placed on the locomotion sequences (same playback path).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "FootstepComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class USoundAttenuation;
class USoundBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESTSIMU_API UFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootstepComponent();

	// --- Audio ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Audio")
	TObjectPtr<USoundBase> FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Audio")
	TObjectPtr<USoundAttenuation> AttenuationOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Audio", meta=(ClampMin="0.0"))
	float VolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Audio", meta=(ClampMin="0.0"))
	float PitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Audio")
	TObjectPtr<USoundBase> LandingSound;

	// --- Cadence ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Cadence", meta=(ClampMin="1.0"))
	float WalkStrideDistance = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Cadence", meta=(ClampMin="1.0"))
	float RunStrideDistance = 175.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Cadence", meta=(ClampMin="0.0"))
	float RunSpeedThreshold = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Cadence", meta=(ClampMin="0.0"))
	float MinSpeedThreshold = 50.f;

	// --- Debug ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Debug")
	bool bDebugDraw = false;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> OwnerMovement;

	float DistanceAccumulator = 0.f;
	TEnumAsByte<EMovementMode> LastMovementMode = MOVE_None;

	void PlayFootstep();
};
