#include "Tool.h"
#include "Components/StaticMeshComponent.h"

ATool::ATool()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolMesh"));
	ToolMesh->SetupAttachment(SceneRoot);
	ToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATool::OnEquipped_Implementation(AActor* NewOwner)
{
}

void ATool::OnUnequipped_Implementation()
{
}

void ATool::UseStart_Implementation()
{
}

void ATool::UseStop_Implementation()
{
}
