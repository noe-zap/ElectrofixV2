#include "FootstepComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#endif

UFootstepComponent::UFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

void UFootstepComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	OwnerMovement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;

	if (!OwnerCharacter || !OwnerMovement)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}

	DistanceAccumulator = WalkStrideDistance;
	LastMovementMode = OwnerMovement->MovementMode;
}

void UFootstepComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter || !OwnerMovement)
	{
		return;
	}

	const EMovementMode CurrentMode = OwnerMovement->MovementMode;

	if (LastMovementMode == MOVE_Falling && CurrentMode == MOVE_Walking && LandingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			LandingSound,
			OwnerCharacter->GetActorLocation(),
			VolumeMultiplier,
			PitchMultiplier,
			0.f,
			AttenuationOverride);
	}
	LastMovementMode = CurrentMode;

	if (CurrentMode != MOVE_Walking)
	{
		DistanceAccumulator = WalkStrideDistance;
		return;
	}

	const float Speed2D = OwnerCharacter->GetVelocity().Size2D();
	if (Speed2D < MinSpeedThreshold)
	{
		return;
	}

	DistanceAccumulator += Speed2D * DeltaTime;

	const float Stride = (Speed2D >= RunSpeedThreshold) ? RunStrideDistance : WalkStrideDistance;

	if (DistanceAccumulator >= Stride)
	{
		PlayFootstep();
		DistanceAccumulator -= Stride;
	}
}

void UFootstepComponent::PlayFootstep()
{
	if (!FootstepSound || !OwnerCharacter)
	{
		return;
	}

	const FVector Location = OwnerCharacter->GetActorLocation();

	UGameplayStatics::PlaySoundAtLocation(
		this,
		FootstepSound,
		Location,
		VolumeMultiplier,
		PitchMultiplier,
		0.f,
		AttenuationOverride);

#if ENABLE_DRAW_DEBUG
	if (bDebugDraw)
	{
		DrawDebugSphere(GetWorld(), Location, 15.f, 8, FColor::Yellow, false, 0.25f);
	}
#endif
}
