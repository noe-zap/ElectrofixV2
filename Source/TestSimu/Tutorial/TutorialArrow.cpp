#include "TutorialArrow.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ATutorialArrow::ATutorialArrow()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false); // we drive position each tick locally

	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	RootComponent = ArrowMesh;
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowMesh->SetCastShadow(false);

	// Render through walls via the existing M_PP_Outline post-process stencil branch.
	ArrowMesh->SetRenderCustomDepth(true);
	ArrowMesh->SetCustomDepthStencilValue(TUTORIAL_ARROW_STENCIL);
}

void ATutorialArrow::BeginPlay()
{
	Super::BeginPlay();
	// Re-assert stencil in case Blueprint subclass overrode the mesh in CDO.
	if (ArrowMesh)
	{
		ArrowMesh->SetRenderCustomDepth(true);
		ArrowMesh->SetCustomDepthStencilValue(TUTORIAL_ARROW_STENCIL);
	}
}

void ATutorialArrow::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATutorialArrow, TargetActor);
	DOREPLIFETIME(ATutorialArrow, TargetOffset);
	DOREPLIFETIME(ATutorialArrow, BaseRotation);
}

void ATutorialArrow::SetBaseRotation(FRotator NewRotation)
{
	if (!HasAuthority())
	{
		return;
	}
	BaseRotation = NewRotation;
}

void ATutorialArrow::SetTarget(AActor* NewTarget, FVector Offset)
{
	if (!HasAuthority())
	{
		return;
	}

	TargetActor = NewTarget;
	TargetOffset = Offset;

	// Server also runs OnRep logic locally for consistency.
	OnRep_Target();
}

void ATutorialArrow::OnRep_Target()
{
	TargetResolveElapsed = 0.f;
	TryApplyTargetVisibility();
}

void ATutorialArrow::TryApplyTargetVisibility()
{
	if (TargetActor.IsValid())
	{
		SetActorHiddenInGame(false);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TargetResolveTimer);
		}
		return;
	}

	// Hide until we resolve; retry for up to ~3 s on clients where target hasn't replicated yet.
	SetActorHiddenInGame(true);
	TargetResolveElapsed += 0.25f;
	if (TargetResolveElapsed < 3.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				TargetResolveTimer, this, &ATutorialArrow::TryApplyTargetVisibility, 0.25f, false);
		}
	}
}

void ATutorialArrow::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* T = TargetActor.Get();
	if (!T)
	{
		return;
	}

	BobPhase += DeltaTime * BobFrequency * 2.f * PI;
	SpinYaw += DeltaTime * SpinSpeed;

	FVector DesiredLoc = T->GetActorLocation() + TargetOffset;
	DesiredLoc.Z += FMath::Sin(BobPhase) * BobAmplitude;

	SetActorLocation(DesiredLoc);

	const FQuat BaseQuat = BaseRotation.Quaternion();
	const FQuat SpinQuat(FVector::UpVector, FMath::DegreesToRadians(SpinYaw));
	SetActorRotation(SpinQuat * BaseQuat);
}
