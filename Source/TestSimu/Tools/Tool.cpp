#include "Tool.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ATool::ATool()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolMesh"));
	ToolMesh->SetupAttachment(SceneRoot);
	ToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ToolMesh->SetOwnerNoSee(true);

	FirstPersonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(SceneRoot);
	FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonMesh->SetOnlyOwnerSee(true);
}

void ATool::Client_AttachFirstPersonMesh_Implementation()
{
	AActor* OwningActor = GetOwner();
	if (OwningActor == nullptr || FirstPersonMesh == nullptr)
	{
		return;
	}

	TArray<USceneComponent*> SceneComps;
	OwningActor->GetComponents<USceneComponent>(SceneComps);
	for (USceneComponent* Comp : SceneComps)
	{
		if (Comp != nullptr && Comp->ComponentHasTag(TEXT("FPCamera")))
		{
			FirstPersonMesh->AttachToComponent(Comp, FAttachmentTransformRules::KeepRelativeTransform);
			return;
		}
	}
}

void ATool::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATool, ToolType);
}

void ATool::BeginPlay()
{
	Super::BeginPlay();
	if (FirstPersonMesh != nullptr)
	{
		FirstPersonMeshDefaultRelativeTransform = FirstPersonMesh->GetRelativeTransform();
	}
}

void ATool::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsPlayingCleanAnim || FirstPersonMesh == nullptr)
	{
		return;
	}

	CleanAnimTime += DeltaTime;
	const float Phase = CleanAnimTime * CleanAnimSpeed;

	FVector Offset = FVector::ZeroVector;
	switch (ToolType)
	{
		case EToolType::Broom:
			Offset.Z = FMath::Sin(Phase) * CleanAnimAmplitude;
			break;
		case EToolType::Sponge:
			Offset.Y = FMath::Cos(Phase) * CleanAnimAmplitude;
			Offset.Z = FMath::Sin(Phase) * CleanAnimAmplitude;
			break;
		default:
			return;
	}

	FTransform NewTransform = FirstPersonMeshDefaultRelativeTransform;
	NewTransform.AddToTranslation(Offset);
	FirstPersonMesh->SetRelativeTransform(NewTransform);
}

void ATool::OnEquipped_Implementation(AActor* NewOwner)
{
}

void ATool::OnUnequipped_Implementation()
{
}

void ATool::UseStart_Implementation()
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (OwningPawn == nullptr || !OwningPawn->IsLocallyControlled())
	{
		return;
	}

	bIsPlayingCleanAnim = true;
	CleanAnimTime = 0.f;
	SetActorTickEnabled(true);
}

void ATool::UseStop_Implementation()
{
	bIsPlayingCleanAnim = false;
	SetActorTickEnabled(false);
	if (FirstPersonMesh != nullptr)
	{
		FirstPersonMesh->SetRelativeTransform(FirstPersonMeshDefaultRelativeTransform);
	}
}
